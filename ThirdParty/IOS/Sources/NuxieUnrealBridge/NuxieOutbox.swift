import Foundation

/// Bounded observational traffic with separately reserved reply and checkout capacity.
final class NuxieOutbox {
  private let lock = NSLock()
  private var admitted: [String: String] = [:]
  private var replies: [String: String] = [:]
  private var replyOrder: [String] = []
  private var checkout: [String] = []
  private var events: [String] = []
  private var features: String?
  private var overflow: String?

  func admit(_ id: String, method: String = "query") -> Bool {
    lock.withLock {
      let capacity: Int
      switch method {
      case "configure", "shutdown", "identify", "reset": capacity = 132
      case "completePurchase", "completeRestore": capacity = 128
      default: capacity = 64
      }
      guard admitted.count < capacity, admitted[id] == nil else { return false }
      admitted[id] = method; return true
    }
  }

  @discardableResult func offer(_ message: String) -> Bool {
    guard let data = message.data(using: .utf8),
      let envelope = try? JSONSerialization.jsonObject(with: data) as? [String: Any] else { return false }
    return lock.withLock {
      if let id = envelope["requestId"] as? String {
        guard admitted[id] != nil, replies[id] == nil else { return false }
        if admitted[id] == "shutdown", envelope["error"] == nil {
          admitted = [id: "shutdown"]; replies.removeAll(); replyOrder.removeAll()
          checkout.removeAll(); events.removeAll(); features = nil; overflow = nil
        }
        replies[id] = message; replyOrder.append(id)
      } else {
        switch envelope["name"] as? String {
        case "purchase", "restore":
          guard checkout.count < 64 else { return false }
          checkout.append(message)
        case "features": features = message
        default:
          if events.count >= 256 {
            events.removeFirst()
            var notice = envelope
            notice["name"] = "overflow"; notice["payload"] = [String: Any]()
            if let bytes = try? JSONSerialization.data(withJSONObject: notice) {
              overflow = String(decoding: bytes, as: UTF8.self)
            }
          }
          events.append(message)
        }
      }
      return true
    }
  }

  func poll() -> String? {
    lock.withLock {
      if !replyOrder.isEmpty {
        let id = replyOrder.removeFirst(); admitted.removeValue(forKey: id)
        return replies.removeValue(forKey: id)
      }
      if !checkout.isEmpty { return checkout.removeFirst() }
      if let value = overflow { overflow = nil; return value }
      if let value = features { features = nil; return value }
      return events.isEmpty ? nil : events.removeFirst()
    }
  }

  func clear() {
    lock.withLock {
      admitted.removeAll(); replies.removeAll(); replyOrder.removeAll()
      checkout.removeAll(); events.removeAll(); features = nil; overflow = nil
    }
  }
}
