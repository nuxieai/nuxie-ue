# Validation

The local gate is `python3 scripts/check.py`. It runs the production portable request ledger with address/undefined-behavior sanitizers, compiles the UE 5.8 Lab and plugin, executes `Nuxie.*` Unreal automation, builds/tests the Kotlin bridge, and runs the Swift bridge tests on an explicitly selected iOS simulator. Missing tooling fails the gate; it is not a skip.

## Evidence so far

- UE 5.8.2 Mac arm64: plugin and Lab editor builds passed.
- After Epic's iOS/Android optional components were installed, the Lab's Android arm64 native build and Gradle debug APK packaging passed. `BuildCookRun -platform=Android -cookflavor=ASTC -cook -stage -pak -package -map=Lab+LabSecond` then passed. The resulting 159 MB APK installed on the Android API 36 arm64 emulator and reached the Lab’s initial Unknown feature state without a fatal startup error. The packaged Development Lab then passed the real local SDK ingest backend workflow twice, including a retained-cache process restart: configure, identify, Ready snapshot, two entity queries, one debit, same-ID replay, one saved action, and unchanged comparison entity. Both APK ZIP alignment and every arm64 ELF load-segment alignment passed the 16 KiB checks.
- iOS arm64 C++/Objective-C++ compilation and linking passed. Both unsigned and automatically provisioned, development-signed Xcode PostBuildSync builds passed. The resulting app contains NuxieUnrealBridge.framework, its native resource bundle/privacy manifest, and the executable’s matching framework load path. The normal Unreal Build.sh path also passed with the local modern-Xcode signing settings, and codesign verification passed. Repeated iOS content cooks encountered Zen insufficient storage (HTTP 507), including after reclaiming this task’s redundant outputs; device launch remains unrun: the paired iPhones are currently unavailable to Xcode.
- iOS simulator source compilation passed, but linking failed because the installed Epic distribution lacks simulator third-party libraries, beginning with PLCrashReporter. This is not an iOS simulator qualification pass.
- Unreal automation: two contract tests passed: session lifecycle and value/receipt behavior, including accepted consumption at zero balance, exact generation/revision handling, full signed native integer bounds, and optional store metadata.
- Portable C++ ledger: timeout, duplicate settlement, identity fencing, and exact numeric limits passed with sanitizers.
- Kotlin native bridge: debug/release compilation and three checkout lifecycle tests passed.
- Swift native bridge: simulator build and three checkout/session tests passed.
- iOS device and simulator framework archives were rebuilt with lossless native scalar mapping.
- Lab maps and a real Blueprint graph were generated and compiled by UE 5.8.2.
- `python3 scripts/package.py --output dist/prepared-all-v1` built the prepared plugin for IOS, Android, and Mac. A fresh copy loaded in BlueprintOnly; the real `BP_NuxieGettingStarted` graph and the CompileAllBlueprints commandlet completed with zero errors/warnings. Blueprint-only mobile cooking remains unrun.
- The Lab UI was visually inspected in standalone editor play; keyboard configuration produced the expected UnsupportedPlatform result on Mac.
- The game-instance-owned external billing harness compiles with retained typed requests and cancellation/failure controls; live checkout invocation remains unrun.
- A disposable local backend workspace/app and authored Experience were created, published, and drained to a completed build. Android commerce validation used real canonical releases and customer grants; the temporary local seed hook was removed afterwards. Native Experience interaction remains unrun.

This is work-in-progress evidence, not release qualification. The following remain required: a cooked iOS Lab run, broader live lifecycle validation, visual Experience/App Action interaction, retained-cache warm start, pause/background/map travel, external checkout sandbox results, and a fresh Blueprint-only consumer packaged from the distribution archive. Update this record with actual results, commands, and artifact identity as each completes.

## Live matrix

Use a disposable local development app and customer with a finite metered grant. Verify configure and identity, Unknown → Ready, two entity queries, accepted original consumption, same-ID replay, exactly one debit, unchanged second entity, denied consumption, anonymous rotation, reidentification, locale override/reset, trigger and rendered Experience, authored app action, dismissal, warm launch, pause, background/resume, and map travel.

The Lab's balance/replay validator checks two different finite entity balances and writes `Saved/NuxieLab/validation.json`. It assumes neither entity has concurrent consumers. Controls wait for admitted consumption/validation to settle so a second click cannot replace the pending saved operation. Saving the pending operation and gameplay application is part of the example. Logs must distinguish operation acceptance from Experience presentation and from store verification.

For external checkout verify success, cancellation, pending, failure, restore, no purchases, wrong/duplicate completion, timeout, teardown, and selected offer fidelity. A manually reported Purchased value is not store sandbox evidence.
