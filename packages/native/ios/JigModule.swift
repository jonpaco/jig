// ios/JigModule.swift

import ExpoModulesCore

public class JigModule: Module {
    public func definition() -> ModuleDefinition {
        Name("Jig")

        AsyncFunction("start") { (config: [String: Any]) in
            if JigBridge.isRunning() { return }

            let rnVersion = config["rnVersion"] as? String ?? "unknown"
            let expoVersion = config["expoVersion"] as? String
            let port = Int32(config["port"] as? Int ?? 4042)

            let appName = Bundle.main.object(
                forInfoDictionaryKey: "CFBundleName") as? String ?? "Unknown"
            let bundleId = Bundle.main.bundleIdentifier ?? "unknown"

            JigBridge.startServer(
                withName: appName,
                bundleId: bundleId,
                rnVersion: rnVersion,
                expoVersion: expoVersion,
                port: port
            )
        }

        Function("stop") {
            JigBridge.stopServer()
        }

        Function("isRunning") { () -> Bool in
            return JigBridge.isRunning()
        }
    }
}
