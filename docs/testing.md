# Validation

The local gate is `python3 scripts/check.py`. It runs the production portable request ledger with address/undefined-behavior sanitizers, compiles the UE 5.8 Lab and plugin, executes `Nuxie.*` Unreal automation, builds/tests the Kotlin bridge, and runs the Swift bridge tests on an explicitly selected iOS simulator. Missing tooling fails the gate; it is not a skip.

## Evidence so far

- UE 5.8.2 Mac arm64: plugin and Lab editor builds passed.
- After Epic's iOS/Android optional components were installed, the Lab's Android arm64 native build and Gradle debug APK packaging passed. `BuildCookRun -platform=Android -cookflavor=ASTC -cook -stage -pak -package -map=Lab+LabSecond` then passed. The resulting 159 MB APK installed on the Android API 36 arm64 emulator and reached the Lab’s initial Unknown feature state without a fatal startup error. The packaged Development Lab then passed the real local SDK ingest backend workflow twice, including a retained-cache process restart: configure, identify, Ready snapshot, two entity queries, one debit, same-ID replay, one saved action, and unchanged comparison entity. Both APK ZIP alignment and every arm64 ELF load-segment alignment passed the 16 KiB checks.
- iOS arm64 C++/Objective-C++ compilation and linking passed. Both unsigned and automatically provisioned, development-signed Xcode PostBuildSync builds passed. The resulting app contains NuxieUnrealBridge.framework, its native resource bundle/privacy manifest, and the executable’s matching framework load path. The normal Unreal Build.sh path also passed with the local modern-Xcode signing settings, and codesign verification passed. After freeing disk space, `BuildCookRun -platform=IOS -clientconfig=Development -build -cook -stage -pak -package -map=Lab+LabSecond -AdditionalCookerOptions=-nowrite` passed in 8m20s, producing a self-contained 449 MB development-signed app. Strict recursive codesign verification passed on that final app. The development Lab subsequently installed and ran on a physical iPhone 17 Pro Max. A Debug Swift bridge was required for the local endpoint override; the normal Release bridge intentionally ignores it. After allowing local-network access, the real ingest backend workflow passed twice, including a retained-cache process restart: configure, identify, entity queries, one accepted debit, exact same-ID replay, one saved action, and an unchanged comparison entity. The examples now set the modern Xcode bundle identifier explicitly as well as the mobile setting.
- iOS simulator source compilation passed, but linking failed because the installed Epic distribution lacks simulator third-party libraries, beginning with PLCrashReporter. This is not an iOS simulator qualification pass.
- Unreal automation: two contract tests passed: session lifecycle and value/receipt behavior, including accepted consumption at zero balance, exact generation/revision handling, full signed native integer bounds, and optional store metadata.
- Portable C++ ledger: timeout, duplicate settlement, identity fencing, and exact numeric limits passed with sanitizers.
- Kotlin native bridge: debug/release compilation and three checkout lifecycle tests passed.
- Swift native bridge: simulator build and three checkout/session tests passed.
- iOS device and simulator framework archives were rebuilt with lossless native scalar mapping. After physical-device validation, both archives were restored to Release. Distribution packaging correctly rejected a Debug receipt, then `python3 scripts/package.py --output dist/prepared-all-v4` passed for IOS+Android+Mac with Release artifacts.
- Lab maps and a real Blueprint graph were generated and compiled by UE 5.8.2.
- `python3 scripts/package.py --output dist/prepared-all-v3` built the prepared plugin for IOS, Android, and Mac. A fresh copy loaded in BlueprintOnly; the real `BP_NuxieGettingStarted` graph and the CompileAllBlueprints commandlet completed with zero errors/warnings. The first Android consumer package exposed a missing-module launch failure: the descriptor omitted `EnabledByDefault`, so Unreal skipped its temporary native target. Explicit opt-in fixes target generation; `scripts/package.py` restores it after Unreal BuildPlugin clears the field. The smoke test reproduced the missing-module failure on the uncorrected archive, then passed using a fresh copy of the corrected archive. Android build/cook/package, install, and execution of the real Blueprint async configuration-error branch now pass. This unconfigured consumer check is not backend evidence. The same fresh archive passed iOS build/cook/stage/package in 1m20s; the final 447 MB app passes strict recursive codesign verification and its executable links NuxieUnrealBridge.framework. Physical iOS launch remains unrun.
- After the macOS Screen Time restriction was removed, the Lab UI was visually inspected again in standalone editor play; keyboard configuration produced the expected UnsupportedPlatform result on Mac.
- The game-instance-owned external billing harness compiles with retained typed requests and cancellation/failure controls; live checkout invocation remains unrun.
- A disposable local backend workspace/app and authored Experience were created, published, and drained to a completed build. Android commerce validation used real canonical releases and customer grants; the temporary local seed hook was removed afterwards. Through iPhone Mirroring, the physical iPhone displayed the published native Experience with its authored “Experience smoke is live” text. Backgrounding to the Home Screen and reactivating the Lab preserved the rendered Experience. This fixture is text-only, so it does not validate an app-action control or authored dismissal.

This is work-in-progress evidence, not release qualification. The following remain required: broader live lifecycle validation, App Action and dismissal interaction, pause ownership/map travel, and external checkout sandbox results. Update this record with actual results, commands, and artifact identity as each completes.

## Live matrix

Use a disposable local development app and customer with a finite metered grant. Verify configure and identity, Unknown → Ready, two entity queries, accepted original consumption, same-ID replay, exactly one debit, unchanged second entity, denied consumption, anonymous rotation, reidentification, locale override/reset, trigger and rendered Experience, authored app action, dismissal, warm launch, pause, background/resume, and map travel.

The Lab's balance/replay validator checks two different finite entity balances and writes `Saved/NuxieLab/validation.json`. It assumes neither entity has concurrent consumers. Controls wait for admitted consumption/validation to settle so a second click cannot replace the pending saved operation. Saving the pending operation and gameplay application is part of the example. Logs must distinguish operation acceptance from Experience presentation and from store verification.

For external checkout verify success, cancellation, pending, failure, restore, no purchases, wrong/duplicate completion, timeout, teardown, and selected offer fidelity. A manually reported Purchased value is not store sandbox evidence.

## Blueprint-only Android startup regression

After copying the prepared plugin and packaging `Examples/BlueprintOnly` for Android, run:

```sh
python3 scripts/check-android-blueprint.py --serial <adb-device-id> --apk Examples/BlueprintOnly/Binaries/Android/BlueprintOnly-arm64.apk
```

Use the unconfigured example. This installs and launches it, rejects native-module startup failures, and requires the real Blueprint async error branch within 60 seconds. It does not clear app data or simulate backend success.
