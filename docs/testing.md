# Validation

This branch is **not yet fully qualified for merge or release**. Passing source tests and earlier device builds do not qualify a newer prepared archive. The remaining matrix below is part of the release criteria.

## Required local gate

Run the native SDK readiness command from this repository on a clean, committed feature branch:

```sh
node ../../scripts/pr-readiness.mjs run
```

Its SDK gate is `python3 scripts/check.py`. Set `UNREAL_ENGINE_ROOT` to UE 5.8, `JAVA_HOME` to Java 21, and `NUXIE_IOS_SIMULATOR_ID` to the simulator selected for this task. The gate:

- Runs the production request ledger with address/undefined-behavior sanitizers.
- Builds the plugin and Lab editor target.
- Requires successful `Nuxie.Contract.SessionLifecycle`, `Nuxie.Contract.ValuesAndReceipts`, and `Nuxie.Contract.DeferredBudget` tests, with no failed or unrun Nuxie tests.
- Builds and tests the Kotlin bridge.
- Builds and tests the Swift bridge on the selected simulator.

Missing tools fail the gate. TypeScript checks do not apply to this native SDK change.

## Current fixes and focused evidence

Qualification uses UE 5.8.2 on Mac arm64, an Android API 36 arm64 emulator, and a physical iPhone 17 Pro Max. Current native pins are iOS `d18e51c6f9ead5e4616bfb24f313512107476156` and Android `b32c9fdba42cb15af48c745ac8c2b1640f4c7708`.

### Identity and restore

