import ExpoModulesCore

public class JigModule: Module {
    private var server: JigWebSocketServer?

    public func definition() -> ModuleDefinition {
        Name("Jig")

        AsyncFunction("start") { (config: [String: Any]) in
            // Delegates to Obj-C — implemented in Task 4
        }

        Function("stop") {
            // Delegates to Obj-C — implemented in Task 4
        }

        Function("isRunning") { () -> Bool in
            return self.server != nil
        }
    }
}
