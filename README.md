# Nuxie for Unreal Engine

Native Experiences, purchases, and feature access for mobile games. Use Blueprint, C++, or both.

Configure once. Keep the client across maps. Observe access as it changes, spend with an authoritative receipt, and let published Experiences call back into your game.

**0.3 is a complete replacement of the old API.** There are no compatibility aliases. One runtime module, one game-instance subsystem, typed asynchronous results, and the same native SDKs used by Nuxie's iOS and Android integrations.

## Start here

| Surface | What it does |
| --- | --- |
| iOS and Android player | Runs the native Nuxie SDK, Experiences, feature queries, consumption, and billing |
| Blueprint | Typed async nodes, observable feature snapshots, structured values, and asynchronous purchase requests |
| C++ | `UNuxieSubsystem` with typed completion delegates |
| Desktop editor | Loads the plugin and compiles graphs; mobile commands return `UnsupportedPlatform` |
| Desktop player or dedicated server | No mobile runtime and no simulated access grants |

The target engine is **UE 5.8**, with builds being qualified on **5.8.2**. This branch is not a qualified release archive yet. See [validation evidence](docs/testing.md) for the distinction between passing checks and pending device qualification.

### Install the plugin

Place the prepared plugin in your project's `Plugins/Nuxie` directory:

```text
MyGame/
  MyGame.uproject
  Plugins/
    Nuxie/
      Nuxie.uplugin
      Source/
      ThirdParty/
      NATIVE-PINS.json
```

Enable **Nuxie**, restart the editor, and enter your client public keys under **Project Settings → Nuxie**. Only the running platform's key is required. These are public SDK keys, never backend secret keys.

Blueprint gameplay does not require a C++ gameplay class. Packaging still requires Unreal's iOS/Android components and the corresponding platform tools. A source checkout also needs the native preparation steps in [building the plugin](docs/getting-started.md). Do not hand-edit generated Gradle, GameActivity, or Xcode files.

## Your first Blueprint graph

Call from your GameInstance initialization event:

```text
Get Project Options
  → Configure (Nuxie)
      Success → Identify (Nuxie), using your signed-in customer ID
      Failure → handle Error.Code and display Error.Message
```

Leave anonymous players anonymous until your game has a stable customer ID. Configuration success means the native client is established; it does not mean feature authority has finished loading.

In a widget:

```text
Construct → Observe Features
  Changed → Snapshot.Kind
    Unknown      → show loading or unavailable
    Reconciling  → show access being reconciled
    Ready        → inspect the feature in Snapshot.All
Destruct → Observer.Cancel
```

`Observe Features` binds before delivering the current snapshot. If you use the subsystem's delegates directly, bind `OnFeaturesChanged` before calling `GetFeatureSnapshot`.

**Unknown is not denial. Missing is not zero.** `GetFeatureState` returns both readiness and `bHasAccess`; only inspect `Access` when that flag is true. `Access.bHasBalance` distinguishes an absent balance from zero. `bUnlimited` is explicit.

The subsystem belongs to your GameInstance and survives map travel. No SDK Actor or custom GameInstance subclass is required.

## Spend a metered feature safely

A feature query can guide the UI. A consumption receipt decides whether the backend accepted a metered action.

1. Save a pending action containing the customer, feature, entity, quantity, and operation ID.
2. Call `ConsumeFeature` with that saved command.
3. On a successful response, inspect `Receipt.bAccepted`.
4. Apply an accepted gameplay effect once, using its operation ID.
5. After a timeout or uncertain network result, retry the **same saved command**.

```text
Save pending action
  → Consume Feature (Nuxie)
      FeatureId: energy
      Command:
        OperationId: saved operation ID
        Quantity: 1
        EntityId: character-a
      Success:
        confirm Receipt.CustomerId belongs to the pending action
        Accepted → apply that action once
        Denied   → show Receipt.Code
      Failure:
        retain the pending action and offer a same-ID retry
```

A successful API call can return a denied receipt. An accepted last-unit receipt can have **zero remaining balance** and still authorize that action. Do not require both acceptance and a positive remaining balance.

Nuxie makes the debit idempotent. Your game owns idempotency for the reward or action. `bIdempotentReplay` tells you that the backend returned a previously recorded outcome; it is not permission to apply gameplay twice.

