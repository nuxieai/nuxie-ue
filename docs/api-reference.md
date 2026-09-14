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
