# Validation

The replacement SDK has passed the checks below. It is **not yet declared merge-ready**: final iOS shutdown/presentation checks, the tracked Android profile-admission observation, and parent-repository integration/readiness remain open. This page separates current device evidence from limitations; historical build results are retained in Git history.

## Candidate and environment

The tested runtime is `718f2801a7e5374d742199842bb8eba7149defa4`, with native pins:

- iOS `f54152c79b26005f902a2487f863ae6f182a3714` ([PR #429](https://github.com/nuxieai/nuxie-ios/pull/429)).
- Android `b32c9fdba42cb15af48c745ac8c2b1640f4c7708` ([PR #111](https://github.com/nuxieai/nuxie-android/pull/111)).

Qualification uses UE 5.8.2 on Mac arm64, an API 36 arm64 Android emulator, and a physical iPhone 17 Pro Max. iOS checks use wired XCTest/devicectl; Android checks use adb. Backend checks use disposable local development apps and customers. Store checks use Apple's Sandbox and Google's no-charge test payment method.

The current Android store APK has SHA-256 `5bedd99e36b5fc3096a0c144aa7b4a5635ab6cf9f0f942d5e21e2d4fc2fdcfff`. Installed bytes match the packaged APK. The main iOS Lab executable has SHA-256 `56fcf3b913c264d14834f56e456f35f0edc46e5d96bb55fd44f296c9a0c9e003`; the Apple store host separately packages the same Release framework into `ai.nuxie.ios.staging`.

## Required local gate

From a clean, committed SDK feature branch:

```sh
node ../../scripts/pr-readiness.mjs run
```

Set `UNREAL_ENGINE_ROOT` to UE 5.8, `JAVA_HOME` to Java 21, and `NUXIE_IOS_SIMULATOR_ID` to an installed simulator. The gate runs `git diff --check origin/main...HEAD` and `python3 scripts/check.py`:

- Production request-ledger checks with address/undefined-behavior sanitizers.
- Plugin and Lab editor compilation.
- All four `Nuxie.Contract` suites: `SessionLifecycle`, `ValuesAndReceipts`, `DeferredBudget`, and `FeatureObserver`.
- Native source-inventory regression, including newly added/deleted Swift and Kotlin files.
- Kotlin bridge build/tests and Swift bridge simulator tests.

The gate passed on the runtime above. The final iOS native gate also passed 1,258 unit tests plus focused runtime, hosted UIKit input, integration and macOS checks. The Android native gate passed its test, API, lint and example-build lanes. Missing tools fail the gate; TypeScript checks do not apply to the Unreal repository.

## Current device results

Dates and times are UTC. These results use the runtime above, not an earlier prepared archive.

| Check | iOS | Android |
| --- | --- | --- |
| Lifecycle: locale, reset/reidentify, denied usage/replay, shutdown/reconfigure, identity across map travel | Five passes, including first launch after installation, September 15 at 23:24:36–48 | Passed September 16 at 00:28:06 |
| Metered consumption | One debit, one saved gameplay action, exact replay, unchanged comparison entity; September 15 at 23:25:05 | Same assertions passed September 16 at 00:28:24; process restart replayed the same operation at 00:29:20 with one applied action |
| Four external purchase outcomes | Purchased, cancelled, pending and failed passed September 16, 00:45:53–00:49:49 | All passed September 16, 00:13:03–00:14:34 |
| Three external restore outcomes | Restored, no purchases and failed passed September 16, 00:50:00–23 | All passed September 16, 00:14:55–00:15:20 |
| Retained request expiry | Purchase and restore expired, completed their failure routes, released pause and rejected late completion | Purchase and restore passed, 00:22:02 and 00:23:27 |
| Shutdown invalidates retained request | Final wired interaction check remains open | Purchase and restore passed, 00:23:56 and 00:24:51 |
| Game-owned pause and callbacks beneath native UI | Final wired interaction check remains open | Typed App Action and deferred local rejection delivered while paused/presenting; dismissal retained the game-owned pause, 00:25:34–45 |
| Map travel and background/foreground with native UI | Final wired interaction check remains open | Travel to `LabSecond`, retained identity/presentation, same process across Home/foreground, and pause release on final dismissal; 00:26:31–00:27:08 |

Each external-controller report checks invalid-outcome rejection without consuming the request, acceptance of the valid outcome, duplicate rejection, and the selected store product (plus Android base plan). These are **simulated controller outcomes**, separate from the real store transactions below. Pending can intentionally leave the Experience open.

The temporary iOS harness initially assumed every outcome dismissed the Experience; its incorrect assertion was removed for pending. A separate XCTest relaunch failed before producing a new report; direct wired launch followed by test attachment succeeded. Stale reports were excluded. The later shutdown check was interrupted by the phone switching to another app, so it is not recorded as passed.

## Real store purchase and restore

All times in this section are September 16, 2026 UTC.

### Google Play

The current `ai.nuxie.example` Lab used `nuxie_qualification`, base plan `monthly`. The real Play sheet displayed **Test card, always approves** and explicitly stated that no charge would occur.

- Purchase completed at 00:18:14.065, followed `unreal_play_purchased`, completed its Journey and released pause. `purchase_synced` arrived at 00:18:14.072.
- A warm managed launch reached Ready. Restore completed at 00:19:19.601, followed `unreal_play_restored`, dismissed the Experience and released pause.
- The Lab's remote Feature query returned allowed at 00:19:31.549. An independent backend `/entitled` query returned HTTP 200 with `unreal_play_access.allowed=true` at 00:19:54.682.

An earlier runtime received a real Google `SUBSCRIPTION_PURCHASED` notification through Google's signed push identity token: normal webhook authentication returned HTTP 202 and the queue completed 1/1. The temporary subscription and route-limited tunnel were removed. The exact persisted RTDN reconciliation result was not independently inspected, and a final-candidate RTDN rerun is not claimed.

### App Store

The current Apple Staging Lab used `ai.nuxie.ios.staging.unreal.qualification.monthly`. StoreKit displayed Sandbox, $0.99/month and the no-charge notice; the user authenticated and physically confirmed the purchase.

- Empty restore completed at 00:13:20.156 through `unreal_apple_empty`.
- Real purchase completed at 00:24:13.603 through `unreal_apple_purchased`.
- Authenticated active restore completed at 00:33:32.180 through `unreal_apple_restored`.
- Each terminal route completed the Journey, dismissed the Experience and released its pause.

The active restore exposed backend handling of a same-original subscription purchased again after a lapse. [UNIV-3191](https://universe.basis.dev/issue/UNIV-3191) fixes that transition in authoritative history, preserving owned source IDs, immutable terms, the access gap, and strict contiguity for actual renewals. It also retains original-purchase and single-transaction deferral.

With backend commit `0533ac72fb`, the real queue completed 1/1. Transaction `2000001236865032` returned HTTP 200 and `unreal_apple_access.allowed=true` at 00:43:34.405. The device emitted `purchase_synced` for both retained transactions at 00:43:36.241–242. Apple's signed subscription status independently reported active at 00:43:58.182. The fix passed 835 connector unit tests, root lint (69 tasks), root typecheck (70 tasks), and independent standards/spec reviews with no actionable findings. Its backend PR/readiness remains separate from the native SDK receipt.

Related backend fixes cover requested-lineage filtering, receipt order and chronological history ([UNIV-3186](https://universe.basis.dev/issue/UNIV-3186)), and full signed subscription-status admission ([UNIV-3189](https://universe.basis.dev/issue/UNIV-3189)). Signature, customer ownership and published release checks remain enforced.

## Prepared Blueprint-only consumers and packaging

`prepared-all-718f280` built for IOS, Android and Mac. All 285 manifest hashes verified. A fresh Blueprint-only consumer copied this archive; all 120 non-generated plugin files still matched after host builds.

- iOS: packaged, passed strict signing, installed cleanly, ran its actual Blueprint identity-success branch at September 15 23:32:59.642, rendered the backend Experience and returned to Unreal after authored Close. Full-device screenshots verified landscape presentation and dismissal.
- Android: a fresh app-data launch with the local endpoint ran the actual Blueprint identity-success branch at September 15 23:34:32.137, rendered the backend Experience and returned to Unreal after authored Close.
- The current Android store APK passed v2 signature verification, ZIP 16 KiB alignment, and load-segment alignment/congruence for all nine ELF libraries. Packaging used `-UbtArgs=-NoUBA` after UE's accelerator reported symlink bookkeeping errors.
- iOS app signing passed `codesign --verify --deep --strict`.

Useful commands:

```sh
python3 scripts/package.py --output dist/prepared
python3 scripts/check-external-controller.py <report> --kind purchase --outcome purchased --store-product-id <store-product-id>
python3 scripts/check-external-controller.py <report> --kind restore --outcome restored
zipalign -c -P 16 4 <apk>
apksigner verify --verbose <apk>
codesign --verify --deep --strict <ios-app>
```

Android purchase reports also take `--base-plan-id <base-plan-id>` when applicable. Check report timestamps against the current run and retain native activity/App Action observations. A prior report or unconfigured Blueprint error branch does not establish configured backend success.

## Running the Lab

Use a disposable local app/customer with finite grants and two distinct entities. The Lab records settings, observations and validation reports under `Saved/NuxieLab` on iOS/Mac and private `files/NuxieLab` on Android. Android debug hosts expose these through `adb shell run-as <package>`. The runner logs its settings path.

`validation.json` verifies one debit, exact receipt replay, one persisted gameplay action, and an unchanged comparison entity. The check assumes no concurrent consumers. Pending operation IDs and applied gameplay actions survive process restart.

Development-only `auto.json` options exercise presentation policy:

- `validateOverlayDispatch: true`: from an authored App Action, make an invalid local Feature query and verify its deferred failure arrives while the Experience covers gameplay.
- `gameOwnedPause: true`: pause before configuration; after dismissal, require `paused=true, presenting=false`.
- `travelOnAppAction: true`: make `unreal_qa_action` travel to `LabSecond` without reconfiguring; require identity, Ready state, presentation and pause to survive. Run separately from game-owned pause, then verify dismissal releases the Experience-owned pause.
- `externalBilling: true` with a supported `externalPurchaseOutcome` or `externalRestoreOutcome`: exercise the controller protocol. An empty outcome retains the request for manual completion, expiry or shutdown checks.

These probes are excluded from Shipping builds. Keep public keys and local fixtures out of committed example defaults; never commit store credentials, purchase tokens or signed transaction bodies.

## Remaining qualification and tracked limitations

- Complete the final iOS shutdown and presentation/pause/travel interaction checks while the unlocked phone is available exclusively to automation.
- Classify the retained-data Android profile-admission observation under [UNIV-3190](https://universe.basis.dev/issue/UNIV-3190). One managed launch after external-controller checks rejected authentication. Clean managed and subsequent warm launches passed, but that does not establish the original cause. The preserved private backup contains app files, not the original shared-preferences replay floor.
- Finish current-head reviews/readiness and the parent backend/submodule integration PR. Preserve unrelated work.
- Historical Apple lineage recovery without an initial ownership anchor remains tracked separately in [UNIV-3184](https://universe.basis.dev/issue/UNIV-3184). The fresh, anchored qualification lineage above passes.
- A retained schema-v1 database from an older native test host requires a clean supported installation; clearer failure propagation is tracked in [UNIV-3183](https://universe.basis.dev/issue/UNIV-3183).
- The installed Epic distribution lacks some iOS-simulator third-party link inputs. Swift bridge simulator tests pass; a full Unreal simulator player is not claimed.
