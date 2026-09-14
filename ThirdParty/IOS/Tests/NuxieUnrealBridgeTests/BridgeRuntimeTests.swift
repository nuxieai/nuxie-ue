import XCTest
@testable import NuxieUnrealBridge

final class BridgeRuntimeTests: XCTestCase {
  func testDetachedSessionFailsInsteadOfHanging() async {
    let bridge = NuxieUnrealRuntime()
    let completed = expectation(description: "settled")
    bridge.invoke("getIdentity", arguments: ["session": "missing"], resolve: { _ in
      XCTFail("An unattached runtime must not read identity"); completed.fulfill()
    }, reject: { code, _, _ in
      XCTAssertEqual(code, "sessionExpired"); completed.fulfill()
    })
    await fulfillment(of: [completed], timeout: 5)
  }
}
