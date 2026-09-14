// swift-tools-version: 6.0
import PackageDescription
import Foundation

let pinsURL = URL(fileURLWithPath: #filePath).deletingLastPathComponent().deletingLastPathComponent().deletingLastPathComponent().appendingPathComponent("NATIVE-PINS.json")
let pins = try JSONSerialization.jsonObject(with: Data(contentsOf: pinsURL)) as! [String: Any]
let ios = pins["ios"] as! [String: String]

let package = Package(
  name: "NuxieUnrealBridge",
  platforms: [
    .iOS(.v17),
  ],
  products: [
    .library(
      name: "NuxieUnrealBridge",
      type: .dynamic,
      targets: ["NuxieUnrealBridge"]
    )
  ],
  dependencies: [
    .package(
      url: ios["repository"]!,
      revision: ios["revision"]!
    )
  ],
  targets: [
    .target(
      name: "NuxieUnrealBridge",
      dependencies: [
        .product(name: "Nuxie", package: "nuxie-ios"),
      ]
    ),
    .testTarget(
      name: "NuxieUnrealBridgeTests",
      dependencies: ["NuxieUnrealBridge"]
    )
  ],
  swiftLanguageModes: [.v5]
)
