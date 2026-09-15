# Architecture

`UNuxieSubsystem` is the public game-instance boundary. It exposes typed operations and publications, and owns the external billing controller. It does not implement entitlement or Journey policy.

`FNuxieSession` owns the process-wide native lease, request correlation, bounded core-ticker dispatch, identity fencing, snapshot ordering, and teardown. A session keeps native ownership until shutdown acknowledges cleanup. One game's identity cannot silently become another game instance's identity.

`NuxieWire` validates native result shapes and preserves nullable balances, exact decimal unsigned counters, and tagged signed integer activity values. `RequestLedger` is a portable production component with compiled behavior tests for timeout, identity admission, and exactly-once completion.

`INuxieNativeTransport` has only contract-version, submit, and poll operations. iOS uses exported C functions and owned UTF-8 strings. Android uses compiled static Kotlin entry points, the Unreal application class loader, UTF-16 JNI strings, and scoped references. Platform headers do not escape the private implementation.

Swift and Kotlin runtimes call the pinned native SDK directly. They serialize native snapshot subscription, customer changes, Experience activity, and asynchronous external purchase requests. Backend consumption receipts remain authoritative. Entity query results never replace the global snapshot.

Blueprint async actions adapt the same subsystem methods. Observers bind before delivering current state and expose cancellation. `NuxieEditor` provides an explicit example-asset generation commandlet; no editor dependency is linked into a mobile player.

The `NATIVE-PINS.json` contract and revisions govern both preparation and packaging. Prepared archives carry their native closure rather than resolving a floating SDK version at consumer build time.

The native outbox retains up to 256 ordinary events plus a coalesced latest feature snapshot and overflow notice. Replies and checkout requests use separate queues and are polled first. Admission reserves 64 ordinary operations, 64 additional checkout-completion slots, and four lifecycle slots; native checkout itself retains at most 64 pending requests. Consumed entries release their storage even during a continuous event stream. Successful shutdown retires old reservations; late replies are discarded by request ID. C++ polling stops immediately when native ownership changes, including reconfiguration from a shutdown callback.

Every bridge event carries session and native feature-generation provenance captured at bridge entry. The shared Unreal fence rejects stale envelopes and events during identity transitions. This does not repair missing provenance before native callback delivery: [UNIV-3175](https://universe.basis.dev/issue/UNIV-3175) tracks the remaining native activity contract gap. Do not claim complete customer-safe activity qualification until that issue is resolved.
