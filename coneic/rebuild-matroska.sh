#!/usr/bin/env bash
# Rebuild pinned EBML/Matroska with the active compiler to avoid C++ ABI drift.
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
prefix="$root/contrib/x86_64-w64-mingw32"
mkdir -p "$root/.coneic-tools/rebuilt-deps"
cd "$root/.coneic-tools/rebuilt-deps"
curl -fL --retry 3 -o libebml-1.4.6.tar.xz https://dl.matroska.org/downloads/libebml/libebml-1.4.6.tar.xz
curl -fL --retry 3 -o libmatroska-1.7.2.tar.xz https://dl.matroska.org/downloads/libmatroska/libmatroska-1.7.2.tar.xz
sha512sum -c "$root/contrib/src/ebml/SHA512SUMS"
sha512sum -c "$root/contrib/src/matroska/SHA512SUMS"
tar -xf libebml-1.4.6.tar.xz
tar -xf libmatroska-1.7.2.tar.xz
for package in libebml-1.4.6 libmatroska-1.7.2; do
  cmake -S "$package" -B "$package/build" -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DCMAKE_FIND_ROOT_PATH="$prefix" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF -DCMAKE_CXX_FLAGS=-D__MSVCRT_VERSION__=0x700
  cmake --build "$package/build" -j"${CONEIC_JOBS:-4}"
  cmake --install "$package/build"
done
