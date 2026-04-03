package expo.modules.jig

import expo.modules.kotlin.modules.Module
import expo.modules.kotlin.modules.ModuleDefinition

class JigModule : Module() {
    private var server: JigWebSocketServer? = null

    override fun definition() = ModuleDefinition {
        Name("Jig")

        AsyncFunction("start") { config: Map<String, Any?> ->
            if (server != null) return@AsyncFunction

            val rnVersion = config["rnVersion"] as? String ?: "unknown"
            val expoVersion = config["expoVersion"] as? String
            val port = (config["port"] as? Double)?.toInt() ?: 4042

            val context = appContext.reactContext
                ?: throw Exception("React context not available")
            val appInfo = context.packageManager.getApplicationInfo(context.packageName, 0)
            val appName = context.packageManager.getApplicationLabel(appInfo).toString()

            val handler = JigHandshakeHandler(appName, context.packageName, rnVersion, expoVersion)
            val newServer = JigWebSocketServer(port, handler)
            newServer.start()
            server = newServer
        }

        Function("stop") {
            server?.let { srv ->
                Thread {
                    try { srv.stop(1000) } catch (_: InterruptedException) {}
                }.start()
            }
            server = null
        }

        Function("isRunning") {
            server != null
        }
    }
}
