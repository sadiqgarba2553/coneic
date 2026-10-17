# Coneic native package parity

Generated: 2026-10-17T07:12:23.486595+00:00

**STATIC PACKAGE CHECKS PASS**

Package: `/root/coneic/release/Coneic`. Baseline: supplied VLC 3.0.24 installer, SHA-256 `d711e1e1fe52052748c39080c7dce63f6b7e4c315efedf1774a3f1957b782ff3`.

This checks files, exported names, PE architecture and static imports. It does not verify playback or runtime feature parity.

| Measure | Count |
| --- | ---: |
| baseline plugin dlls | 360 |
| package plugin dlls | 360 |
| baseline plugin jars | 2 |
| baseline data files | 205 |
| package files | 684 |
| pe images checked | 366 |

## Missing plugin DLLs

None.

## Additional plugin DLLs

None.

## Missing plugin JARs

None.

## Missing data files

None.

## Missing notice files

None.

## Missing core files

None.

## Core exported names

| Image | Baseline | Current | Missing |
| --- | ---: | ---: | ---: |
| `libvlc.dll` | 316 | 316 | 0 |
| `libvlccore.dll` | 764 | 764 | 0 |
| `plugins/gui/libqt_plugin.dll` | 3 | 3 | 0 |

## Static dependency issues

None within the declared resolution scope.

## Optional integrations

ActiveX: baseline=True, package=True. NPAPI: baseline=False, package=False. These differences are separate from desktop-player checks.

## Limits

- Static package audit only; no binary was loaded or executed.
- Known Windows DLL/API-set names are allowlisted, not checked for presence on a particular Windows release.
- Both ordinary and delay-load imports are inspected; runtime-computed LoadLibrary/GetProcAddress dependencies remain unobserved.
- DLL resolution only considers the package root and importing image directory; custom runtime loader directories are unobserved.
- Identical exported names do not establish ABI/semantic compatibility.
- Different hashes are expected after rebuilding; input and package hashes identify artifacts, not functional equivalence.
- ActiveX and NPAPI differences are reported separately and do not fail desktop-player checks.
