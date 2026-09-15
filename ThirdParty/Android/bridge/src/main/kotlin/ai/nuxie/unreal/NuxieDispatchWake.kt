package ai.nuxie.unreal

import android.os.Handler

/** Renew a lost engine wake until its game-thread drain acknowledges idle. */
internal class NuxieDispatchWake(private val main: Handler, private val renew: () -> Boolean) {
  // Owned exclusively by the Android main looper, including request admission.
  private var scheduled = false
  private val retry = object : Runnable {
    override fun run() {
      if (renew()) main.postDelayed(this, 16)
      else scheduled = false
    }
  }

  fun request() {
    main.post {
      if (!scheduled) {
        scheduled = true
        main.post(retry)
      }
    }
  }
}
