import XCTest
@testable import NuxieUnrealBridge

final class OutboxTests: XCTestCase {
  private func event(_ name: String, _ sequence: Int) -> String {
    "{\"session\":\"game\",\"identityGeneration\":\"2\",\"name\":\"\(name)\",\"payload\":{\"sequence\":\(sequence)}}"
  }
  func testBurstCannotStarveRepliesOrCheckoutAndStorageIsBounded() {
    let box = NuxieOutbox()
    for i in 0..<10000 { XCTAssertTrue(box.offer(event("activity", i))); XCTAssertTrue(box.offer(event("features", i))) }
    XCTAssertTrue(box.admit("reply"))
    let reply = "{\"requestId\":\"reply\",\"result\":null}"
    XCTAssertTrue(box.offer(reply))
    XCTAssertFalse(box.offer(reply), "Duplicate terminal replies cannot use more capacity")
    XCTAssertTrue(box.offer(event("restore", 0)))
    XCTAssertEqual(box.poll(), reply)
    XCTAssertEqual(box.poll(), event("restore", 0))
    XCTAssertTrue(box.poll()!.contains("overflow"))
    XCTAssertEqual(box.poll(), event("features", 9999))
    for i in 9744..<10000 { XCTAssertEqual(box.poll(), event("activity", i)) }
    XCTAssertNil(box.poll())
  }
  func testReservedCapacityRejectsAndRecoversWithoutDroppingAdmittedReplies() {
    let box = NuxieOutbox()
    for i in 0..<64 { XCTAssertTrue(box.admit(String(i))); XCTAssertTrue(box.offer(event("purchase", i))) }
    XCTAssertFalse(box.admit("overflow"))
    XCTAssertFalse(box.offer(event("restore", 65)))
    XCTAssertTrue(box.offer("{\"requestId\":\"0\",\"result\":null}"))
    XCTAssertNotNil(box.poll())
    XCTAssertTrue(box.admit("replacement"))
    XCTAssertEqual(box.poll(), event("purchase", 0))
    XCTAssertTrue(box.offer(event("restore", 65)))
  }
  func testShutdownAndCheckoutHaveReservedAdmissionAndShutdownRetiresStuckRequests() {
    let box = NuxieOutbox()
    for i in 0..<64 { XCTAssertTrue(box.admit(String(i))) }
    XCTAssertFalse(box.admit("ordinary"))
    XCTAssertTrue(box.admit("checkout", method: "completeRestore"))
    XCTAssertTrue(box.admit("shutdown", method: "shutdown"))
    XCTAssertTrue(box.offer("{\"requestId\":\"shutdown\",\"result\":null}"))
    XCTAssertNotNil(box.poll())
    XCTAssertFalse(box.offer("{\"requestId\":\"0\",\"result\":null}"), "Late canceled reply cannot affect replacement session")
    for i in 0..<64 { XCTAssertTrue(box.admit("replacement-\(i)")) }
  }
  func testRejectedCheckoutSettlesImmediately() async {
    let bridge = NuxiePurchaseDelegateBridge(timeoutSeconds: 30) { _, _ in false }
    let result = await bridge.restorePurchases()
    guard case .failed(let error) = result else { return XCTFail("Rejected admission must fail") }
    XCTAssertEqual(error.localizedDescription, "bridge_overloaded")
  }
}
