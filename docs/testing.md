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

The Lab releases only its own pause, using the matching Experience/version/Journey key. Completion is handled as well as explicit dismissal because a terminal Journey can retire its screen without a user-dismissal activity. Android device qualification on 2026-09-15 preserved a game-owned pause after both Journey completion and dismissal at 16:10:20.108 UTC (`paused=true, presenting=false`). A separate run travelled to `LabSecond` while presenting: identity, Ready status, presentation and pause passed at 16:10:57.273; a further App Action delivered on the destination map; dismissal at 16:11:14.005 cleared the Lab-owned pause (`paused=false, presenting=false`). The corresponding iOS checks remain outstanding.

For the active-overlay local-rejection check, set `validateOverlayDispatch: true` in the development Lab's `Saved/NuxieLab/auto.json`. This intentionally calls `CheckFeature` with an empty Feature ID from an App Action and records the expected deferred failure. It is excluded from Shipping builds.

Two additional development-only options exercise the game's presentation policy against a real published Experience:

- `gameOwnedPause: true` pauses gameplay before configuration. After dismissing the Experience, the observations must retain `paused=true` while `presenting=false`.
- `travelOnAppAction: true` makes the authored `unreal_qa_action` travel once to `LabSecond` while the Experience is open. The destination checks identity, Ready status, presentation, and pause through the public API without reconfiguring the client. After dismissal, its Lab-owned pause must clear. Use a separate run without `gameOwnedPause` for this check.

## Prepared consumer evidence

Commit `f753f7d` passed the complete local readiness gate. Its Release iOS archives and prepared plugin built for IOS, Android and Mac; all 285 manifest file hashes verified. A fresh content-only consumer copied that plugin and packaged successfully for Android and iOS. Android executed the real Blueprint unconfigured-error branch on the emulator. iOS passed strict recursive codesign verification, installed on the physical phone, and executed that branch at 16:03:42.350 UTC. These runs prove prepared-module startup and async delivery, not configured backend success. The subsequent Lab-only qualification controls do not alter the plugin runtime; the full readiness gate also passed on `fa43fb2`.

A subsequent Android consumer build configured the same prepared plugin with a local development public key and a localhost-only network exception. Its real Blueprint identity-success branch executed at 16:23:20.204 UTC. Native persisted authority matched test app `app_01m2hxd0ezjqp1jnz807hxkb86`; its release-pinned Journey retained version `ver_01m2jqapeyjmaq5m0jvgcafwjd` and artifact digests. The published Experience rendered, and tapping its authored Close returned to Unreal. The equivalent configured iOS proof remains outstanding.

The current iOS Lab (`fa43fb2`) also built, passed strict recursive signing verification, and installed. Its subsequent launch was denied because the phone was locked; no new lifecycle result is claimed.

The Android Lab APK identified above also passed v2 signature verification, ZIP 16 KiB alignment and ELF load-segment alignment/congruence checks across all nine shared libraries.

## External restore evidence

On Android, the external restore reached the retained C++ controller while the Experience covered paused gameplay at 16:27:55.220 UTC. Its 60-second native deadline produced the authored failure route and released pause at 16:28:55.280. Inspection then reported no pending restore, and a late completion was rejected.

A second restore remained pending after presentation dismissal. SDK shutdown completed at 16:30:11.944, invalidated the retained request, and rejected a subsequent completion. These checks validate external-controller delivery, expiry and teardown; they do not establish a store purchase, restored entitlement, or the other purchase/restore outcomes.

## Google Play sandbox evidence

On 2026-09-15, the current Lab and native pins were packaged for the existing Play test package `ai.nuxie.example`, version code 12. The APK SHA-256 was `4c0f2205af50a4772bdb143fe06de96d806dfe547e710e1bd761f064e31ad51f`. Installation on the API 36 Play emulator preserved existing app data and used the same signing certificate as the previous test app.

The local backend imported the active `nuxie_qualification` subscription with base plan `monthly`, published its test commerce release and a native purchase/restore Experience, and granted the boolean Feature `unreal_play_access`. The Play sheet displayed **Test card, always approves** and explicitly stated that no charge would occur.

