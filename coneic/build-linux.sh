#!/usr/bin/env bash
# Native Linux build using the distribution's development libraries.
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${CONEIC_BUILD_DIR:-$root/build-linux}"
prefix="${CONEIC_PREFIX:-/opt/coneic}"
network_prefix="${CONEIC_NETWORK_PREFIX:-$root/build-linux-deps}"
jobs="${CONEIC_JOBS:-4}"
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || { echo 'CONEIC_JOBS must be a positive integer' >&2; exit 1; }
if ! git -C "$root" rev-parse --git-dir >/dev/null 2>&1 && [[ ! -s "$root/src/revision.txt" ]]; then
    printf '%s\n' 'coneic-0.2.1-alpha-source' > "$root/src/revision.txt"
fi
if [[ ! -x "$root/configure" ]]; then
    (cd "$root" && ./bootstrap)
fi
if [[ -d "$network_prefix/lib/pkgconfig" ]]; then
    export PKG_CONFIG_PATH="$network_prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
fi
mkdir -p "$build"
cd "$build"
"$root/configure" --prefix="$prefix" --enable-qt --enable-vpx --disable-debug "$@"
make -j"$jobs"
