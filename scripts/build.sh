#!/usr/bin/env bash
# Configure, build and test LibVIO; install it when a prefix is given.
#
#   scripts/build.sh                 # build and test in ./build
#   scripts/build.sh --prefix DIR    # ... then install into DIR
#
# BUILD_DIR overrides the build directory.
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
build=${BUILD_DIR:-$root/build}
prefix=

while [[ $# -gt 0 ]]; do
    case $1 in
        --prefix) prefix=$2; shift 2 ;;
        -h|--help) sed -n '2,7p' "$0"; exit 0 ;;
        *) printf 'unknown option: %s\n' "$1" >&2; exit 2 ;;
    esac
done

cmake -S "$root" -B "$build"
cmake --build "$build" -j "$(nproc)"
ctest --test-dir "$build" --output-on-failure
if [[ -n $prefix ]]; then
    cmake --install "$build" --prefix "$prefix"
fi
