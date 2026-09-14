# iOS bridge

Swift directly imports the exact SDK revision in `NATIVE-PINS.json`. C exports provide contract version, request dispatch, message polling, and owned-string release. All native operations enter the main actor; Unreal receives asynchronous replies through the game-thread dispatcher.

`ThirdParty/IOS/scripts/build-framework.sh` prepares separate device and arm64 simulator framework archives and includes `Nuxie_Nuxie.bundle`. The module rule selects the simulator artifact for `UnrealArch.IOSSimulator`. Build and test through Xcode, not a host `swift build`.

The debug-only endpoint override is `NUXIE_UNREAL_API_ENDPOINT`. Release builds retain native environment configuration. Packaged qualification must verify framework embedding, native runtime symbols, resources, Experience rendering, and warm-start behavior.
