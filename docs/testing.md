# Validation

## Final native pins

iOS `1e6970f306a9dac2ed567a239bf0e64a83e2d7cc` and Android
`20f9d42f7d5fe1cba6e2426d63c24499eb966ce7` add shared immutable-video
bindings, caption preference refresh, and cancellation of obsolete admission
preparation. These retain the previously qualified runtime renderer artifacts.
Both final native preparations passed and their receipts validate against the
exact frozen pins, source inventory, and artifact bytes. Full BuildPlugin
packaging passed IOS, Android, and Mac at `f79f309`; independent verification
checked all 321 packaged file hashes and exact pins. Logs use
`.nuxie/task3b-unreal-frozen-`; the package is `dist/video-frozen-f79f309`.
Historical device evidence below remains scoped to its tested native revisions;
this final refresh repeats native preparation, packaging, and wrapper checks.

## Pins qualified before final admission and caption fixes

iOS `48fa51d6591f61d437620abfa06eb7fcb1a64564` adds cache preservation when a
second signed release declares inconsistent size metadata for a valid cached
video. Android remains `4d65783e2eec5b585673041146dff887258d3c93`. Both native preparations passed; independent receipt validation checked two Apple
archives and 81 Android artifacts. Full BuildPlugin packaging passed IOS, Android,
and Mac at `fc552f5`; all 296 packaged file hashes and exact pins were verified.
The current Apple framework was staged into the unchanged, normally rendered
Metal Lab player and re-signed as described below. Eight physical Bat Phone
screenshots showed repeated red/blue phases on warm launch. Evidence is retained
in `.nuxie/task3b-unreal-lease-ios/`; preparation/package logs use the
`.nuxie/task3b-unreal-lease-fix-` prefix. The earlier cold and origin-outage Apple
evidence below identifies its preceding revision explicitly.

## Published video pins qualified before cache rejection fix

iOS `95d76d41eb4cc945cb57e5c1bcd8333ed15d55cc` and Android `4d65783e2eec5b585673041146dff887258d3c93` include
published Apple runtime 0.10.8 and Android runtime 0.4.8, rendered-video visibility,
and interruption recovery fixes. Both native preparations passed, and independent
receipt validation verified two Apple archives and 81 Android artifacts. The
native source-inventory regression also passed. The local readiness gate passed at `6082fd6`: sanitized C++ checks, all four
Unreal automation suites, nine Android bridge tests, and ten Swift bridge tests.
Full BuildPlugin packaging passed IOS, Android, and Mac; independent verification
checked all 296 packaged file hashes and exact native pins.

Physical iOS qualification used the Bat Phone with the normal UE Metal player.
The retained engine executable was unchanged; the freshly built Release
`NuxieUnrealBridge.framework` replaced its older dynamic framework. The nested
framework and app were signed again with the existing development identity and
entitlements; `codesign --verify --deep --strict` passed. This explicitly
qualifies that staged integration, not a fresh full Unreal package build.
After a clean installation, 12 screenshots showed repeated red/blue video
phases. A warm process relaunch passed six samples. A further warm relaunch
while the fixture origin process was suspended passed eight samples; this is
bounded origin-outage coverage, not an airplane-mode or indefinite-offline claim.
The origin was restored immediately afterward.

The exported native cache independently matched the signed inventory:
scene `242a0ebc242f2617d923a9e1e04a7cf01b9d5d6a8e62cc934f1f26df6efef4db`
(1,086 bytes) and video
`f0a65563c100506c0f98c138e8be1ae333c9879bb77237fbada60bddcfa78669`
(22,065 bytes). Screenshots, capture times, pixel results, cache verification,
and staged executable/framework hashes are retained in the parent worktree's
`.nuxie/task3b-unreal-current-ios/`. Native preparation logs are
`.nuxie/task3b-unreal-final-ios-prepare.log` and
`.nuxie/task3b-unreal-final-android-prepare.log`.

