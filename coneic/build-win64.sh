#!/usr/bin/env bash
# Coneic Windows build driver. Upstream feature/build defaults remain intact.
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"
unset CC CXX
export VLC_PREBUILT_CONTRIBS_URL="https://artifacts.videolan.org/vlc-3.0/win64/vlc-contrib-x86_64-w64-mingw32-910cc9cada6080dd777fbafb2ebdd0c24f702c30.tar.zst"
export CONEIC_JOBS="${CONEIC_JOBS:-4}"
[[ "$CONEIC_JOBS" =~ ^[1-9][0-9]*$ ]] || { echo 'CONEIC_JOBS must be a positive integer' >&2; exit 1; }
mkdir -p .coneic-tools/bin
cat > .coneic-tools/bin/getconf <<'TOOL'
#!/bin/sh
if [ "$1" = "_NPROCESSORS_ONLN" ]; then
    printf '%s\n' "${CONEIC_JOBS:-4}"
else
    exec /usr/bin/getconf "$@"
fi
TOOL
chmod +x .coneic-tools/bin/getconf
export PATH="$root/.coneic-tools/bin:$PATH"
mkdir -p .coneic-tools/compat
compiler=x86_64-w64-mingw32-gcc
"$compiler" --version > .coneic-tools/compat/compiler.txt
"$compiler" -dumpmachine >> .coneic-tools/compat/compiler.txt
definitions=()
missing=()
while read -r symbol definition; do
    # Exact linker spelling; avoids header macros and prototype differences.
    cat > .coneic-tools/compat/probe.c <<PROBE
extern char coneic_probe_symbol __asm__("$symbol");
void *volatile coneic_probe_address = &coneic_probe_symbol;
int main(void) { return coneic_probe_address == 0; }
PROBE
    if ! "$compiler" -D__MSVCRT_VERSION__=0x700 .coneic-tools/compat/probe.c \
        -o .coneic-tools/compat/probe.exe 2>".coneic-tools/compat/probe-$definition.log"; then
        missing+=("$symbol")
        definitions+=("-DCONEIC_NEED_$definition=1")
    fi
done <<'SYMBOLS'
fstat64i32 FSTAT64I32
fstat64 FSTAT64
stat64 STAT64
stat64i32 STAT64I32
ftime64 FTIME64
__imp_wcrtomb IMP_WCRTOMB
__imp_mbrtowc IMP_MBRTOWC
__imp_mbrlen IMP_MBRLEN
__imp_ftello64 IMP_FTELLO64
__imp_fseeko64 IMP_FSEEKO64
__imp__set_errno IMP_SET_ERRNO
__imp___timezone IMP_TIMEZONE
__imp___daylight IMP_DAYLIGHT
__imp___p___argc IMP_ARGC
__imp___p__environ IMP_ENVIRON
SYMBOLS
printf '%s\n' "${definitions[@]}" > .coneic-tools/compat/definitions.txt
if ((${#missing[@]})); then
    printf 'Missing MinGW symbols: %s\n' "${missing[*]}" >&2
    if [[ "${CONEIC_ENABLE_MINGW_COMPAT:-0}" != 1 ]]; then
        echo 'Use a matching upstream toolchain, or opt in with CONEIC_ENABLE_MINGW_COMPAT=1.' >&2
        exit 1
    fi
    "$compiler" -std=c11 -Wall -Wextra -Werror -D__MSVCRT_VERSION__=0x700 \
        "${definitions[@]}" -c coneic/mingw_compat.c -o .coneic-tools/compat/mingw_compat.o
    export LDFLAGS="${LDFLAGS:-} $root/.coneic-tools/compat/mingw_compat.o"
fi
if [ -n "${CONEIC_CONTRIB_ARCHIVE:-}" ]; then
    expected="49f7690b3d09760f842f9f8e9612c28de8cbb1d027bcb3f6aaa25767611eea8c"
    actual="$(sha256sum "$CONEIC_CONTRIB_ARCHIVE" | cut -d' ' -f1)"
    [ "$actual" = "$expected" ] || { echo "Contrib archive hash mismatch" >&2; exit 1; }
    mkdir -p contrib/contrib-win64
    cp "$CONEIC_CONTRIB_ARCHIVE" contrib/contrib-win64/vlc-contrib-x86_64-w64-mingw32-latest.tar.zst
fi
# -r keeps release optimization; empty -i avoids unrelated installer/ActiveX fetches.
# Preserve the original failure log. Only the observed EBML C++ ABI boundary
# triggers a dependency rebuild; unrelated failures remain failures.
if ./extras/package/win32/build.sh -a x86_64 -p -l -r -i '' 2>&1 | tee .coneic-tools/windows-build.log; then
    :
elif grep -q "undefined reference.*EbmlString" .coneic-tools/windows-build.log; then
    bash coneic/rebuild-matroska.sh
    make -C win64 -j"$CONEIC_JOBS"
else
    echo "Build failed; see .coneic-tools/windows-build.log" >&2
    exit 1
fi
# Auto-detected codecs must be enabled, not merely packaged as empty plugins.
if ! grep '^CPPFLAGS_vpx =' win64/modules/Makefile | grep -q -- '-DENABLE_VP8_DECODER.*-DENABLE_VP9_DECODER.*-DENABLE_VP8_ENCODER.*-DENABLE_VP9_ENCODER'; then
    bash coneic/rebuild-vpx.sh
    (cd win64 && ./config.status --recheck && ./config.status)
    touch modules/codec/vpx.c
    make -C win64 -j"$CONEIC_JOBS"
fi
grep -q '^#define ENABLE_NLS 1' win64/config.h || { echo 'Translations are not enabled; inspect gettext detection.' >&2; exit 1; }
grep '^CPPFLAGS_vpx =' win64/modules/Makefile | grep -q -- '-DENABLE_VP8_DECODER.*-DENABLE_VP9_DECODER.*-DENABLE_VP8_ENCODER.*-DENABLE_VP9_ENCODER' || { echo 'VP8/VP9 paths are incomplete.' >&2; exit 1; }
make -C win64 package-win32-debug-zip

