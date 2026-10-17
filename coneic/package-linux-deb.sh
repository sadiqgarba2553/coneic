#!/usr/bin/env bash
# Ubuntu 24.04 x64 package; system dependencies are derived from every ELF.
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${CONEIC_BUILD_DIR:-$root/build-linux}"
output="${1:-$root/build-linux/packages}"
version="${CONEIC_DEB_VERSION:-0.2.1~alpha.1}"
network_prefix="${CONEIC_NETWORK_PREFIX:-$root/build-linux-deps}"
jobs="${CONEIC_JOBS:-4}"
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || { echo 'CONEIC_JOBS must be a positive integer' >&2; exit 1; }
[[ "$version" =~ ^[0-9][0-9A-Za-z.+:~_-]*$ ]] || { echo 'Invalid package version' >&2; exit 1; }
[[ "$(dpkg --print-architecture)" == amd64 ]] || { echo 'This package target is amd64' >&2; exit 1; }
mkdir -p "$output"
output="$(cd "$output" && pwd)"
stage="$(mktemp -d "${TMPDIR:-/tmp}/coneic-deb.XXXXXXXX")"
trap 'rm -rf -- "$stage"' EXIT
chmod 755 "$stage"
make -C "$build" -j"$jobs" install DESTDIR="$stage"
[[ -x "$stage/opt/coneic/bin/vlc" ]] || { echo 'Configure with --prefix=/opt/coneic' >&2; exit 1; }
install -Dm755 "$root/coneic/linux/coneic" "$stage/usr/bin/coneic"
install -Dm644 "$root/coneic/linux/coneic.desktop" "$stage/usr/share/applications/coneic.desktop"
install -Dm644 "$root/share/coneic/coneic.svg" "$stage/usr/share/icons/hicolor/scalable/apps/coneic.svg"
mkdir -p "$stage/usr/share/doc/coneic" "$stage/DEBIAN" "$stage/debian"
for name in COPYING COPYING.LIB AUTHORS THANKS; do
    [[ ! -f "$root/$name" ]] || install -m644 "$root/$name" "$stage/usr/share/doc/coneic/$name"
done
install -m644 "$root/docs/LINUX.md" "$stage/usr/share/doc/coneic/README.Linux.md"
cp -a "$root/modules/gui/qt/fonts/coneic" "$stage/usr/share/doc/coneic/fonts"
cp -a "$root/modules/gui/qt/pixmaps/coneic-reference" "$stage/usr/share/doc/coneic/design-reference"
cp -a "$root/modules/gui/qt/pixmaps/coneic-supplement" "$stage/usr/share/doc/coneic/design-supplement"
if [[ -d "$network_prefix/share/doc/coneic-network-deps" ]]; then
    cp -a "$network_prefix/share/doc/coneic-network-deps" "$stage/usr/share/doc/coneic/network-dependencies"
fi
# Ubuntu minimal images retain copyright even when other documentation is filtered.
{
    printf 'Coneic by BLUECLOUD — https://bluecloudai.online\nVLC and dependency rights remain with their original authors.\nSee AUTHORS and individual source headers for copyright holders.\n\n'
    cat "$root/COPYING" "$root/COPYING.LIB" \
        "$root/modules/gui/qt/fonts/coneic/LICENSE.txt" \
        "$root/modules/gui/qt/pixmaps/coneic-reference/ATTRIBUTION.md" \
        "$root/modules/gui/qt/pixmaps/coneic-supplement/ATTRIBUTION.md"
    if [[ -d "$network_prefix/share/doc/coneic-network-deps" ]]; then
        cat "$network_prefix/share/doc/coneic-network-deps/"COPYING*
    fi
} > "$stage/usr/share/doc/coneic/copyright"
find "$stage/opt/coneic" -name '*.la' -delete
find "$stage/opt/coneic" -type f -name '*.so*' -exec strip --strip-unneeded {} +
strip --strip-unneeded "$stage/opt/coneic/bin/vlc" "$stage/opt/coneic/lib/vlc/vlc-cache-gen"
LD_LIBRARY_PATH="$stage/opt/coneic/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    "$stage/opt/coneic/lib/vlc/vlc-cache-gen" "$stage/opt/coneic/lib/vlc/plugins"
printf 'Source: coneic\nSection: video\nPriority: optional\nMaintainer: BLUECLOUD <187056627+sadiqgarba2553@users.noreply.github.com>\n\nPackage: coneic\nArchitecture: amd64\nDescription: Coneic media player\n' > "$stage/debian/control"
printf 'libvlc 5 coneic\nlibvlccore 9 coneic\n' > "$stage/debian/shlibs.local"
elf=()
while IFS= read -r -d '' path; do
    if readelf -h "$path" >/dev/null 2>&1; then elf+=(-e"$path"); fi
done < <(find "$stage/opt/coneic" -type f \( -name '*.so*' -o -path '*/bin/*' -o -name 'vlc-cache-gen' \) -print0)
depends="$(cd "$stage" && dpkg-shlibdeps -O -xconeic -l"$stage/opt/coneic/lib" "${elf[@]}" | sed -n 's/^shlibs:Depends=//p')"
[[ -n "$depends" ]] || { echo 'No runtime dependencies were resolved' >&2; exit 1; }
size="$(du -sk "$stage/opt" "$stage/usr" | awk '{s+=$1} END {print s}')"
cat > "$stage/DEBIAN/control" <<EOF
Package: coneic
Version: $version
Architecture: amd64
Section: video
Priority: optional
Maintainer: BLUECLOUD <187056627+sadiqgarba2553@users.noreply.github.com>
Homepage: https://bluecloudai.online
Installed-Size: $size
Depends: $depends, libqt5svg5, hicolor-icon-theme
Description: Coneic media player — 0.2.1 Alpha
 A VLC 3.0.24 source fork by BLUECLOUD with a redesigned Qt interface,
 Moment Cards, Study Mode, Smart Collections, library backup and relinking,
 a command palette and customisable Home.
 Built for Ubuntu 24.04 x64. The full source and licenses are available
 from https://github.com/sadiqgarba2553/coneic.
EOF
# Temporary dpkg-shlibdeps inputs are not package payload.
rm -r -- "$stage/debian"
(cd "$stage" && find opt usr -type f -print0 | sort -z | xargs -0 md5sum) > "$stage/DEBIAN/md5sums"
asset_version="${version//\~/-}"
dpkg-deb --root-owner-group --build "$stage" "$output/coneic_${asset_version}_amd64.deb"
sha256sum "$output/coneic_${asset_version}_amd64.deb"
