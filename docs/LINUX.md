# Coneic for Linux

The initial native target is **Ubuntu 24.04, x86-64 (amd64)**. It uses the same VLC 3.0.24 source and Coneic 0.2.1 Alpha Qt interface as Windows. Linux has its own platform modules and uses distribution codec and Qt libraries; the Windows plugin count is not a Linux feature-parity measurement.

## Install the Debian package

Download the Ubuntu package, open a terminal in its folder, then run:

```sh
sudo apt install ./coneic_0.2.1-alpha.1_amd64.deb
coneic
```

APT installs the declared runtime dependencies. Launch **Coneic** from the application menu or run `coneic /path/to/video.mp4`. No Windows compatibility layer is needed. Remove it with `sudo apt remove coneic`; this retains personal data.

Coneic installs under `/opt/coneic`, with a launcher at `/usr/bin/coneic`. It does not replace the system VLC executable or register itself as the default player. Its launcher uses private VLC libraries/plugins. The core configuration is `$XDG_CONFIG_HOME/coneic/vlcrc` (default `~/.config/coneic/vlcrc`); Qt settings and `personal-library.json` are in that same `coneic` configuration directory. Native VLC data and caches use dedicated `coneic/vlc` subdirectories under the XDG data/cache roots. Qt shortcuts apply while Coneic is active.

## Build from source on Ubuntu 24.04

Enable the Ubuntu source repositories (`deb-src`, or `Types: deb deb-src` in the Ubuntu deb822 sources file), then install build dependencies:

```sh
sudo apt update
sudo apt build-dep vlc
sudo apt install qtbase5-dev libqt5svg5-dev libvpx-dev dpkg-dev
bash coneic/build-linux-network-deps.sh
bash coneic/build-linux.sh
bash coneic/package-linux-deb.sh ./build-linux/packages
```

Run these scripts from a clean native Linux checkout, preferably on a Linux filesystem. They do not use the Windows contrib toolchain. `CONEIC_JOBS` controls build parallelism (default 4). Extra arguments to `build-linux.sh` are passed directly to configure. Packaging requires the default `/opt/coneic` prefix. Each ELF binary/plugin is included in automatic runtime-dependency analysis; VLC core libraries are provided privately by the package.

The tested build also installs these optional libraries **before building**:

```sh
sudo apt install libaom-dev libgme-dev librist-dev libvpl-dev \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
  libjack-jackd2-dev libprojectm-dev
```

Use a native Git checkout or the Linux source tarball so shell scripts retain Unix line endings and executable permissions.

The network-dependency helper builds the upstream contrib recipes for Live555 2026.06.24 and microdns 0.2.0, including their pinned checksums and patches. These provide RTSP and mDNS discovery modules. It installs static libraries in `build-linux-deps`; the main build detects that directory. `CONEIC_NETWORK_PREFIX` can select another prefix for both scripts and packaging. The Linux source tarball includes the two original dependency archives under `contrib/tarballs`, and the package includes their license notices.

## Run the UI checks

Install `xvfb xauth ffmpeg` and the built package, then run as a regular user:

```sh
bash coneic/tests/run_linux_ui.sh /tmp/coneic-checks
QT_SCALE_FACTOR=2 bash coneic/tests/run_linux_ui.sh /tmp/coneic-checks-2x
python3 coneic/tests/validate_cli.py --app /usr/bin --output /tmp/coneic-codec-checks
```

The tests use an isolated X server, configuration, media fixtures and library files. JSON reports and rendered screenshots are written to the selected directory. They exercise actual native playback and the personal features; silent dummy audio is used in this headless environment.

The Linux H.264 short-clip check explicitly uses `venc=x264{tune=zerolatency}`. In the first native test, a two-second conversion using the default x264 lookahead produced an empty video track when playback stopped before the encoder buffer drained. The low-latency preset avoids that case; this preview does not claim the default short-clip conversion behavior is fixed in VLC's engine. VP8/VP9 validation requires the native libvpx plugin; the Linux build driver requires that development library rather than silently omitting the module.

The Linux package also includes the [bounded H.264/H.265 buffer-metadata fix](HXXX-METADATA-FIX.md). That separate defect dropped timestamps during buffer replacement and produced invalid MP4 timing. The real-buffer regression reproduces three failures with the original helper and passes all five cases with the correction.

## Validation of the published package

The installed `.deb` passes 23 native UI checks, 19 persistence checks, 16 live feature checks and 14 library-tool checks at both normal and 200% scale. Some checks aggregate others, so these counts should not be summed as independent coverage. Eight native media checks pass for MJPEG/WAV decoding and H.264, VP8 and VP9 encoding/decoding. Encoded codec types, dimensions and decoded frame counts are independently checked with FFprobe; the H.264 fixture yields 18 frames, VP8 19 and VP9 55.

The package contains 390 plugin libraries. All 398 ELF files resolve their shared-library dependencies on the test host. All 917 regular archive files match the package's integrity manifest. The minimal WSL image intentionally excludes most `/usr/share/doc` files via its dpkg policy; the audit identifies those exclusions explicitly, verifies the complete archive, and confirms the combined copyright/license file is installed. No unexpected installed-file verification failures remain.

JSON evidence and actual screenshots are under `docs/evidence/linux-0.2.1` and `docs/screenshots/linux-0.2.1`. These checks ran as a regular user on Ubuntu 24.04 under WSL2 with an isolated X server and dummy audio, rather than on physical Linux audio/video hardware.

## Scope and provenance

Moment Cards, Study Mode, Smart Collections, media relinking, backup/restore, the command palette, Home customisation and BLUECLOUD attribution use shared source. Hardware decoding, physical audio/video devices, protected discs and network/casting services require separate Linux-machine testing. A WSL/headless UI check cannot certify those environments. Other distributions, ARM builds, Flatpak, AppImage and Snap are not claimed as supported by this Ubuntu package.

The player retains VLC's GPL/LGPL notices. Juxtopposed design-reference icons are CC BY 4.0 and Inter fonts use OFL; source attribution is in `docs/DESIGN-REFERENCE.md`, and notices are included in the package. The BLUECLOUD parent-company website is <https://bluecloudai.online>.
