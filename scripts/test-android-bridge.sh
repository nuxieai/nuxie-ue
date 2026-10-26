#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
if [[ "${_NUXIE_POOL_NESTED:-0}" != 1 && "${NUXIE_HOMELAB:-1}" != 0 && "${NUXIE_EXECUTION:-}" != local && -f "$HOME/.config/nuxie/homelab-cluster.json" ]]; then
  exec python3 "${SCRIPT_DIR}/pool.py" test-android-bridge "$@"
fi
cd "$(dirname "$0")/../ThirdParty/Android"
./gradlew :bridge:testDebugUnitTest :bridge:assembleRelease
