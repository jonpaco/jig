// ios/JigWebSocketServer.m

#import "JigWebSocketServer.h"
#import "JigDispatcher.h"
#import "JigSessionContext.h"
#import "JigAppInfo.h"

@interface JigWebSocketServer ()
@property (nonatomic, strong) nw_listener_t listener;
@property (nonatomic, strong) dispatch_queue_t queue;
@property (nonatomic, strong) NSMutableDictionary<NSNumber *, JigSessionContext *> *contexts;
@property (nonatomic, assign) NSInteger nextConnectionId;
@property (nonatomic, strong) JigDispatcher *dispatcher;
@property (nonatomic, strong) JigAppInfo *appInfo;
@end

@implementation JigWebSocketServer

- (instancetype)initWithPort:(uint16_t)port
                  dispatcher:(JigDispatcher *)dispatcher
                     appInfo:(JigAppInfo *)appInfo
                       queue:(dispatch_queue_t)queue {
    self = [super init];
    if (self) {
        _dispatcher = dispatcher;
        _appInfo = appInfo;
        _queue = queue;
        _contexts = [NSMutableDictionary new];
        _nextConnectionId = 0;

        nw_parameters_t parameters = nw_parameters_create_secure_tcp(
            NW_PARAMETERS_DISABLE_PROTOCOL, NW_PARAMETERS_DEFAULT_CONFIGURATION);
        nw_parameters_set_reuse_local_address(parameters, true);

        nw_protocol_options_t wsOptions = nw_ws_create_options(nw_ws_version_13);
        nw_protocol_stack_t stack = nw_parameters_copy_default_protocol_stack(parameters);
        nw_protocol_stack_prepend_application_protocol(stack, wsOptions);

        _listener = nw_listener_create_with_port(
            [[NSString stringWithFormat:@"%d", port] UTF8String],
            parameters);

        if (!_listener) {
            NSLog(@"[Jig] Failed to create listener on port %d", port);
            return nil;
        }
    }
    return self;
}

- (void)start {
    __weak typeof(self) weakSelf = self;

    nw_listener_set_state_changed_handler(self.listener,
        ^(nw_listener_state_t state, nw_error_t _Nullable err) {
        if (state == nw_listener_state_ready) {
            NSLog(@"[Jig] WebSocket server listening on port %d",
                  nw_listener_get_port(weakSelf.listener));
        } else if (state == nw_listener_state_failed) {
            NSLog(@"[Jig] Listener failed");
        }
    });

    nw_listener_set_new_connection_handler(self.listener,
        ^(nw_connection_t connection) {
        [weakSelf handleNewConnection:connection];
    });

    nw_listener_set_queue(self.listener, self.queue);
    nw_listener_start(self.listener);
}

- (void)stop {
    nw_listener_cancel(self.listener);
    for (JigSessionContext *ctx in self.contexts.allValues) {
        if (ctx.cancel) {
            ctx.cancel();
        }
    }
    [self.contexts removeAllObjects];
}

#pragma mark - Connection handling

- (void)handleNewConnection:(nw_connection_t)connection {
    NSInteger connId = self.nextConnectionId++;

    JigSessionContext *context = [[JigSessionContext alloc]
        initWithConnectionId:connId appInfo:self.appInfo];

    __weak typeof(self) weakSelf = self;
    context.sendText = ^(NSString *text) {
        [weakSelf sendText:text toConnection:connection];
    };
    context.cancel = ^{
        nw_connection_cancel(connection);
    };

    self.contexts[@(connId)] = context;

    nw_connection_set_state_changed_handler(connection,
        ^(nw_connection_state_t state, nw_error_t _Nullable err) {
        if (state == nw_connection_state_ready) {
            NSLog(@"[Jig] Client %ld connected", (long)connId);
            [weakSelf.dispatcher handleOpen:context];
            [weakSelf receiveMessagesOnConnection:connection id:connId];
        } else if (state == nw_connection_state_failed ||
                   state == nw_connection_state_cancelled) {
            [weakSelf.contexts removeObjectForKey:@(connId)];
        }
    });

    nw_connection_set_queue(connection, self.queue);
    nw_connection_start(connection);
}

- (void)receiveMessagesOnConnection:(nw_connection_t)connection
                                 id:(NSInteger)connId {
    __weak typeof(self) weakSelf = self;

    nw_connection_receive_message(connection,
        ^(dispatch_data_t _Nullable content, nw_content_context_t _Nullable ctx,
          bool is_complete, nw_error_t _Nullable error) {

        if (error || !content) {
            [weakSelf.contexts removeObjectForKey:@(connId)];
            nw_connection_cancel(connection);
            return;
        }

        const void *buffer;
        size_t size;
        dispatch_data_t mapped = dispatch_data_create_map(content, &buffer, &size);
        NSString *text = [[NSString alloc] initWithBytes:buffer
                                                  length:size
                                                encoding:NSUTF8StringEncoding];
        (void)mapped; // prevent early release

        if (text) {
            JigSessionContext *sessionCtx = weakSelf.contexts[@(connId)];
            if (sessionCtx) {
                [weakSelf.dispatcher dispatch:text context:sessionCtx];
            }
        }

        [weakSelf receiveMessagesOnConnection:connection id:connId];
    });
}

- (void)sendText:(NSString *)text toConnection:(nw_connection_t)connection {
    NSData *data = [text dataUsingEncoding:NSUTF8StringEncoding];
    dispatch_data_t dispatchData = dispatch_data_create(
        data.bytes, data.length, self.queue, DISPATCH_DATA_DESTRUCTOR_DEFAULT);

    nw_protocol_metadata_t metadata = nw_ws_create_metadata(nw_ws_opcode_text);
    nw_content_context_t context = nw_content_context_create("jigMessage");
    nw_content_context_set_metadata_for_protocol(context, metadata);

    nw_connection_send(connection, dispatchData, context, true,
        ^(nw_error_t _Nullable error) {
        if (error) {
            NSLog(@"[Jig] Send error: %@", error);
        }
    });
}

@end
