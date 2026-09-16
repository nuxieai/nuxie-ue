# Nuxie examples

## Mobile SDK Lab

Open `NuxieLab/NuxieLab.uproject` after preparing the native artifacts and running `python3 scripts/link-example.py` from the SDK root. The Lab uses the public SDK API, real Blueprint assets, and two maps. Enter the platform public key, customer, Feature ID, and two entity IDs. Configure, identify, then run **Validate debit and replay** against finite development grants. The validator saves its operation before sending, checks one debit and unchanged comparison entity, and records `Saved/NuxieLab/validation.json`. Retrying a pending operation always retains its original customer and operation ID.

The native store integration is the default. To inspect the external controller lifecycle, shut down first and choose **Configure external billing harness**. The `UNuxieLabBilling` game-instance subsystem retains the typed purchase/restore request across map travel. Trigger an Experience containing checkout, then cancel the retained purchase or fail restore from the Lab controls. Duplicate/expired requests return false. Leaving a request pending exercises its native deadline. This harness deliberately has no store provider and does not report Purchased or Restored; sandbox success requires a real provider integration.

The Mac editor can inspect UI, assets, and typed Blueprint pins. Native commands return UnsupportedPlatform there. Package the Lab for iOS/Android to validate the backend and native Experience. See [validation evidence](../docs/testing.md) for the device and store matrix.

## Blueprint-only consumer

`BlueprintOnly/BlueprintOnly.uproject` has no Source directory or gameplay C++ module. Copy an output from `scripts/package.py` into its ignored `Plugins/Nuxie` directory. Open `Content/BP_NuxieGettingStarted`: its actual async graph reads project options, configures Nuxie, gets coherent identity, and branches on typed success/failure. Supply public keys in Project Settings → Plugins → Nuxie before mobile packaging. The prepared archive has been packaged into fresh iOS and Android consumers. Both ran this graph’s configured identity-success branch against the local backend, rendered a published Experience, and returned to Unreal after authored dismissal. See [validation evidence](../docs/testing.md) for the tested revisions and limits.

Both examples keep generated products, installed plugins, reports, and local settings ignored. Do not commit private credentials or machine-specific paths.

## Unattended development validation

A Development Lab build can run the same public API workflow automatically. Put a local `auto.json` under the app's `Saved/NuxieLab` directory:

```json
{
  "publicKey": "your-platform-development-public-key",
  "customerId": "your-development-customer",
  "featureId": "energy",
  "entityId": "character-a",
  "comparisonEntityId": "character-b"
}
```

Give both entities finite grants in your development backend. On startup, the Lab configures, identifies, queries both entities remotely, consumes one unit, retries the original operation, and checks that only the first entity lost one unit. Read `Saved/NuxieLab/validation.json` for the verdict and operation ID. Each launch with this file present starts a new validation operation; remove it when finished. A pending saved operation must be resolved before another run can start. The runner is absent from Shipping builds. Set `"configureOnly": true` to configure and identify without starting a debit, and optionally set `"triggerEvent"` to populate the Experience trigger control. Remove `auto.json` before map-travel checks so the recreated widget does not start another configuration. Development builds also append actual Lab callbacks and their map, pause, and presentation state to `Saved/NuxieLab/observations.jsonl`; this records observed behavior without injecting SDK callbacks.

Android logs its private settings path at startup: `files/NuxieLab/auto.json` inside the app sandbox. For a debuggable Lab, create the directory with `adb shell run-as ai.nuxie.unreal.lab mkdir -p files/NuxieLab`, then write settings with `adb shell run-as ai.nuxie.unreal.lab tee files/NuxieLab/auto.json < auto.json`. Read reports with `adb exec-out run-as ai.nuxie.unreal.lab cat files/NuxieLab/validation.json`. Substitute your configured application ID when it differs from the example default. The example's Development-only network policy permits HTTP solely to `localhost`, `127.0.0.1`, and Android's `10.0.2.2` host bridge. Forward the SDK ingest port from `pnpm run dev:print` with `adb reverse`, then launch `com.epicgames.unreal.GameActivity` with the `NUXIE_UNREAL_API_ENDPOINT` string extra. These settings do not change the SDK's production network policy.

The Lab’s presentation subsystem retains pause ownership across map travel. It pauses for native Experience activity and restores only a pause it introduced; an already-paused game stays paused. The UI keeps its UI-only input policy while the native overlay owns touch input. Native activity identifies Experience/version/Journey references, not unique presentation instances, so the Lab does not infer instance IDs.

Set `"lifecycle": true` in a Development runner file to validate locale override/reset, anonymous reset and reidentification, denied over-balance usage with exact replay and unchanged balance, shutdown/reconfiguration, and identity retention across travel to `LabSecond`. This requires the same finite development entity grant and no concurrent consumers. Read `Saved/NuxieLab/lifecycle.json` for its verdict. The runner persists a one-time map-travel continuation and then returns to configure-only mode. A crash before travel is reported as an invalid continuation on the next cold launch, not a passing map-travel check. These API checks do not replace native Experience interaction or store sandbox qualification.
