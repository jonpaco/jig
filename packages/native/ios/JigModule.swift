// ios/JigModule.swift

import ExpoModulesCore

public class JigModule: Module {
    private var server: JigWebSocketServer?

    public func definition() -> ModuleDefinition {
        Name("Jig")

        AsyncFunction("start") { (config: [String: Any]) in
            if self.server != nil { return }

            let rnVersion = config["rnVersion"] as? String ?? "unknown"
            let expoVersion = config["expoVersion"] as? String
            let port = UInt16(config["port"] as? Int ?? 4042)

            let appName = Bundle.main.object(
                forInfoDictionaryKey: "CFBundleName") as? String ?? "Unknown"
            let bundleId = Bundle.main.bundleIdentifier ?? "unknown"

            let appInfo = JigAppInfo(
                name: appName,
                bundleId: bundleId,
                platform: "ios",
                rnVersion: rnVersion,
                expoVersion: expoVersion
            )

            let supportedDomains: [String] = []

            let middlewares: [JigMiddleware] = [
                JigHandshakeGate(),
                JigDomainGuard(supportedDomains: supportedDomains),
            ]
            let handlers: [JigHandler] = [
                JigHandshakeHandler(),
                JigScreenshotHandler(),
            ]

            let queue = DispatchQueue(label: "jig.websocket")
            let dispatcher = JigDispatcher(
                middlewares: middlewares,
                handlers: handlers,
                supportedDomains: supportedDomains,
                queue: queue
            )

            let server = JigWebSocketServer(
                port: port,
                dispatcher: dispatcher,
                appInfo: appInfo,
                queue: queue
            )
            server.start()
            self.server = server
        }

        Function("stop") {
            self.server?.stop()
            self.server = nil
        }

        Function("isRunning") { () -> Bool in
            return self.server != nil
        }
    }
}