| Check | Observed result (UTC) |
| --- | --- |
| Real Play purchase | Play response code 0; Unreal received `purchase_completed` and `unreal_play_purchased` at 17:27:52.020 |
| Backend reconciliation | Initial local worker configuration returned 503; after its existing encryption key was supplied, the SDK's retained evidence retried automatically and `/purchase` returned 200 |
| Acknowledgement and access | Evidence was synced and Play-acknowledged; Unreal returned to Ready at 17:29:54.778 and emitted `purchase_synced` at 17:29:54.826; a fresh backend profile contained `unreal_play_access` |
| Real Play restore | The same customer and subscription produced `restore_completed` and `unreal_play_restored` at 17:34:35.685, completing the Journey and returning to the Lab |

These observations come from the Unreal application, not the earlier native example's retained purchase history. They qualify the managed Play purchase/restore path. RTDN delivery, live price text in the authored screen, the external-controller outcome matrix, and App Store sandbox qualification remain outstanding.

## Earlier evidence: useful, not final-candidate qualification

Earlier Lab commit `6b29e26` passed local-backend lifecycle and metered consumption on both platforms: configure/identify, Ready, two entity queries, accepted debit, exact same-ID replay, one saved action, unchanged comparison entity, denied over-balance consumption and replay, locale override/reset, anonymous rotation, reidentification, shutdown/reconfigure, and identity across map travel. Both lifecycle reports ended on `LabSecond`.

Earlier physical iOS runs rendered the published Experience and exercised authored Close and typed App Action controls, releasing pause on dismissal. Earlier Android APKs passed ZIP 16 KiB alignment and all nine arm64 libraries passed ELF load-segment checks. Earlier iOS apps passed strict recursive codesign verification. These results predate the current dispatcher/native pins.

Prepared archive `prepared-all-v6` previously built for IOS, Android, and Mac. Earlier Blueprint-only consumers compiled their real graphs; Android ran the real async configuration-error branch, and an iOS consumer installed/launched on the phone. They must be rebuilt from the final archive. An unconfigured error branch proves module startup, not backend success.

The installed Epic distribution lacks iOS-simulator third-party link inputs, beginning with PLCrashReporter. Swift bridge simulator tests are supported; a full Unreal simulator player is not claimed as passing.

## Remaining qualification

- Final iOS Lab manual/lifecycle/Experience/App Action/dismissal checks after the current changes.
- iOS: preservation of a game-owned pause and map travel while presenting. Complete the external billing outcome matrix on both platforms: success/cancel/pending/failure/restore/no-purchases, duplicate/wrong completion, and selected offer fidelity; iOS expiry/teardown and Android purchase expiry/teardown remain unqualified. Android restore expiry and shutdown evidence is recorded above.
- Real App Store sandbox purchase and restore against the backend using Apple Nuxie Staging (`ai.nuxie.ios.staging`). Complete Play RTDN delivery and live price rendering; managed Play purchase/restore evidence is recorded above. Synthetic external-controller completions do not establish real store outcomes.
- Configured backend success in the fresh prepared Blueprint-only iOS consumer using the committed explicit identifiers; startup and packaging evidence is recorded above.
- Final package signing/alignment checks, review, committed-candidate readiness receipt, and accurate PR evidence.

Use a disposable local app/customer with finite grants for backend checks. The Lab writes `Saved/NuxieLab/validation.json`, checks two distinct entities, persists pending operation IDs and gameplay application, and assumes no concurrent consumers. Distinguish request acceptance, presentation, and verified store outcomes.

For the unconfigured Blueprint-only Android startup check:

```sh
python3 scripts/check-android-blueprint.py --serial <adb-device-id> --apk Examples/BlueprintOnly/Binaries/Android/BlueprintOnly-arm64.apk
```

This installs and launches the consumer, rejects native-module startup failures, and requires its real Blueprint async error branch within 60 seconds. It does not clear data or simulate backend success.
