package ai.nuxie.unreal

import android.app.Activity
import android.os.Handler
import android.os.Looper
import java.util.concurrent.ConcurrentLinkedQueue
import org.json.JSONObject

/** JNI admission never waits for the Android UI thread or a native operation. */
object NuxieUnrealBridge {
  private val main = Handler(Looper.getMainLooper())
  private val messages = ConcurrentLinkedQueue<String>()
  private var runtime: NuxieUnrealRuntime? = null
  @JvmStatic fun contractVersion(): Int = 1
  @JvmStatic fun dispatch(activity: Activity, request: String): Boolean {
    val id = try { JSONObject(request).getString("requestId") } catch (_: Exception) { return false }
    return main.post {
      try {
        val active = runtime ?: NuxieUnrealRuntime(activity, object : NuxieUnrealRuntime.Callback {
          override fun onMessage(message: String) { messages.add(message) }
        }).also { runtime = it }
        active.attachActivity(activity)
        active.dispatch(request)
      } catch (error: Exception) {
        messages.add(JSONObject(mapOf("requestId" to id, "error" to mapOf(
          "code" to "nativeError", "message" to (error.message ?: "Native admission failed")))).toString())
      }
    }
  }
  @JvmStatic fun popMessage(): String? = messages.poll()
}
