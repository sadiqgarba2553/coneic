#!/usr/bin/env bash
# Run after the native Linux build, using its real libvlccore block allocator.
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build="${CONEIC_BUILD_DIR:-$root/build-linux}"
out="${1:-$build/coneic-tests}"
mkdir -p "$out"
cc -DHAVE_CONFIG_H -I"$build" -I"$root/include" -I"$root/modules/packetizer" \
    "$root/coneic/tests/hxxx_metadata_probe.c" "$root/modules/packetizer/hxxx_nal.c" \
    -L"$build/src/.libs" -Wl,-rpath,"$build/src/.libs" -lvlccore \
    -o "$out/hxxx-metadata-probe"
"$out/hxxx-metadata-probe" | tee "$out/hxxx-metadata-results.json"