[UNIV-3175](https://universe.basis.dev/issue/UNIV-3175) is implemented in [iOS PR #429](https://github.com/nuxieai/nuxie-ios/pull/429) and [Android PR #111](https://github.com/nuxieai/nuxie-android/pull/111). Native activities expose their final durable customer attribution and capture-time identity-session currency. The Unreal bridges discard activities whose native identity is no longer current. Native tests cover A → B → A, reset, shutdown, delayed delivery, and final attribution changed by `beforeSend`. Both native readiness gates passed on the revisions above; final Unreal integration qualification remains separate.

[UNIV-3181](https://universe.basis.dev/issue/UNIV-3181) was reproduced by tapping an authored Restore control when Play Billing was unavailable. A connection exception escaped the native coroutine and killed the host. Android now returns and emits the correlated restore failure while preserving cancellation. The connection-failure regression failed before the fix; all 80 `PurchaseServiceTest` tests passed afterwards. The complete Android readiness gate then passed: `git diff --check origin/main...HEAD` and `./gradlew :nuxie-android:test :nuxie-android:apiCheck :nuxie-android:lint :example-app:assembleDebug`.

### Android callbacks during native presentation

[UNIV-3180](https://universe.basis.dev/issue/UNIV-3180) required more than gameplay pause support: covering GameActivity with a native Experience also suspends Unreal's dispatcher. The plugin now enables `EnableNewBackgroundBehavior`, retains a scoped engine wake while dispatch work remains, and acquires/releases that wake only on Unreal's Android lifecycle event thread. A nonblocking event counter coalesces notifications; Android-main-thread renewal recovers a wake superseded by suspension. Local deferred callbacks share a bounded queue and cannot exceed 256 deliveries per engine frame.

Focused review identified and corrected deferred callbacks stranded behind overlays, same-iteration recursive draining, and activation/suspension races. The final focused source review found no remaining actionable defects. The 1,200-callback Unreal regression passed, along with session lifecycle and value/receipt tests. Kotlin renewal coverage verifies a single notification continues renewing independently, duplicate requests coalesce, idle acknowledgement stops retries, and a new request restarts them.

The rebuilt Android Lab was manually exercised on 2026-09-15 UTC. Its APK SHA-256 was `547de545b09cb70bb2b4eb98179f29ceb91f07ce47bf82ea74782aba39637c3a`:

| Check | Observed result |
| --- | --- |
| Published native Experience | Rendered the authored controls against the local backend |
| Typed App Action | Three typed payload values delivered while `paused=true, presenting=true` |
| Locally rejected SDK call from App Action | Failure callback delivered at 15:46:42.673 while the Experience remained open |
| Background and foreground | Home Screen at 15:47:16; app reopened at 15:47:34 with the Experience retained |
| App Action after foregrounding | Action and local failure callback delivered at 15:47:55.935 with the overlay open |
| Unavailable Play Billing restore | Authored `restore_failed` route completed without a process crash |
| Journey completion | At 15:48:10.073, both `paused` and `presenting` became false |
| SDK shutdown | Completed at 15:48:17.332; a subsequent query returned the unconfigured error |

The Lab releases only its own pause, using the matching Experience/version/Journey key. Completion is handled as well as explicit dismissal because a terminal Journey can retire its screen without a user-dismissal activity. Manual map travel after this correction retained `paused=false, presenting=false` in the preceding dedicated pause build. Preservation of a pre-existing game-owned pause and travel while an Experience remains open still require qualification.

For the active-overlay local-rejection check, set `validateOverlayDispatch: true` in the development Lab's `Saved/NuxieLab/auto.json`. This intentionally calls `CheckFeature` with an empty Feature ID from an App Action and records the expected deferred failure. It is excluded from Shipping builds.

## Earlier evidence: useful, not final-candidate qualification

Earlier Lab commit `6b29e26` passed local-backend lifecycle and metered consumption on both platforms: configure/identify, Ready, two entity queries, accepted debit, exact same-ID replay, one saved action, unchanged comparison entity, denied over-balance consumption and replay, locale override/reset, anonymous rotation, reidentification, shutdown/reconfigure, and identity across map travel. Both lifecycle reports ended on `LabSecond`.

Earlier physical iOS runs rendered the published Experience and exercised authored Close and typed App Action controls, releasing pause on dismissal. Earlier Android APKs passed ZIP 16 KiB alignment and all nine arm64 libraries passed ELF load-segment checks. Earlier iOS apps passed strict recursive codesign verification. These results predate the current dispatcher/native pins.

Prepared archive `prepared-all-v6` previously built for IOS, Android, and Mac. Earlier Blueprint-only consumers compiled their real graphs; Android ran the real async configuration-error branch, and an iOS consumer installed/launched on the phone. They must be rebuilt from the final archive. An unconfigured error branch proves module startup, not backend success.

The installed Epic distribution lacks iOS-simulator third-party link inputs, beginning with PLCrashReporter. Swift bridge simulator tests are supported; a full Unreal simulator player is not claimed as passing.

## Remaining qualification

- Final iOS Lab manual/lifecycle/Experience/App Action/dismissal checks after the current changes.
- Both platforms: preservation of a game-owned pause, map travel while presenting, and external billing success/cancel/pending/failure/restore/no-purchases, duplicate/wrong completion, timeout, teardown, and selected offer fidelity.
- Real App Store and Google Play sandbox purchase and restore against the backend. Existing apps are Apple Nuxie Staging (`ai.nuxie.ios.staging`) and Play Nuxie Staging (`ai.nuxie.example`); Play product `nuxie_qualification` / base plan `monthly` is active. Locating these settings, a synthetic Purchased completion, and the unavailable-billing failure check are not store purchase evidence.
- Final Release native archives, prepared plugin build, and fresh Blueprint-only iOS/Android consumers using the committed explicit identifiers.
- Final package signing/alignment checks, review, committed-candidate readiness receipt, and accurate PR evidence.

Use a disposable local app/customer with finite grants for backend checks. The Lab writes `Saved/NuxieLab/validation.json`, checks two distinct entities, persists pending operation IDs and gameplay application, and assumes no concurrent consumers. Distinguish request acceptance, presentation, and verified store outcomes.

For the unconfigured Blueprint-only Android startup check:

```sh
python3 scripts/check-android-blueprint.py --serial <adb-device-id> --apk Examples/BlueprintOnly/Binaries/Android/BlueprintOnly-arm64.apk
```

This installs and launches the consumer, rejects native-module startup failures, and requires its real Blueprint async error branch within 60 seconds. It does not clear data or simulate backend success.