`Quantity` and `RequiredBalance` are positive Integer64 values up to **2^53−1**. Returned balances are Double values and can be fractional or absent. Generation and revision are exact decimal strings, so unsigned native counters never pass through a Blueprint signed integer or a JSON double.

### The same operation in C++

Add `"Nuxie"` to your game module's dependencies. The Lab contains the complete save-before-send flow; this shows the public call shape:

```cpp
#include "NuxieSubsystem.h"

UNuxieSubsystem* Client = GetGameInstance()->GetSubsystem<UNuxieSubsystem>();
FNuxieFeatureCommand Command;
Command.OperationId = SavedOperationId; // Persist before sending.
Command.Quantity = 1;
Command.EntityId = TEXT("character-a");

Client->ConsumeFeature(TEXT("energy"), Command,
    FNuxieConsumeCompletion::CreateWeakLambda(this,
        [this](const TNuxieResult<FNuxieUsageReceipt>& Result)
        {
            if (!Result.IsSuccess())
            {
                // Keep the saved command for a same-ID retry.
                return;
            }
            const FNuxieUsageReceipt& Receipt = Result.GetValue();
            // Confirm customer ownership before applying the saved action.
            // Receipt.bAccepted is authoritative, including at zero balance.
        }));
```

Every completion runs on Unreal's game thread. Weak delegates stop invoking a destroyed UObject; destroying that object does not undo a debit already accepted by the backend.

## Identity and access

| Operation | Use it for |
| --- | --- |
| `GetIdentity` | A coherent customer ID, anonymous ID, and identified flag |
| `Identify` | Sign in with optional properties and properties-set-once |
| `Reset` | Sign out and rotate the anonymous identity |
| `GetFeatureSnapshot` / `GetFeatureState` | Read the current global feature publication without a network request |
| `CheckFeature` | Query a feature with explicit entity, required balance, and cache/remote policy |
| `ConsumeFeature` | Submit a durable operation and receive the backend's typed outcome |

Set `Query.EntityId` for per-character or other entity-scoped access. Set `Query.Policy = Remote` when the query must reach native remote authority. An entity query never overwrites the global UI snapshot.

Await identity changes in sequence. Queries during a transition fail with `LifecycleBusy`; replies admitted under a previous identity fail with `IdentityChanged`. Already-admitted consumption can still return its original customer's receipt, which remains useful for reconciling that saved action. It must not affect another customer's gameplay.

A feature snapshot's customer and native generation are established together from an identity response. Old generations and out-of-order revisions are discarded.

## Experiences and app actions

Author and publish an Experience that starts on your event, then trigger that exact event:

```text
Shop button → Trigger (Nuxie)
  EventName: shop_opened
  Properties: { source: inventory }

OnAppAction → switch Action.Name
  continue_game → resume the intended game action
```

`Trigger` acknowledges native event acceptance. Eligibility and Journey state decide whether an Experience appears. Observe `OnActivity` for runtime activity and `OnAppAction` for authored actions delegated to your game. `Dismiss` requests dismissal.

Native Experiences appear above Unreal's viewport. Your game owns pause, input, and audio policy. The bridge dispatcher uses the core ticker and continues when gameplay is paused; OS suspension can delay callbacks.

`FNuxieActivity.Properties` and `FNuxieAppAction.Payload` contain typed `FNuxieScalar` values. Branch on `Kind` to read String, Integer64, Double, or Boolean. Native integer properties retain their full signed 64-bit range. `bHasPayload` distinguishes an absent app-action payload from an empty map.

### Structured input values

`FNuxieProperties` represents an immutable-by-value object. Blueprint provides `StringValue`, `BoolValue`, `NumberValue`, `NullValue`, `ObjectValue`, `ArrayValue`, and `WithProperty`. Use `ParseProperties` for existing JSON data and check its boolean result and error output.

```cpp
FNuxieProperties Properties;
FString Error;
if (FNuxieProperties::TryParse(
        TEXT("{\"source\":\"inventory\",\"items\":[1,true,null]}"),
        Properties, Error))
{
    Client->Trigger(TEXT("shop_opened"), Properties, Completion);
}
```

Nested arrays and objects are supported to depth 32. Inputs reject non-finite numbers and integers outside the portable exact range. Failed parsing preserves the previous value. Event names beginning with `$` are reserved for SDK telemetry.

