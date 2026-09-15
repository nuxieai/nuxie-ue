package ai.nuxie.unreal

import ai.nuxie.sdk.billing.RestoreResult
import kotlinx.coroutines.runBlocking
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(sdk = [35], manifest = Config.NONE)
class OutboxTest {
  private fun event(name: String, sequence: Int) =
    """{"session":"game","identityGeneration":"2","name":"$name","payload":{"sequence":$sequence}}"""

  @Test fun aBoundedDrainReportsRemainingWorkUntilEveryQueueIsEmpty() {
    val box = NuxieOutbox()
    assertFalse(box.hasMessages())
    repeat(256) { box.offer(event("activity", it)) }
    box.offer(event("features", 0))
    box.offer(event("restore", 0))
    assertTrue(box.admit("reply"))
    box.offer("""{"requestId":"reply","result":null}""")
    repeat(256) { assertNotNull(box.poll()) }
    assertTrue(box.hasMessages())
    repeat(3) { assertNotNull(box.poll()) }
    assertFalse(box.hasMessages())
    // An arrival after the final drain must request another wake.
    box.offer(event("purchase", 1))
    assertTrue(box.hasMessages())
    assertEquals(event("purchase", 1), box.poll())
    assertFalse(box.hasMessages())
  }

  @Test fun burstsCannotStarveRepliesOrCheckoutAndStorageIsBounded() {
    val box = NuxieOutbox()
    repeat(10000) { assertTrue(box.offer(event("activity", it))); assertTrue(box.offer(event("features", it))) }
    assertTrue(box.admit("reply"))
    val reply = """{"requestId":"reply","result":null}"""
    assertTrue(box.offer(reply)); assertFalse(box.offer(reply))
    assertTrue(box.offer(event("restore", 0)))
    assertEquals(reply, box.poll()); assertEquals(event("restore", 0), box.poll())
    assertTrue(box.poll()!!.contains("overflow"))
    assertEquals(event("features", 9999), box.poll())
    for (i in 9744 until 10000) assertEquals(event("activity", i), box.poll())
    assertNull(box.poll())
  }
  @Test fun reservedCapacityRejectsAndRecoversWithoutDroppingAdmittedReplies() {
    val box = NuxieOutbox()
    repeat(64) { assertTrue(box.admit(it.toString())); assertTrue(box.offer(event("purchase", it))) }
    assertFalse(box.admit("overflow")); assertFalse(box.offer(event("restore", 65)))
    assertTrue(box.offer("""{"requestId":"0","result":null}"""))
    assertNotNull(box.poll()); assertTrue(box.admit("replacement"))
    assertEquals(event("purchase", 0), box.poll()); assertTrue(box.offer(event("restore", 65)))
  }
  @Test fun shutdownAndCheckoutHaveReservedAdmissionAndShutdownRetiresStuckRequests() {
    val box = NuxieOutbox()
    repeat(64) { assertTrue(box.admit(it.toString())) }
    assertFalse(box.admit("ordinary"))
    assertTrue(box.admit("checkout", "completeRestore")); assertTrue(box.admit("shutdown", "shutdown"))
    assertTrue(box.offer("""{"requestId":"shutdown","result":null}""")); assertNotNull(box.poll())
    assertFalse(box.offer("""{"requestId":"0","result":null}"""))
    repeat(64) { assertTrue(box.admit("replacement-$it")) }
  }
  @Test fun rejectedCheckoutSettlesImmediately() = runBlocking {
    val bridge = NuxiePurchaseDelegateBridge({ _, _ -> false }, timeoutMs = 30000)
    assertTrue(bridge.restorePurchases() is RestoreResult.Failed)
  }
}
