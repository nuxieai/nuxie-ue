#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../ThirdParty/Android"
./gradlew :bridge:testDebugUnitTest :bridge:assembleRelease
