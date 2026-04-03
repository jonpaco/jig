package expo.modules.jig

import expo.modules.kotlin.modules.Module
import expo.modules.kotlin.modules.ModuleDefinition

class JigModule : Module() {
    private var server: JigWebSocketServer? = null

    override fun definition() = ModuleDefinition {
        Name("Jig")

        AsyncFunction("start") { config: Map<String, Any?> ->
            // Delegates to Java — implemented in Task 5
        }

        Function("stop") {
            // Delegates to Java — implemented in Task 5
        }

        Function("isRunning") {
            server != null
        }
    }
}
