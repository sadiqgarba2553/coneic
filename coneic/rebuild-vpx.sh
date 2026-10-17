#!/usr/bin/env bash
# Rebuild the pinned codec dependency with the active compiler.
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
prefix="$root/contrib/x86_64-w64-mingw32"
mkdir -p "$root/.coneic-tools/vpx"
cd "$root/.coneic-tools/vpx"
curl -fL --retry 3 -o libvpx-1.16.0.tar.gz https://github.com/webmproject/libvpx/archive/v1.16.0.tar.gz
sha512sum -c "$root/contrib/src/vpx/SHA512SUMS"
tar -xf libvpx-1.16.0.tar.gz
cd libvpx-1.16.0
for item in 0001-Disable-forcing-flags-for-Apple-targets.patch libvpx-ios.patch \
  0001-do-not-use-CLOCK_MONOTONIC_RAW-in-older-iOS-macOS.patch libvpx-remove-bitcode.patch \
  0001-fix-compilation-with-Apple-Clang-13.patch 0001-force-detection-of-pthread-on-Windows.patch; do
  patch -p1 -i "$root/contrib/src/vpx/$item"
done
sed -i.orig 's,++17,++14,g' configure
mkdir -p ../build
cd ../build
CROSS=x86_64-w64-mingw32- LDFLAGS="${LDFLAGS:-} -L$prefix/lib" \
 ../libvpx-1.16.0/configure --disable-docs --disable-examples --disable-unit-tests \
 --disable-install-bins --disable-install-docs --disable-dependency-tracking \
 --enable-vp9-highbitdepth --disable-tools --target=x86_64-win64-gcc --prefix="$prefix" \
 --enable-debug --extra-cflags="-D__MSVCRT_VERSION__=0x700 -D_WIN32_WINNT=0x0502 -DWINVER=0x0502 -g -O2 -I$prefix/include"
make -j"${CONEIC_JOBS:-4}" CONFIG_DEBUG=1
make install CONFIG_DEBUG=1
