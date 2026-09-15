import XCTest
@_spi(Testing) import Nuxie
@testable import NuxieUnrealBridge

final class BridgeMappingTests: XCTestCase {
  func testHostEndpointWorksIndependentlyOfNativeBuildConfiguration() throws {
    let config = NuxieConfiguration(apiKey: "test-key")
    try applyHostTestingEndpoint(["environment": "development", "testingApiEndpoint": "http://127.0.0.1:21384"], to: config)
    XCTAssertEqual(config.testingOverrides.apiEndpoint, URL(string: "http://127.0.0.1:21384"))
  }

  func testHostEndpointRejectsProductionAndInvalidURLs() {
    for input: [String: Any] in [
      ["environment": "production", "testingApiEndpoint": "https://localhost"],
      ["environment": "development", "testingApiEndpoint": "file:///tmp/endpoint"],
      ["environment": "development", "testingApiEndpoint": "relative/path"],
      ["environment": "development", "testingApiEndpoint": "https://user:password@localhost"],
      ["environment": "development", "testingApiEndpoint": 123]
    ] {
      let config = NuxieConfiguration(apiKey: "test-key")
      XCTAssertThrowsError(try applyHostTestingEndpoint(input, to: config))
      XCTAssertNil(config.testingOverrides.apiEndpoint)
    }
  }

  func testMissingHostEndpointPreservesNativeConfiguration() throws {
    let config = NuxieConfiguration(apiKey: "test-key")
    try applyHostTestingEndpoint(["environment": "production"], to: config)
    XCTAssertNil(config.testingOverrides.apiEndpoint)
  }

  func testRestoreHasBoundedSettlement() async {
    let bridge = NuxiePurchaseDelegateBridge(timeoutSeconds: 0.01) { _, _ in true }
    let result = await bridge.restorePurchases()
    guard case .failed = result else { return XCTFail("Missing controller reply must time out") }
  }

  func testRestoreCorrelationAndDuplicateCompletion() async {
    var bridge: NuxiePurchaseDelegateBridge!
    bridge = NuxiePurchaseDelegateBridge { name, payload in
      XCTAssertEqual(name, "restore")
      let id = payload["requestId"] as! String
      bridge.completeRestore(requestId: "wrong-id", payload: ["type": "failed"])
      bridge.completeRestore(requestId: id, payload: ["type": "restored"])
      bridge.completeRestore(requestId: id, payload: ["type": "failed"])
      return true
    }
    let result = await bridge.restorePurchases()
    guard case .restored = result else { return XCTFail("Matching first result wins") }
  }
}
