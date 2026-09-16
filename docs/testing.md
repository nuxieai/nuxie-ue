# Validation

The replacement SDK has passed the checks below. It is **not yet declared merge-ready**: the current Apple sandbox transaction, final readiness receipt, and parent integration PR remain open. The historical Android profile-admission investigation is tracked separately. This page separates current device evidence from limitations; historical build results are retained in Git history.

## Candidate and environment

Runtime plugin candidate: `be33771d855d8bbd8b4c3dd19e18ba8462036036`. Lab candidate `a62f360` adds a Development-only shutdown probe; plugin source and native dependencies are unchanged.

- iOS `5369e7c06ef986fcea666a92ced816c90530aa95` ([PR #429](https://github.com/nuxieai/nuxie-ios/pull/429)).
- Android `57561cb57fa0f333b0ca06cf8a3652375953337b` ([PR #111](https://github.com/nuxieai/nuxie-android/pull/111)).
- Parent integration `133d16f7dfab29993e7334ed10522bfaea9c09f8`, based on `e42ce6bcc5`.

Qualification uses UE 5.8.2 on Mac arm64, an API 36 arm64 Android emulator, and a physical iPhone 17 Pro Max. iOS checks use wired XCTest/devicectl; Android checks use adb. Backend checks use disposable local development apps and customers. Store checks use Apple's Sandbox and Google's no-charge test payment method.

The refreshed Lab binaries at `a62f360` have SHA-256 `4e327984706e86ef9e632d578271844f181bb52f5cab3ac822482273b50c0fa5` (Android APK) and `330c99fbea077e957de0752819747de688f709a18770504302a477c6b2947cba` (Apple Lab executable). Earlier `be33771` controller and store checks exercise unchanged plugin source and native dependencies. The added Lab shutdown probe was packaged and run on both platforms.

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

The gate passed at current runtime `be33771`. The current iOS native gate also passed its unit, focused runtime, hosted UIKit input, integration and macOS checks. The Android native gate passed its test, API, lint and example-build lanes. Missing tools fail the gate; TypeScript checks do not apply to the Unreal repository.

## Current device results

Dates and times are September 16, 2026 UTC. Both columns qualify the current native pins; the new shutdown probe uses the `a62f360` Lab.

| Check | iOS | Android |
| --- | --- | --- |
| Lifecycle: locale, reset/reidentify, denied usage/replay, shutdown/reconfigure, identity across map travel | Two fresh passes at 02:14:17 and 02:14:20 | Passed at 01:36:23 |
| Metered consumption | One debit, one saved action, exact replay and unchanged second entity at 02:14:22 | Passed at 01:36:33; process restart replay preserved the prior applied-action count |
| Four external purchase outcomes | Purchased, cancelled, pending and failed passed at 01:55–01:56 | All passed on the refreshed runtime |
| Three external restore outcomes | Restored, no purchases and failed passed at 01:55–01:56 | All passed on the refreshed runtime |
| Retained request expiry | Both requests expired, followed failure routes, released pause and rejected late completion | Both passed |
| Shutdown invalidates retained request while native UI is active | Purchase/restore passed at 02:13:37/48; shutdown took 0.60/0.63 seconds | Purchase/restore passed at 02:16:47/57; shutdown took 0.25/0.12 seconds |
| Game-owned pause and callbacks beneath native UI | Typed App Action and deferred local rejection delivered while paused/presenting; dismissal preserved game-owned pause | Same assertions passed |
| Map travel and background/foreground with native UI | `LabSecond`, retained presentation and pause through Home/resume, release on authored Close | Same-process Recent Apps resume preserved presentation; authored Close released pause |

Each external-controller report checks invalid-outcome rejection without consuming the request, acceptance of the valid outcome, duplicate rejection, and the selected store product (plus Android base plan). These are **simulated controller outcomes**, separate from the real store transactions below. Pending can intentionally leave the Experience open.

The iOS shutdown probe calls public `Shutdown` directly while the external request is retained and native UI owns input. Native Close intentionally waits for an in-flight purchase; closing first cannot exercise shutdown while pending. Reports prove that shutdown completes well before the 60-second deadline, the request object remains retained but inactive, late completion fails, and status becomes Unconfigured.

Android foreground qualification resumes the existing task through Recent Apps. An explicit adb activity launch re-enters Unreal's single-task activity and dismisses the overlay; it is not counted as existing-task resume.

## Real store purchase and restore

All times in this section are September 16, 2026 UTC.

### Google Play

The current `ai.nuxie.example` Lab used `nuxie_qualification`, base plan `monthly`. The real Play sheet displayed **Test card, always approves** and explicitly stated that no charge would occur.

- Purchase completed at 01:44:11.333 (backend sync at 01:44:12.182), followed `unreal_play_purchased`, completed its Journey and released pause. `purchase_synced` arrived at 01:44:12.182.
- A warm managed launch reached Ready. Restore completed at 01:44:53.869, followed `unreal_play_restored`, dismissed the Experience and released pause.
- The Lab's remote Feature query returned allowed at 01:45:17.452. An independent backend `/entitled` query returned HTTP 200 with `unreal_play_access.allowed=true` at 01:44:54.428.

An earlier runtime received a real Google `SUBSCRIPTION_PURCHASED` notification through Google's signed push identity token: normal webhook authentication returned HTTP 202 and the queue completed 1/1. The temporary subscription and route-limited tunnel were removed. The exact persisted RTDN reconciliation result was not independently inspected, and a final-candidate RTDN rerun is not claimed.

### App Store

The historical Apple Staging Lab used `ai.nuxie.ios.staging.unreal.qualification.monthly`. StoreKit displayed Sandbox, $0.99/month and the no-charge notice; the user authenticated and physically confirmed the purchase.

- Empty restore completed at 00:13:20.156 through `unreal_apple_empty`.
- Real purchase completed at 00:24:13.603 through `unreal_apple_purchased`.
- Authenticated active restore completed at 00:33:32.180 through `unreal_apple_restored`.
- Each terminal route completed the Journey, dismissed the Experience and released its pause.

The active restore exposed backend handling of a same-original subscription purchased again after a lapse. [UNIV-3191](https://universe.basis.dev/issue/UNIV-3191) fixes that transition in authoritative history, preserving owned source IDs, immutable terms, the access gap, and strict contiguity for actual renewals. It also retains original-purchase and single-transaction deferral.

With backend commit `0533ac72fb`, the real queue completed 1/1. Transaction `2000001236865032` returned HTTP 200 and `unreal_apple_access.allowed=true` at 00:43:34.405. The device emitted `purchase_synced` for both retained transactions at 00:43:36.241–242. Apple's signed subscription status independently reported active at 00:43:58.182. The fix passed 835 connector unit tests, root lint (69 tasks), root typecheck (70 tasks), and independent standards/spec reviews with no actionable findings. Its backend PR/readiness remains separate from the native SDK receipt.

Related backend fixes cover requested-lineage filtering, receipt order and chronological history ([UNIV-3186](https://universe.basis.dev/issue/UNIV-3186)), and full signed subscription-status admission ([UNIV-3189](https://universe.basis.dev/issue/UNIV-3189)). Signature, customer ownership and published release checks remain enforced.

## Prepared Blueprint-only consumers and packaging

`prepared-all-be33771` built for IOS, Android and Mac. All 310 manifest hashes verified. A fresh Blueprint-only consumer copied this archive; all 145 non-generated plugin files still matched after host builds.

- iOS: the refreshed consumer packaged and passed strict signing. Executable SHA-256: `435051c2b0cde03c6540a2a2f9a23f5b5559f991440175330cdbb1b625de9dd1`. A fresh installation ran the real identity-success Blueprint, rendered the published Experience at 02:17:54, and returned to Unreal after authored Close. A prior retained-data launch had already completed its startup Journey; it was not counted as a fresh-install rendering test.
- Android: a fresh app-data launch with the local endpoint ran the actual Blueprint identity-success branch at September 16 01:33:45.165, rendered the backend Experience and returned to Unreal after authored Close.
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
- `externalShutdownKind: "purchase"` or `"restore"`, together with external billing: invoke public Shutdown while the retained checkout is active and record `external-shutdown.json`.
- `externalBilling: true` with a supported `externalPurchaseOutcome` or `externalRestoreOutcome`: exercise the controller protocol. An empty outcome retains the request for manual completion, expiry or shutdown checks.

These probes are excluded from Shipping builds. Keep public keys and local fixtures out of committed example defaults; never commit store credentials, purchase tokens or signed transaction bodies.

## Remaining qualification and tracked limitations

- Complete the current Apple sandbox transaction and record its authoritative backend result.
- Classify the retained-data Android profile-admission observation under [UNIV-3190](https://universe.basis.dev/issue/UNIV-3190). One managed launch after external-controller checks rejected authentication. Clean managed and subsequent warm launches passed, but that does not establish the original cause. The preserved private backup contains app files, not the original shared-preferences replay floor.
- Finish current-head reviews/readiness and the parent backend/submodule integration PR. Preserve unrelated work.
- Historical Apple lineage recovery without an initial ownership anchor remains tracked separately in [UNIV-3184](https://universe.basis.dev/issue/UNIV-3184). The fresh, anchored qualification lineage above passes.
- A retained schema-v1 database from an older native test host requires a clean supported installation; clearer failure propagation is tracked in [UNIV-3183](https://universe.basis.dev/issue/UNIV-3183).
- The installed Epic distribution lacks some iOS-simulator third-party link inputs. Swift bridge simulator tests pass; a full Unreal simulator player is not claimed.

## Parent integration evidence

`pnpm run pr:ready` passed at parent `133d16f7df`: commerce documentation contract and exact Swift/provider compilation, root lint (69 tasks), root typecheck (70 tasks), connector unit tests (835), durable-object unit tests (1,076), and readiness-policy tests (66). SDK gates pass separately at the native pins above. The associated parent PR is prepared but not yet opened; final qualification and current evidence documentation still need completion.

Six alternating external/managed launches (three pairs) also reached Ready and presented authenticated Experiences without clearing the current Android installation. The historical [UNIV-3190](https://universe.basis.dev/issue/UNIV-3190) rejection remains unclassified; these successful current runs do not establish its cause.
