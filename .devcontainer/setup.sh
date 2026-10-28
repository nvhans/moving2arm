#!/bin/sh
set -eu
cd "$(dirname "$0")/.."

case "${MOVING2ARM_DEV_ARCH:-}" in
    amd64|arm64) ;;
    *) echo "Run this setup in the project's Dev Container image." >&2; exit 1 ;;
esac

if [ ! -f multi-platform-dev/simde/simde/x86/sse.h ] ||
   [ ! -f multi-platform-dev/porting-advisor-for-graviton/src/porting-advisor.py ]; then
    git submodule update --init --recursive
fi

cmake --preset dev-debug
cmake --build --preset dev-debug --target calculation
printf '\nReady: use the Porting tasks to build/test, or F5 to debug the scalar example.\n'
