# Public API

Include `NuxieSubsystem.h` and obtain the subsystem from your `UGameInstance`. All methods and delegates are game-thread APIs.

| Method | Completion | Inputs |
| --- | --- | --- |
| Configure | FNuxieCompletion | FNuxieOptions |
| Shutdown | FNuxieCompletion | None |
| Identify | FNuxieCompletion | CustomerId, FNuxieIdentityOptions |
| Reset | FNuxieCompletion | None |
| GetIdentity | FNuxieIdentityCompletion | None |
| CheckFeature | FNuxieFeatureCompletion | FeatureId, FNuxieFeatureQuery |
| ConsumeFeature | FNuxieConsumeCompletion | FeatureId, FNuxieFeatureCommand |
| Trigger | FNuxieCompletion | EventName, FNuxieProperties |
| Dismiss | FNuxieCompletion | None |
| SetLocale | FNuxieCompletion | Locale; empty restores device locale |

Each async Blueprint node has typed Success and Failure delegates. The C++ completion types use Unreal `TDelegate` and support `CreateWeakLambda`.

Synchronous reads: `GetStatus`, `GetFeatureSnapshot`, `GetFeatureState`.

Events: `OnStatusChanged`, `OnIdentityChanged`, `OnFeaturesChanged`, `OnActivity`, `OnAppAction`, `OnError`.

Input values are separate from native scalar results. `FNuxieProperties` supports nested portable JSON. Native activity and app-action property maps use `FNuxieScalar`, whose tagged Integer field is a full int64.

The canonical fields, presence flags, and Blueprint metadata are in `Source/Nuxie/Public/NuxieTypes.h`. The [README](../README.md) explains their lifecycle and gameplay semantics.

`FNuxieProperties::WithString`, `WithBool`, `WithInteger`, `WithNumber`, `WithNull`, `WithObject`, and `WithArray` mutate only on success. Each returns `bool` and writes `FString& Error`. Blueprint uses `MakeProperties`, typed value constructors (including fallible `IntegerValue` and `NumberValue`), then `WithProperty`. Integer inputs are checked before conversion and must lie in ±9,007,199,254,740,991. `TryParse` accepts an object; `FNuxieJsonValue::TryParse` also accepts a scalar or array.

At most 64 ordinary requests can be admitted concurrently. Checkout completions and lifecycle cleanup have reserved capacity. Excess ordinary requests report `Overloaded`; a rejected identity change preserves the current Ready session. `OnError(Overloaded)` also reports an observational event buffer overflow. After successful shutdown, old native reservations are retired and late replies cannot enter the replacement session.
