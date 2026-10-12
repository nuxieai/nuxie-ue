# Unreal runner lanes and shared cache

The default pipeline composes its existing portable contract and combined native
bridge recipes through `scripts/ci/pipeline.py`. `NUXIE_SDK_SPLIT_LANES=1` requires
`NUXIE_CI_RUNNER_POLICY_VERSION=2` and separates those same command seams:

- Linux runs portable contracts, Android bridge tests and the Android preparation
  wrapper. The wrapper retains the pinned SDK Maven publication, bridge AAR and
  owning receipt.
- Mac runs the iOS test through its simulator lease wrapper with minimum OS 17.0,
  then the framework packaging wrapper. Device and simulator embedded-framework
  ZIPs, native resource bundles and the owning receipt remain required.

Each lane prepares only its own native platform in its private checkout; there
is no cross-job native product transfer. Supplied native products continue to
validate the original producer's source pin, configuration and complete
inventory before consumption. An invalid receipt fails instead of silently
rebuilding another platform. The canonical `python3 scripts/check.py` still
includes licensed UE 5.8 editor and full consumer qualification.

Policy v2 uses `linux-14g-v2` and `mac-64g-v2` capabilities with `linux-portable`,
`linux-native` and `macos-apple` profiles. Physical-host concurrency groups use
an eager limit of one. The installed root-controlled helper admits job/cache
ownership and supplies resource ceilings to each Bazel invocation. Linux
requires provisioned Java 21, Android API 36, build tools 36.0.0, NDK
29.0.14206865 and CMake 3.22.1; the lane validates those existing tools instead
of changing shared installation state. The owning post-artifact hook invokes
installed cleanup after evidence uploads and preserves uncertain/active output.

Remote cache transport remains off by default and on bootstrap. Independently
qualified readers may use `NUXIE_CI_BAZEL_REMOTE_CACHE_MODE=read` and an exact
`NUXIE_CI_BAZEL_CACHE_PROFILES` allowlist. The renderer selects only matching
v2 profiles with explicit action eligibility, an approved SHA-256 toolchain
domain, endpoint, public leaf/pin paths and a private reader credential path.
The helper validates the actual public pin and endpoint-bound credential before
requests. SDK writers stay disabled until separate source closure and writer
policy qualification.

[UNIV-4312](https://universe.basis.dev/issue/UNIV-4312) owns this consolidation.
Activation follows affected-host drain and complete native/consumer canaries.
CI work uses separate current-main checkouts while another migration continues;
reconcile overlapping landed changes through a normal rebase without changing
that task's checkout, native products or pin updates.
