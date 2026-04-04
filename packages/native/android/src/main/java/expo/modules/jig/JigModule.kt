package expo.modules.jig

import expo.modules.jig.handlers.HandshakeHandler
import expo.modules.jig.handlers.ScreenshotHandler
import expo.modules.jig.middleware.DomainGuard
import expo.modules.jig.middleware.HandshakeGate
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
            val androidAppInfo = context.packageManager
                .getApplicationInfo(context.packageName, 0)
            val appName = context.packageManager
                .getApplicationLabel(androidAppInfo).toString()

            val appInfo = JigAppInfo(
                appName, context.packageName, "android",
                rnVersion, expoVersion
            )

            val supportedDomains = emptyList<String>()
            val middlewares = listOf<JigMiddleware>(
                HandshakeGate(),
                DomainGuard(supportedDomains),
            )
            val handlers = listOf<JigHandler>(
                HandshakeHandler(),
                ScreenshotHandler { appContext.currentActivity },
            )
            val dispatcher = JigDispatcher(
                middlewares, handlers, supportedDomains
            )

            val newServer = JigWebSocketServer(port, dispatcher, appInfo)
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