Android playback is qualified through the supported DXT cook below. Earlier
results identify the revisions they qualified.

## Current Android host startup diagnosis — September 18, 2026

The retained ETC2 Development player reproducibly fails before SDK configuration
on the approved API 36 emulator. `-vulkan -AllowCPUDevices` reaches UE's
`FTexture2DResource::GetPlatformMipsSize`, then `RHICalcTexturePlatformSize`
fails its temporary `vkCreateImage` with `VK_ERROR_VALIDATION_FAILED_EXT`.
The device reports llvmpipe Vulkan 1.3.0. This reproduces the earlier ASTC failure;
a pass-through observation layer subsequently identified the failing image as
`VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK` (148), 32×32, six mip levels, mutable-format
flag, usage 7, and compatible view formats 147/148. A standalone Vulkan program
then reproduced the same creation failure outside Unreal and the SDK, both with
and without the view-format list. The same program successfully created BC1,
BC3, and RGBA SRGB images with identical dimensions and flags, despite the driver
advertising support for all four formats. This isolates the ETC2 creation failure
to the emulator driver. A supported DXT cook successfully starts the normal Vulkan engine and SDK;
shipping texture settings remain unchanged. The emulator driver defect is tracked
in [UNIV-3287](https://universe.basis.dev/issue/UNIV-3287).

A scoped Khronos validation layer identifies a separate concrete engine/driver
contract error: UE enables `VK_KHR_dynamic_rendering` without its required
`VK_KHR_depth_stencil_resolve` extension. Setting
`-ini:Engine:[ConsoleVariables]:r.Vulkan.AllowDynamicRendering=0` is accepted
but leaves the extension enabled and the image failure unchanged. It is not a
fix. Both validation-layer versions 1.4.357 and 1.3.290 then crash during engine
initialization, so they do not establish the original image failure's cause.
`-nullrhi` also fails before SDK configuration: Android inherits the generic
PC D3D shader-format selection and `FNullDynamicRHI::Init` rejects that platform.
No SDK or engine workaround has been committed. All temporary global Android
GPU-debug settings were removed after diagnosis.

The retained logs in the parent worktree are
`.nuxie/task3b-unreal-agent-startup.log`,
`.nuxie/task3b-unreal-agent-nullrhi.log`,
`.nuxie/task3b-unreal-agent-validation-correct.log`,
`.nuxie/task3b-unreal-agent-validation-1.3.log`, and
`.nuxie/task3b-unreal-agent-no-dynamic-rendering.log`.
The Android Development Lab was built with `BuildCookRun -platform=Android
-cookflavor=DXT -clientconfig=Development -build -cook -stage -pak -package
-map=Lab+LabSecond -AdditionalCookerOptions=-nowrite`. Launch arguments were
`-vulkan -AllowCPUDevices
-ini:Engine:[ConsoleVariables]:r.Vulkan.SupportsBCTextureFormats=1`; this enables
the BC formats independently verified on this emulator. A clean app-data launch
produced seven actual video samples after loading, with repeated red/blue phases.
Warm process restart and actual Home/Recents return each passed three visible
phase transitions. A warm relaunch while the shared fixture origin was suspended
also passed three transitions after the profile request timed out; the origin
was restored in a finally block. This is bounded origin-outage coverage.
Exported scene/video cache bytes matched the signed hashes
and sizes listed above. Evidence is retained in
`.nuxie/task3b-unreal-dxt-android/`; build output is
`.nuxie/task3b-unreal-android-dxt-build.log`. This is emulator qualification with
normal engine rendering, not a hardware-decoder performance measurement.

## Native video format fix — September 18, 2026

The preceding qualification used iOS `858321e2` and Android `1514b1c`. Native preparation
completed against the iOS content-addressed-video fix. Both native receipts
validate current source, pins, and bytes (two Apple archives and 56 Android
artifacts). `scripts/package.py --output dist/video-alias-fix` passed all IOS, Android,
and Mac BuildPlugin targets. Independent verification checked all 271 file hashes
and the exact native pins. Actual signed-video mobile playback and final
readiness remain outstanding at these pins.

## Earlier native pin refresh — September 18, 2026

The earlier development pins were iOS `072e38b24df67f7e6326815ed5e126c93c8e67d7`
and Android `1514b1cce3d64502b483c41fa551e7290caf10b0`, including shared
decoder admission and hidden-screen suspension. Android preparation rebuilt
the pinned SDK and bridge; iOS preparation rebuilt Release device/simulator
archives. Both receipts validate against current source, pins, and bytes.

`scripts/check.py` passed the sanitized C++ request ledger, receipt tests,
Unreal editor/Lab build and automation, Android bridge tests/build, and all ten
Swift simulator bridge tests. Android used an isolated Gradle cache because
shared cache metadata was unavailable. `scripts/package.py --output
dist/video-refresh` passed BuildPlugin for IOS, Android, and Mac. Independent
inspection verified all 271 packaged file hashes and the current native pins.
Signed-video playback through Unreal mobile players and final readiness/review
remain outstanding.

## Earlier video delivery candidate — September 18, 2026

This candidate pins pushed native development revisions iOS
`38428e8bb1c65605d6c982ff22b2a18d63229950` and Android
`e76714a76e14b8f293e782934c107a789d0a67f0`, pending final native qualification
and review under [UNIV-3262](https://universe.basis.dev/issue/UNIV-3262).

On UE 5.8.2 and Xcode 27, `scripts/check.py` passed the sanitized production C++
request-ledger checks, native source-inventory regression, editor plugin/Lab
build, all four `Nuxie.Contract` automation suites, nine Android bridge tests,
and ten Swift simulator bridge tests. Android preparation built the pinned SDK
and bridge AAR. iOS preparation produced Release embedded-framework archives
for device and simulator; the simulator binary contains both arm64 and x86_64.
Both platform receipts validate against current source, pins, and artifact bytes.

`scripts/package.py --output dist/video-candidate` also passed BuildPlugin for
iOS, Android arm64, and Mac arm64, including Development and Shipping targets
and the Mac editor plugin. Independent verification checked all 246 packaged
file hashes and exact native pins. Mac compilation is editor/package evidence;
it does not add a supported desktop native Experience host.

These checks establish bridge behavior and native build integration. They do
not qualify signed-video playback, acquisition, captions, or lifecycle through
an Unreal mobile player. Those checks and final readiness/review remain
outstanding. The earlier live/store evidence below covers the previous native
revisions and must not be treated as video-candidate evidence.

## Earlier qualification

The replacement SDK has passed local qualification on both mobile platforms: public API behavior, controller lifetime, native presentation, real sandbox purchase/restore, prepared Blueprint-only consumers, and Shipping artifact checks. The exact candidate, evidence and explicitly unrun release lanes are recorded below. Final committed-tree readiness is recorded in [Unreal PR #6](https://github.com/nuxieai/nuxie-ue/pull/6) and [parent integration PR #6512](https://github.com/nuxieai/nuxie-dev/pull/6512).

## Candidate and environment

The merged dependency pins are iOS `0a84b32475a48db097b4e8b32e05b44736801ed0` and Android `a7130806b4212b18284711a7720b6fa677baeb57`. iOS has the identical Git tree as its qualified candidate below; Android differs only in accessibility device tests and qualification documentation. Production native source is unchanged. CI retirement also changes no runtime code. The final local gate verifies these landed pins.

Runtime and Android Lab candidate: `2ebaa68a21809ea813f130ba5e5ee48a66ef5427`. The iOS Lab uses `a62f360`; its plugin source and iOS dependency are unchanged. Rebuilding the prepared iOS framework after the Android refresh verified all six physical-device payload files byte-for-byte identical, including the executable and four bundle resources. The iOS device, store, consumer and Shipping results below therefore qualify the same iOS implementation. The rebuilt simulator executable is covered separately by the Swift bridge gate.

- iOS `5369e7c06ef986fcea666a92ced816c90530aa95` ([PR #429](https://github.com/nuxieai/nuxie-ios/pull/429)).
- Android `27e7abe145156f99c4a57e1a9f46fd1763e9ceb3` ([PR #111](https://github.com/nuxieai/nuxie-android/pull/111)).
- Backend qualification uses parent `0bebd4610ae5d342cb9627607aa7917f22530448`, based on `88b63bd2b7`; [PR #6512](https://github.com/nuxieai/nuxie-dev/pull/6512) records the final SDK pointer.

Qualification uses UE 5.8.2 on Mac arm64, an API 36 arm64 Android emulator, and a physical iPhone 17 Pro Max. iOS checks use wired XCTest/devicectl; Android checks use adb. Backend checks use disposable local development apps and customers. Store checks use Apple's Sandbox and Google's no-charge test payment method.

The Android Lab APK has SHA-256 `a1cee367d7364c4a9e77bb21ed99c51f90c5acbbbe1f4097a1bf0af3a0710791`; the qualified Apple Lab executable has SHA-256 `330c99fbea077e957de0752819747de688f709a18770504302a477c6b2947cba`. The Development-only shutdown probe was packaged and run on both platforms.

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

The PR records the committed-tree gate receipt, including the shutdown probe and current native pins. The current iOS native gate also passed its unit, focused runtime, hosted UIKit input, integration and macOS checks. The Android native gate passed its test, API, lint and example-build lanes. Missing tools fail the gate; TypeScript checks do not apply to the Unreal repository.

## Hosted validation

The manual **SDK validation** GitHub workflow executes the portable production request-ledger test with address/undefined-behavior sanitizers on Linux. Full Unreal and native qualification uses the required local gate above because it requires the licensed UE installation and mobile toolchains.

The retired `nuxie-unreal-ci` Buildkite pipeline is archived; its pipeline configuration and compatibility launcher have been removed. PRs use the local readiness receipt rather than the former `Required signoffs` status.

## Current device results

Dates and times are September 16, 2026 UTC. Both columns qualify the current native pins; Android was rerun after integrating the native accessibility refresh.

| Check | iOS | Android |
| --- | --- | --- |
| Lifecycle: locale, reset/reidentify, denied usage/replay, shutdown/reconfigure, identity across map travel | Two fresh passes at 02:14:17 and 02:14:20 | Passed at 04:06:27 |
| Metered consumption | One debit, one saved action, exact replay and unchanged second entity at 02:14:22 | Passed at 04:06:34; process restart replay preserved the prior applied-action count |
| Four external purchase outcomes | Purchased, cancelled, pending and failed passed at 01:55–01:56 | All passed on the refreshed runtime |
| Three external restore outcomes | Restored, no purchases and failed passed at 01:55–01:56 | All passed on the refreshed runtime |
| Retained request expiry | Both requests expired, followed failure routes, released pause and rejected late completion | Both passed |
| Shutdown invalidates retained request while native UI is active | Purchase/restore passed at 02:13:37/48; shutdown took 0.60/0.63 seconds | Purchase/restore passed at 04:09:13/04:09:24; shutdown took 0.33/0.26 seconds |
| Game-owned pause and callbacks beneath native UI | Typed App Action and deferred local rejection delivered while paused/presenting; dismissal preserved game-owned pause | Same assertions passed |
| Map travel and background/foreground with native UI | `LabSecond`, retained presentation and pause through Home/resume, release on authored Close | Same-process Recent Apps resume preserved presentation; authored Close released pause |

Each external-controller report checks invalid-outcome rejection without consuming the request, acceptance of the valid outcome, duplicate rejection, and the selected store product (plus Android base plan). These are **simulated controller outcomes**, separate from the real store transactions below. Pending can intentionally leave the Experience open.

The iOS shutdown probe calls public `Shutdown` directly while the external request is retained and native UI owns input. Native Close intentionally waits for an in-flight purchase; closing first cannot exercise shutdown while pending. Reports prove that shutdown completes well before the 60-second deadline, the request object remains retained but inactive, late completion fails, and status becomes Unconfigured.

Android foreground qualification resumes the existing task through Recent Apps. An explicit adb activity launch re-enters Unreal's single-task activity and dismisses the overlay; it is not counted as existing-task resume.

## Real store purchase and restore

All times in this section are September 16, 2026 UTC.

### Google Play

The current `ai.nuxie.example` Lab used `nuxie_qualification`, base plan `monthly`. The real Play sheet displayed **Test card, always approves** and explicitly stated that no charge would occur.

- Purchase completed at 04:14:24.100 (backend sync at 04:14:25.097), followed `unreal_play_purchased`, completed its Journey and released pause. `purchase_synced` arrived at 04:14:25.097.
- A warm managed launch reached Ready. Restore completed at 04:14:39.077, followed `unreal_play_restored`, dismissed the Experience and released pause.
- An independent backend `/entitled` query returned HTTP 200 with `unreal_play_access.allowed=true` after restore at 04:14:41.009.

The initial fresh-checkout attempt correctly rejected an existing active subscription without an explicit replacement policy. Google independently confirmed that the existing purchase was a test subscription; revoking that disposable test purchase allowed the fresh checkout above. No SDK behavior was changed for this fixture reset.

An earlier runtime received a real Google `SUBSCRIPTION_PURCHASED` notification through Google's signed push identity token: normal webhook authentication returned HTTP 202 and the queue completed 1/1. The temporary subscription and route-limited tunnel were removed. The exact persisted RTDN reconciliation result was not independently inspected, and a final-candidate RTDN rerun is not claimed.

### App Store

The current Apple Staging Lab uses `ai.nuxie.ios.staging.unreal.qualification.monthly`. StoreKit displayed Sandbox, $0.99/month and the no-charge notice; the user authenticated and physically confirmed the purchase.

- Real cancellation completed at 02:47:02.722 through `unreal_apple_cancelled` and released pause.
- Real purchase completed at 03:16:57.530 through `unreal_apple_purchased`.
- Active restore completed at 03:18:00.157 through `unreal_apple_restored`.
- Both success routes completed their Journeys, dismissed their Experiences and released pause. The local fixture can immediately admit another Journey; each dismissal is checked before the next presentation.
- Device `purchase_synced` arrived at 03:17:39.623 and the client returned to Ready. The initial retryable reconciliation response was not counted as successful synchronization.
- An independent authenticated `/purchase` replay of Apple's signed transaction `2000001236916382` returned HTTP 200, idempotent replay and `unreal_apple_access.allowed=true` at 03:17:50.418.
- Apple's signed subscription status independently reported the same original lineage active at 03:18:07.720. Signed proof bodies and account credentials remain private.

An additional checkout emitted its failure route before the successful restore; it is retained in the device observations and is not counted as a successful purchase. Store-managed deferred/Ask-to-Buy approval is unrun with this account configuration. The external-controller pending cases prove that protocol outcome, not a real deferred store transaction.

Earlier sandbox qualification exposed a same-original subscription purchased again after a lapse. [UNIV-3191](https://universe.basis.dev/issue/UNIV-3191) fixes that transition in authoritative history, preserving owned source IDs, immutable terms, the access gap, and strict contiguity for actual renewals. Original-purchase and single-transaction deferral remain intact. The fresh transaction above independently exercises that fixed path on the current backend. The fix passed 835 connector unit tests, root lint/typecheck and independent standards/spec reviews.

Related backend fixes cover requested-lineage filtering, receipt order and chronological history ([UNIV-3186](https://universe.basis.dev/issue/UNIV-3186)), and full signed subscription-status admission ([UNIV-3189](https://universe.basis.dev/issue/UNIV-3189)). Signature, customer ownership and published release checks remain enforced.

## Prepared Blueprint-only consumers and packaging

`prepared-all-2ebaa68` built for IOS, Android and Mac. All 360 manifest hashes verified. A fresh Android Blueprint-only consumer copied this archive; all 196 non-generated plugin files still matched after the host build. The qualified iOS consumer used `prepared-all-be33771`; the refreshed archive has identical physical iOS framework payloads and unchanged iOS plugin source.

- iOS: the refreshed consumer packaged and passed strict signing. Executable SHA-256: `435051c2b0cde03c6540a2a2f9a23f5b5559f991440175330cdbb1b625de9dd1`. A fresh installation ran the real identity-success Blueprint, rendered the published Experience at 02:17:54, and returned to Unreal after authored Close. A prior retained-data launch had already completed its startup Journey; it was not counted as a fresh-install rendering test.
- Android: a fresh app-data launch with the local endpoint ran the actual Blueprint identity-success branch at September 16 04:09:40.334, rendered the backend Experience and returned to Unreal after authored Close.
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

## Tracked limitations and release boundaries

- [UNIV-3190](https://universe.basis.dev/issue/UNIV-3190) retains one historical Android profile-admission rejection. Six alternating external/managed launches on the current runtime passed without clearing app data. The original cause remains unclassified: the preserved backup lacks the original shared-preferences replay floor. Authentication fences were not weakened.
- Historical Apple lineage recovery without an initial ownership anchor remains tracked in [UNIV-3184](https://universe.basis.dev/issue/UNIV-3184). The anchored qualification lineage above passes.
- A retained schema-v1 database from an older native test host requires a clean supported installation; clearer failure propagation is tracked in [UNIV-3183](https://universe.basis.dev/issue/UNIV-3183).
- The installed Epic distribution lacks some iOS-simulator third-party link inputs. Swift bridge simulator tests pass; a full Unreal simulator player is unrun.
- Store-managed deferred approval and final-candidate RTDN persistence are unrun. Store submission, distribution signing and public archive publication are separate release steps.

## Shipping artifacts

The Android Shipping build passed from `2ebaa68`; the iOS Shipping build passed from `ab05005`, with unchanged iOS plugin source and identical prepared physical framework payloads. Comparing actual Development and Shipping binaries verifies removal of the Lab's unattended runner and external-controller probe markers. The iOS Shipping host also omits `NUXIE_UNREAL_API_ENDPOINT`. All four native Apple bundle resources, including privacy and timezone data, match the prepared framework byte-for-byte; strict signing passes.

The Android Shipping APK retains both native runtime libraries and timezone data. Signature verification, ZIP 16 KiB alignment, and load-segment alignment/congruence pass for all seven Shipping ELF libraries. These builds use local development signing/non-distribution packaging; they do not claim store submission validation.

- Android Shipping APK SHA-256: `12481e96ef8f056638cbe04835ad9a047bc0bbeb036a4af79239d1cace18bde8`.
- iOS Shipping executable SHA-256: `53fb6c0d676543405ccee39160b501e91f2a832a989bb2e8e023678f619ced77`.

Both use `RunUAT.sh BuildCookRun` with `-clientconfig=Shipping -build -cook -stage -pak -package -map=Lab+LabSecond -AdditionalCookerOptions=-nowrite -UbtArgs=-NoUBA`, plus `-platform=Android -cookflavor=ASTC` or `-platform=IOS`.

## Parent integration evidence

The parent readiness gate runs `pnpm run pr:ready`: commerce documentation contract and exact Swift/provider compilation, root lint and typecheck, connector and durable-object unit tests, and readiness-policy tests. SDK gates pass separately at the native pins above. [Parent PR #6512](https://github.com/nuxieai/nuxie-dev/pull/6512) records the final pointer and its committed-tree receipt. No required local gate is intentionally skipped.
