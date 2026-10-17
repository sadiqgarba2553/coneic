# Coneic â€” VLC source fork

This project is a local source fork of VideoLAN VLC **3.0.24**, exact upstream commit **6de05adcbaf2e8b85fe86aad4169393098628119**. Branch: main. It supersedes the earlier standalone WPF prototype as the route to VLC feature parity.

## Scope

The existing VLC media engine, plugin sources, command-line controls, menus, action handlers, capture/stream/disc dialogs, renderer discovery, extensions, preferences, filters, playlist and media-library implementations are retained. This is a source-preservation strategy, not a fresh reimplementation of each feature.

Presentation changes add Coneic window/tray/executable branding, your supplied artwork, a reference-based Qt interface, Inter typography and native focus/press states. The theme follows the requested [Apple design skill](https://github.com/emilkowalski/skills/blob/main/skills/apple-design/SKILL.md), adapted to Windows/Qt. High-contrast mode keeps native styling; reduced-motion settings disable optional Qt UI effects. Existing slider tracking and keyboard/accessibility behavior remain native Qt.

Windows settings use the separate **%APPDATA%/coneic** directory. Upstream internal module IDs and API identifiers remain unchanged for compatibility. VLC copyright, credits, licenses and upstream About information are retained alongside Coneic/BLUECLOUD and design attribution. See docs/DESIGN-REFERENCE.md for current adaptation scope.

## Your logo

Original source: Desktop/Untitled design.svg.
SHA-256: **90fd204077e6d96fe22f4644cd3d24bd324bd35fda7fb539478a82a5da2c5b93**.

The source SVG was copied byte-for-byte. PNGs and a nine-resolution ICO were mechanically rendered from it; its orange background and artwork were preserved. Embedded raster layers are part of the supplied SVG. Raster assets are used in the Qt interface to avoid differences in SVG mask/filter support across Qt versions.

## Windows x64 build

The build was prepared using an isolated WSL2 Ubuntu Base 24.04.5 environment named **ConeicBuild**. Build data resides in the chat workspace's work/vlc-fork/wsl-build folder; native sources/intermediates are under /root/coneic/source in that environment. It is separate from Windows' global compiler configuration.

Use Linux and a MinGW x86_64 cross-toolchain with the dependencies listed in the build audit. Keep the build tree on the Linux filesystem for performance. Native tools include protoc 3.4.1 and the jobserver-capable Ninja selected by upstream bootstrap.

```sh
CONEIC_ENABLE_MINGW_COMPAT=1 bash coneic/build-win64.sh
```

The driver retains upstream release, GUI, skins, translations and media feature defaults. It pins the matching VideoLAN contrib archive URL. A supplied cache path can be set via CONEIC_CONTRIB_ARCHIVE. The recorded cache SHA-256 is 49f7690b3d09760f842f9f8e9612c28de8cbb1d027bcb3f6aaa25767611eea8c; this is a locally observed digest, not an upstream-signed checksum.

The build used Ubuntu MinGW headers/runtime 11.0.1 and GCC 13.2. The matching contrib archive was compiled with a newer runtime. The driver probes fifteen exact missing CRT symbol spellings independently and enables only the needed adapters when CONEIC_ENABLE_MINGW_COMPAT=1 is set. The adapters use the existing CRT implementations, assert the x64 structure layouts, and passed 40 Windows runtime checks. Source: coneic/mingw_compat.c. They do not disable VLC modules. Use a matching upstream toolchain when available.

The prebuilt EBML library also used a different C++ conversion-operator symbol spelling. The wrapper detects this specific link failure and invokes coneic/rebuild-matroska.sh to rebuild libebml 1.4.6 and libmatroska 1.7.2 with the active compiler. Downloads are verified against the original VLC contrib SHA-512 manifests. Other build failures remain failures.

Empty installer mode avoids the upstream packaging step's unpinned external ActiveX repository fetch. Desktop player packaging and any baseline component differences must be reported separately, not inferred from source preservation.

## Verification

The accompanying reports distinguish:
- byte-level source preservation and option/API/menu inventories;
- successful compilation;
- packaged plugin inventory against the supplied installer;
- native GUI and playback/conversion runtime tests;
- physical-device and external-network behavior not exercised.

The opt-in native UI runner uses the CONEIC_TEST_OUTPUT environment variable. It opens the main window and selected existing dialogs, captures images and menu inventory, then quits. Normal launch does not enable it. Tests do not start capture devices, transmit network streams or alter the operating system's accessibility settings.

Do not describe source preservation alone as a completed, runtime-certified one-to-one replacement. Use the latest delivery and validation reports for the actual status.

## Licensing

This fork remains under the upstream GPL terms in COPYING, with component licenses retained. Your provided branding artwork is separate input; this project does not make ownership claims about it. This is a development preview published in the independent Coneic repository. It is not an official VideoLAN release.

## Final preview status

The native Windows build and package checks pass. See docs/VALIDATION.md for current runtime results and the unresolved embedding comparison. libvpx was rebuilt with all four VP8/VP9 paths, AVX512 dispatch and high-bit-depth support. All 104 catalogs are packaged, and German menus were verified.
