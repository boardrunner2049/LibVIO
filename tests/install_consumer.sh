#!/usr/bin/env bash
# Install LibVIO into a temporary prefix and build a small program against
# it, once with pkg-config and once with CMake's find_package.
set -euo pipefail

build_dir=$1
source_dir=$(cd "$(dirname "$0")" && pwd)/consumer
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

cmake --install "$build_dir" --prefix "$work/prefix" >/dev/null
libdir=$(find "$work/prefix" -name libvio.so -printf '%h\n' | head -1)

export PKG_CONFIG_PATH="$libdir/pkgconfig"
cc -o "$work/consumer_pc" "$source_dir/main.c" $(pkg-config --cflags --libs vio) \
    -Wl,-rpath,"$libdir"
"$work/consumer_pc"

contract=$(pkg-config --variable=contractdir vio)
for f in api_reference.md api_summary.md host_api.md VERSION; do
    [[ -f $contract/$f ]] || { echo "missing contract file: $contract/$f" >&2; exit 1; }
done

cmake -S "$source_dir" -B "$work/cmake_build" -DCMAKE_PREFIX_PATH="$work/prefix" \
    -DCMAKE_BUILD_RPATH="$libdir" >/dev/null
cmake --build "$work/cmake_build" >/dev/null
"$work/cmake_build/consumer"
