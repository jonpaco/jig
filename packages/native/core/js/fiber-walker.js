(function() {
  'use strict';

  /* === Fiber Walker === */

  function getComponentName(fiber) {
    var parent = fiber.return;
    while (parent) {
      if (typeof parent.type === 'function' || typeof parent.type === 'object') {
        var name = parent.type.displayName || parent.type.name;
        if (name && name !== 'View' && name !== 'RCTView' &&
            name !== 'Text' && name !== 'RCTText') {
          return name;
        }
      }
      parent = parent.return;
    }
    return null;
  }

  function extractProps(fiber) {
    var props = fiber.memoizedProps;
    if (!props || typeof props !== 'object') return null;
    var result = {};
    var hasAny = false;
    for (var key in props) {
      var val = props[key];
      var t = typeof val;
      if (t === 'string' || t === 'number' || t === 'boolean' || val === null) {
        result[key] = val;
        hasAny = true;
      }
    }
    return hasAny ? result : null;
  }

  function walkFiber(fiber, result, includeProps) {
    if (!fiber) return;
    if (fiber.tag === 5 && fiber.stateNode) {
      var entry = {
        reactTag: fiber.stateNode._nativeTag,
        component: getComponentName(fiber)
      };
      if (includeProps) {
        var parent = fiber.return;
        while (parent) {
          if (typeof parent.type === 'function' || typeof parent.type === 'object') {
            var name = parent.type.displayName || parent.type.name;
            if (name && name !== 'View' && name !== 'RCTView' &&
                name !== 'Text' && name !== 'RCTText') {
              var props = extractProps(parent);
              if (props) entry.props = props;
              break;
            }
          }
          parent = parent.return;
        }
      }
      result.push(entry);
    }
    walkFiber(fiber.child, result, includeProps);
    walkFiber(fiber.sibling, result, includeProps);
  }

  function doWalk(includeProps) {
    var hook = global.__REACT_DEVTOOLS_GLOBAL_HOOK__;
    var roots = hook && hook.getFiberRoots ? hook.getFiberRoots(1) : null;
    if (!roots || roots.size === 0) return [];

    var result = [];
    roots.forEach(function(root) {
      walkFiber(root.current, result, includeProps);
    });
    return result;
  }

  /* === Internal WebSocket Client === */

  var ws = null;
  var reconnectTimer = null;
  var handshakeComplete = false;

  function connect() {
    try {
      ws = new WebSocket('ws://127.0.0.1:4042');
    } catch (e) {
      scheduleReconnect();
      return;
    }

    ws.onopen = function() {
      /* server sends server.hello automatically */
    };

    ws.onmessage = function(evt) {
      try {
        var msg = JSON.parse(evt.data);
      } catch (e) {
        return;
      }

      if (msg.method === 'server.hello') {
        ws.send(JSON.stringify({
          jsonrpc: '2.0',
          id: 1,
          method: 'client.hello',
          params: {
            protocol: { version: 1 },
            client: { name: '@jig/internal', version: '0.1.0' }
          }
        }));
        return;
      }

      if (msg.id === 1 && msg.result) {
        handshakeComplete = true;
        ws.send(JSON.stringify({
          jsonrpc: '2.0',
          id: 2,
          method: 'jig.internal.ready',
          params: {}
        }));
        return;
      }

      if (msg.method === 'jig.internal.walkFibers') {
        var includeProps = msg.params && msg.params.includeProps === true;
        var fibers = doWalk(includeProps);
        ws.send(JSON.stringify({
          jsonrpc: '2.0',
          method: 'jig.internal.fiberData',
          params: { fibers: fibers }
        }));
        return;
      }
    };

    ws.onclose = function() {
      handshakeComplete = false;
      scheduleReconnect();
    };

    ws.onerror = function() {
      /* onclose will fire after onerror */
    };
  }

  function scheduleReconnect() {
    if (reconnectTimer) return;
    reconnectTimer = setTimeout(function() {
      reconnectTimer = null;
      connect();
    }, 1000);
  }

  connect();
})();
