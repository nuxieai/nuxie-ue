package ai.nuxie.unreal

import android.os.Handler
import android.os.Looper
import java.util.concurrent.TimeUnit
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.Shadows.shadowOf
import org.robolectric.annotation.Config
import org.robolectric.annotation.LooperMode

@RunWith(RobolectricTestRunner::class)
@Config(sdk = [35], manifest = Config.NONE)
@LooperMode(LooperMode.Mode.PAUSED)
class DispatchWakeTest {
  @Test fun aSingleRequestRenewsUntilAcknowledgedThenStopsAndCanRestart() {
    val looper = shadowOf(Looper.getMainLooper())
    var pending = true
    var attempts = 0
    val wake = NuxieDispatchWake(Handler(Looper.getMainLooper())) { ++attempts; pending }
    repeat(100) { wake.request() }
    looper.idle()
    assertEquals(1, attempts)
    // No additional producer notifications: suspension may have consumed the
    // original activation, so the platform thread must independently retry.
    looper.idleFor(48, TimeUnit.MILLISECONDS)
    assertEquals(4, attempts)
    pending = false
    looper.idleFor(16, TimeUnit.MILLISECONDS)
    assertEquals(5, attempts)
    looper.idleFor(1000, TimeUnit.MILLISECONDS)
    assertEquals(5, attempts)
    pending = true
    wake.request()
    looper.idle()
    assertEquals(6, attempts)
    pending = false
    looper.idleFor(16, TimeUnit.MILLISECONDS)
  }
}
