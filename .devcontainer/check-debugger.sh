#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
case "${MOVING2ARM_DEV_ARCH:-}" in
    amd64|arm64) ;;
    *) echo "Run this check in the project's Dev Container image." >&2; exit 1 ;;
esac
cmake --build --preset dev-debug --target calculation
gdb --nx --batch --quiet --return-child-result \
    -ex 'set disable-randomization off' \
    -ex 'break calculate' \
    -ex run \
    -ex 'python assert int(gdb.parse_and_eval("n")) == 4' \
    -ex 'python assert float(gdb.parse_and_eval("a[0]")) == 1.0' \
    -ex 'python assert float(gdb.parse_and_eval("b[1]")) == 2.0' \
    -ex continue \
    --args "build/devcontainer-${MOVING2ARM_DEV_ARCH}/dev-debug/simple_port/calculation"
