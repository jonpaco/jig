package expo.modules.jig

import expo.modules.kotlin.modules.Module
import expo.modules.kotlin.modules.ModuleDefinition

/**
 * Expo module definition that delegates to the shared C core via JNI.
 *
 * This is the Expo-integrated path (npm install, not injection).
 * The C core's libjig.so is loaded at class init time and the native
 * methods bridge into jig_server_create / jig_server_start / etc.
 *
 * TODO: Integrate with Gradle/CMake build so libjig.so is built as part
 * of the app's native compilation. Requires android/jni/CMakeLists.txt
 * to be referenced from build.gradle's externalNativeBuild block.
 *
 * TODO: Implement nativeStart/nativeStop/nativeIsRunning JNI functions
 * in a separate C file (JigExpoModule.c) that wraps the C core API
 * with proper JNI signatures.
 */
class JigModuleCCore : Module() {
    companion object {
        init {
            System.loadLibrary("jig")
        }
    }

    // JNI native methods — implemented in C, to be wired up
    private external fun nativeStart(
        name: String,
        bundleId: String,
        platform: String,
        rnVersion: String,
        expoVersion: String?,
        port: Int,
    )
    private external fun nativeStop()
    private external fun nativeIsRunning(): Boolean

    override fun definition() = ModuleDefinition {
        Name("Jig")

        AsyncFunction("start") { config: Map<String, Any?> ->
            val rnVersion = config["rnVersion"] as? String ?: "unknown"
            val expoVersion = config["expoVersion"] as? String
            val port = (config["port"] as? Double)?.toInt() ?: 4042

            val context = appContext.reactContext
                ?: throw Exception("React context not available")
            val androidAppInfo = context.packageManager
                .getApplicationInfo(context.packageName, 0)
            val appName = context.packageManager
                .getApplicationLabel(androidAppInfo).toString()

            nativeStart(appName, context.packageName, "android", rnVersion, expoVersion, port)
        }

        Function("stop") {
            nativeStop()
        }

        Function("isRunning") {
            nativeIsRunning()
        }
    }
}
