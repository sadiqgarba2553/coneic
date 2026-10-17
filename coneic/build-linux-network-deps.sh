#!/usr/bin/env bash
# Pinned VLC contrib libraries missing from Ubuntu 24.04 repositories.
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
prefix="${CONEIC_NETWORK_PREFIX:-$root/build-linux-deps}"
jobs="${CONEIC_JOBS:-4}"
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || { echo 'CONEIC_JOBS must be a positive integer' >&2; exit 1; }
[[ "$(uname -m)" == x86_64 ]] || { echo 'This native target is x86_64' >&2; exit 1; }
mkdir -p "$prefix" "$root/contrib/coneic-linux"
prefix="$(cd "$prefix" && pwd)"
cd "$root/contrib/coneic-linux"
../bootstrap --build=x86_64-linux-gnu --host=x86_64-linux-gnu \
    --prefix="$prefix" --disable-all --enable-live555 --enable-microdns
make -j"$jobs" .live555 .microdns
mkdir -p "$prefix/share/doc/coneic-network-deps"
install -m644 live555/COPYING "$prefix/share/doc/coneic-network-deps/COPYING.live555"
install -m644 live555/COPYING.LESSER "$prefix/share/doc/coneic-network-deps/COPYING.LESSER.live555"
install -m644 microdns/COPYING "$prefix/share/doc/coneic-network-deps/COPYING.microdns"
