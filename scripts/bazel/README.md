# Bazel worktree caches

The Bazel workspace imports `.bazel-cache.bazelrc`, matching `nuxie-runtime`
and all Nuxie SDKs. Action products, dependency downloads, and fetched
repository trees share `~/.cache/nuxie/bazel` across Git worktrees.

Bazel derives a separate output base from each checkout's path. Its `bazel-*`
links and build outputs remain specific to that checkout, while matching actions
can be restored from the shared cache.

Use `scripts/bazel/bazel.sh` to run the pinned Bazel version. Set an absolute
`NUXIE_BAZEL_CACHE_DIR` to relocate only the reusable caches. A shared
`NUXIE_BAZEL_OUTPUT_USER_ROOT` still contains separate output bases for each
checkout; an explicit `--output_base` must be unique to the checkout.

Verify the cache override without compiling native sources:

```sh
python3 -B -m unittest discover -s scripts/bazel -p 'test_cache.py'
```

This workspace establishes the cache policy for the SDK's Bazel migration.
Existing host-language build and qualification commands remain available.

## Direct portable and mobile bridge builds

```sh
python3 scripts/bazel/sdk.py test
python3 scripts/bazel/sdk.py test-android
python3 scripts/prepare-android.py
NUXIE_IOS_SIMULATOR_ID=<available-simulator-id> python3 scripts/bazel/sdk.py test-ios
bash ThirdParty/IOS/scripts/build-framework.sh
python3 scripts/check.py
```

The portable request-ledger test compiles directly as a C++20 `cc_test` with
address/undefined-behavior sanitizers. Android bridge Kotlin/Java and Swift bridge
sources compile in direct `//:android_bridge` / `//:ios_bridge` targets. Their
existing JUnit/Robolectric and XCTest cases remain the test inputs. The direct
Apple XCFramework target supplies all three required device/simulator
architectures; staging retains Unreal's embedded-framework ZIP root and the
native resource bundle. Native receipts inventory the actual Bazel build graph,
preparation scripts, bridge sources and resulting customer artifacts.

Native SDK preparation consumes exact `NATIVE-PINS.json` source revisions. It
can reuse absolute `NUXIE_IOS_ARTIFACTS` / `NUXIE_ANDROID_ARTIFACTS` receipt paths
from a parent checkout, or prepare standalone `.native/` products. Both paths
verify revision, committed source, safe paths and SHA-256. Shared caches contain
compiler actions and dependencies; `ThirdParty/*/lib`, Maven files, Unreal
intermediates, distribution directories and output bases remain checkout-local.

The owning engine check still requires installed UE 5.8 and runs UBT, UHT,
Unreal automation and UAT packaging with that engine's toolchain. Portable and
native bridge Buildkite steps use the same local Bazel entry points. Native CI
requires Xcode, an explicit `NUXIE_IOS_SIMULATOR_ID`, Java 21, Android SDK
36/build-tools 36.0.0 and the native SDK's pinned NDK. Engine/editor/player
qualification remains an independent installed-engine gate.
