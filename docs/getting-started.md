# Build from source

Requirements: UE 5.8.2 with Mac editor plus iOS and Android optional components; Xcode with an iOS simulator; Android SDK and NDK matching Unreal's requirements; JDK 21; Python 3; a C++20 compiler. UE 5.8 mobile defaults target Android SDK 35 and iOS 17 or newer. Unreal's platform validation remains authoritative for installed tool versions.

Set `UNREAL_ENGINE_ROOT`, `JAVA_HOME`, `ANDROID_HOME`, and `NUXIE_IOS_SIMULATOR_ID` in your shell. Do not commit machine paths. The default engine path in the local check script is Epic's standard Mac installation directory.

From the SDK root:

```sh
python3 scripts/prepare-android.py
bash ThirdParty/IOS/scripts/build-framework.sh
python3 scripts/link-example.py
python3 scripts/check.py
```

Android preparation fetches the exact pinned SDK into `.native/android`, publishes it to a private build-local Maven directory, compiles/tests the bridge, and prepares the Maven closure and AAR. It refuses to overwrite modified native input source. iOS preparation builds separate device and arm64 simulator framework archives and includes the native resource bundle.

Open `Examples/NuxieLab/NuxieLab.uproject`. The checked-in maps and Blueprint are actual Unreal assets. The explicit `NuxieCreateExamples` commandlet creates them in a fresh example project and refuses to overwrite existing teaching graphs.

Create a distribution using the bounded staging script, which verifies native input/artifact hashes and avoids recursing into example plugin links:

```sh
python3 scripts/package.py --platforms IOS+Android+Mac --output dist/prepared-mobile
```

The output directory must not already exist. The script runs Unreal's `BuildPlugin`, includes `Config/FilterPlugin.ini` resources, and writes `ARTIFACT-MANIFEST.json` with engine version, native pins, target platforms, and file hashes. `--platforms Mac` validates editor installation only. Copy the prepared output into `Examples/BlueprintOnly/Plugins/Nuxie`, then open that project. It has real Blueprint assets and no gameplay C++ module. Mobile cook/package and device runs remain separate qualification steps; an editor archive does not prove them.

The example's source link is only for contributors. A consumer installs a copied prepared plugin into `Plugins/Nuxie`; it must not depend on your SDK checkout, local Maven home, or another project's compiled module.

For local native development, obtain the backend origin from `pnpm run dev:print` in the parent checkout. iOS debug processes accept `NUXIE_UNREAL_API_ENDPOINT`; debuggable Android apps accept that intent extra. Supply the current local origin explicitly. Android HTTP development also needs a debug-only network policy when using cleartext HTTP; release traffic must retain its normal policy.
