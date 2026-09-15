import Foundation

private let outbox = NuxieOutbox()
private var bridge: NuxieUnrealRuntime?

@discardableResult private func deliver(_ value: String) -> Bool { outbox.offer(value) }

@_cdecl("NuxieUnreal_Dispatch")
public func nuxieUnrealDispatch(_ pointer: UnsafePointer<CChar>) -> Int32 {
  let json = String(cString: pointer)
  guard let data = json.data(using: .utf8),
      let request = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
      let id = request["requestId"] as? String,
      let method = request["method"] as? String,
      let arguments = request["arguments"] as? [String: Any], outbox.admit(id, method: method) else { return 0 }
  DispatchQueue.main.async {
    if bridge == nil {
      let next = NuxieUnrealRuntime()
      next.emit = { deliver($0) }
      bridge = next
    }
    func reply(_ value: [String: Any]) {
      if let bytes = try? JSONSerialization.data(withJSONObject: value) { deliver(String(decoding: bytes, as: UTF8.self)) }
    }
    bridge!.invoke(method, arguments: arguments,
      resolve: { reply(["requestId": id, "result": $0 ?? NSNull()]) },
      reject: { code, message, _ in reply(["requestId": id, "error": ["code": code, "message": message]]) })
  }
  return 1
}

@_cdecl("NuxieUnreal_PopMessage")
public func nuxieUnrealPopMessage() -> UnsafeMutablePointer<CChar>? {
  guard let value = outbox.poll() else { return nil }
  return strdup(value)
}

@_cdecl("NuxieUnreal_FreeCString")
public func nuxieUnrealFreeCString(_ pointer: UnsafeMutablePointer<CChar>?) { free(pointer) }

@_cdecl("NuxieUnreal_Detach")
public func nuxieUnrealDetach() {
  DispatchQueue.main.async {
    bridge?.invalidate(); bridge = nil
    outbox.clear()
  }
}

@_cdecl("NuxieUnreal_ContractVersion")
public func nuxieUnrealContractVersion() -> Int32 { 1 }
