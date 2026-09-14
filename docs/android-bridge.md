# Android bridge

The Kotlin runtime directly calls the SDK revision in `NATIVE-PINS.json`. `NuxieUnrealBridge` exports compiled static `contractVersion`, `dispatch(Activity, String)`, and `popMessage` entry points. The Unreal transport uses the application class loader and UTF-16 JNI strings.

UPL copies the bridge AAR, pinned SDK Maven files, and pin manifest into the app's Gradle project. The SDK publication supplies its transitive dependency metadata and native runtime assets. Consumer builds must not resolve the retired 0.1 SDK or depend on a developer's Maven home.

Run `python3 scripts/prepare-android.py` to prepare, and `bash scripts/test-android-bridge.sh` to execute bridge tests. Device qualification additionally inspects the merged manifest, dependency resolution, native library closure, shrinker retention, and 16 KiB alignment.
