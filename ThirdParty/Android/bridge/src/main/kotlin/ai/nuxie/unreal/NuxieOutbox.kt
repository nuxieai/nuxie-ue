package ai.nuxie.unreal

import org.json.JSONObject

/** Reserved replies and checkout requests cannot sit behind observational events. */
internal class NuxieOutbox {
  private val admitted = mutableMapOf<String, String>()
  private val replies = linkedMapOf<String, String>()
  private val checkout = ArrayDeque<String>()
  private val events = ArrayDeque<String>()
  private var features: String? = null
  private var overflow: String? = null

  @Synchronized fun admit(id: String, method: String = "query"): Boolean {
    val capacity = when (method) {
      "configure", "shutdown", "identify", "reset" -> 132
      "completePurchase", "completeRestore" -> 128
      else -> 64
    }
    if (admitted.size >= capacity || id in admitted) return false
    admitted[id] = method
    return true
  }

  @Synchronized fun cancelAdmission(id: String) { admitted.remove(id) }

  @Synchronized fun offer(message: String): Boolean {
    val envelope = JSONObject(message)
    if (envelope.has("requestId")) {
      val id = envelope.getString("requestId")
      if (id !in admitted || id in replies) return false
      if (admitted[id] == "shutdown" && !envelope.has("error")) {
        admitted.clear(); admitted[id] = "shutdown"
        replies.clear(); checkout.clear(); events.clear(); features = null; overflow = null
      }
      replies[id] = message
    } else when (envelope.optString("name")) {
      "purchase", "restore" -> {
        if (checkout.size >= 64) return false
        checkout.addLast(message)
      }
      "features" -> features = message
      else -> {
        if (events.size >= 256) {
          events.removeFirst()
          overflow = JSONObject().put("session", envelope.getString("session"))
            .put("name", "overflow").put("identityGeneration", envelope.getString("identityGeneration"))
            .put("payload", JSONObject()).toString()
        }
        events.addLast(message)
      }
    }
    return true
  }

  @Synchronized fun poll(): String? {
    replies.entries.firstOrNull()?.let { (id, value) ->
      replies.remove(id); admitted.remove(id); return value
    }
    if (checkout.isNotEmpty()) return checkout.removeFirst()
    overflow?.let { overflow = null; return it }
    features?.let { features = null; return it }
    return events.removeFirstOrNull()
  }
}
