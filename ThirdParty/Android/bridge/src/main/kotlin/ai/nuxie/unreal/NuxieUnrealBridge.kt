package ai.nuxie.unreal

import android.app.Activity
import android.os.Handler
import android.os.Looper
import org.json.JSONObject

/** JNI admission never waits for the Android UI thread or a native operation. */
object NuxieUnrealBridge {
  private val main = Handler(Looper.getMainLooper())
  private val messages = NuxieOutbox()
  private var runtime: NuxieUnrealRuntime? = null
  @JvmStatic private external fun notifyMessages()
  @JvmStatic private external fun renewDispatchWake(): Boolean
  private val dispatchWake = NuxieDispatchWake(main) { renewDispatchWake() }
  @JvmStatic fun scheduleDispatchWake() = dispatchWake.request()
  private fun offer(message: String): Boolean {
    val accepted = messages.offer(message)
    if (accepted) notifyMessages()
    return accepted
  }
  @JvmStatic fun hasMessages(): Boolean = messages.hasMessages()
  @JvmStatic fun contractVersion(): Int = 1
  @JvmStatic fun dispatch(activity: Activity, request: String): Boolean {
    val envelope = try { JSONObject(request) } catch (_: Exception) { return false }
    val id = envelope.optString("requestId").takeIf { it.isNotEmpty() } ?: return false
    if (!messages.admit(id, envelope.optString("method"))) return false
    val accepted = main.post {
      try {
        val active = runtime ?: NuxieUnrealRuntime(activity, object : NuxieUnrealRuntime.Callback {
          override fun onMessage(message: String): Boolean = offer(message)
        }).also { runtime = it }
        active.attachActivity(activity)
        active.dispatch(request)
      } catch (error: Exception) {
        offer(JSONObject(mapOf("requestId" to id, "error" to mapOf(
          "code" to "nativeError", "message" to (error.message ?: "Native admission failed")))).toString())
      }
    }
    if (!accepted) messages.cancelAdmission(id)
    return accepted
  }
  @JvmStatic fun popMessage(): String? = messages.poll()
}
