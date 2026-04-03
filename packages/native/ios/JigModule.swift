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

            let handler = JigHandshakeHandler(
                rnVersion: rnVersion,
                expoVersion: expoVersion
            )

            var error: NSError?
            let server = JigWebSocketServer(port: port, handler: handler, error: &error)
            if let error = error { throw error }
            server?.start()
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