## Purchases: native by default

The default billing mode uses Nuxie's native store integration. Observe feature reconciliation after purchase or restore. Starting checkout, or reporting `Purchased`, does not itself grant access.

If your game already owns billing:

1. Create a controller that implements `INuxiePurchaseController` or the Blueprint **Nuxie Purchase Controller** interface.
2. Set `BillingMode = External` and `ExternalController` before configuring.
3. Implement the asynchronous `BeginPurchase` and `BeginRestore` events.

```text
BeginPurchase(Request)
  → start your billing provider's checkout with Request.Product
  → Request.TryComplete(Purchased / Cancelled / Pending / Failed, Message)

BeginRestore(Request)
  → start your provider's restore flow
  → Request.TryComplete(Restored / NoPurchases / Failed, Message)
```

The subsystem retains the controller across maps and retains pending request objects. A request permits one completion, expires after its native deadline, and becomes inactive at identity invalidation or shutdown. `TryComplete` returns false for an inactive request. Internal correlation IDs are private.

Honor the selected product, Android base plan, purchase option, and offer supplied in `Request.Product`. Your external billing integration finishes or acknowledges transactions. Avoid running native Nuxie checkout and another billing owner for the same transaction.

## Errors and lifecycle

Connect every async **Failure** pin. Branch on `Error.Code`; messages are diagnostic text.

| Error | Meaning |
| --- | --- |
| `UnsupportedPlatform` | The call needs an iOS or Android player |
| `InvalidArgument` | Invalid key, identifier, quantity, controller, or structured value |
| `NotConfigured` | Wait for configuration to succeed |
| `AlreadyConfigured` | Shutdown before changing configuration |
| `LifecycleBusy` | Finish the current identity or shutdown transition |
| `IdentityChanged` | A pending result belongs to a previous customer |
| `SessionInUse` | Another game instance owns the process-wide native SDK |
| `SDKShutdown` | Shutdown invalidated the operation |
| `OperationTimeout` | No reply within the active dispatch timeout; preserve durable operation IDs |
| `InvalidResponse` / `IncompatibleBridge` | Inspect packaging, contract version, and native pins |
| `NativeError` | Inspect `NativeCode` and the diagnostic message |

Equivalent concurrent configuration shares setup. To change keys, options, or controllers, await `Shutdown` and configure again. A failed cleanup retains the native lease until shutdown succeeds, preventing another instance from inheriting uncertain customer state.

Ordinary requests have a 90-second active dispatch timeout. Gameplay pause does not stop it; background suspension is excluded. A timeout or shutdown is not rollback. Unsolicited bridge errors use `OnError`; operation errors go to that operation's completion.

`SetLocale` overrides locale; an empty string restores device locale.

## Explore the examples

- **[NuxieLab](Examples/NuxieLab)** — a C++ UMG app with two maps, a real Blueprint getting-started graph, editable development settings, entity queries, saved consumption and replay, locale, identity, and Experience/App Action activity.
- **[BlueprintOnly](Examples/BlueprintOnly)** — a consumer with no gameplay C++ module. Install a prepared plugin and open its getting-started map.

The Lab's **Validate debit and replay** action uses a finite metered grant to check that an original operation plus a same-ID replay produces exactly one debit and one saved action. Use a disposable development customer, not production balances. Keep the feature and entity exclusive to that validation run.

Settings entered into the Lab are local to that run; client public keys can also be provided through Unreal project settings. Saved pending operations live in the platform's SaveGame storage. Do not retry a saved operation under a different customer.

For local endpoints, use your backend checkout's `pnpm run dev:print`. The iOS debug bridge reads `NUXIE_UNREAL_API_ENDPOINT`; the Android debuggable app reads the same-named launch intent extra. Release builds use the configured Nuxie environment. Never commit local credentials or machine-specific endpoints.

## Contribute and validate

See [build instructions](docs/getting-started.md), [architecture](docs/architecture.md), [API reference](docs/api-reference.md), and [testing](docs/testing.md).

Native revisions and the bridge contract are pinned in `NATIVE-PINS.json`. Build artifacts must come from those revisions. Source checks, editor tests, and native compilation are useful evidence; packaged device runs and store sandbox checkout remain separate qualification steps.
