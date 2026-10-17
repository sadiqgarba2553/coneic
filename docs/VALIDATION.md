# Validation and known limits

This is a development preview. There are three separate kinds of evidence: source preservation, static package comparison, and bounded runtime tests. No one category proves every VLC feature works.

| Check | Observed result |
| --- | --- |
| Upstream module registrations | 479 retained |
| Core option declarations | 405 retained |
| Export declarations | 1,080 retained |
| Audited Qt workflow files | 117 audited; exact presentation edits listed in current source report |
| Plugin DLLs / JARs | 360 / 2; none missing |
| Baseline support files | 205; none missing |
| Core named exports | 316 libVLC + 764 core retained |
| Required DLL imports | No unresolved dependencies in static check |
| CRT adapter probe | 40 pass, zero failures |
| Native executable icons | Nine exact PNG payload matches |
| Native UI | 21 pass; clean exit; includes real playback and thumbnail frames |
| MJPEG/WAV controls | 13 pass; clean engine shutdown |
| Native executable media checks | 8 pass: MJPEG/WAV and H.264/VP8/VP9 |
| Native Qt H.264 playback | Exit 0 |
| Translations | 104 source catalogs; German native menus observed |

## What was corrected

The older local compiler required narrowly gated CRT adapters. EBML/Matroska were rebuilt for their C++ ABI. Gettext was initially disabled by a failed link probe; all required CRT symbols were supplied and NLS was rebuilt. VP8/VP9 paths were initially compiled out because of a prebuilt AVX512 link failure; libvpx was rebuilt with AVX512 and high bit depth enabled, detection repeated, and the plugin object explicitly rebuilt. Runtime VP8 and VP9 encoding/decoding now pass. File presence alone would have missed these gaps.

The test host exports `LC_ALL=C.UTF-8`, which overrides a language choice. Clearing that override for the translated child verified German menu labels. The normal host environment was restored afterward.

## Open embedding failure

Standalone libVLC H.264 embedding followed by stop/release produced **0xC0000005** here. The supplied VLC 3.0.24 reproduced the same failure. Native C and Python probes were used. The root cause is unresolved; this is an observation in this environment, not an externally confirmed upstream diagnosis.

MJPEG/WAV control lifetimes pass. The native application GUI and command-line H.264 paths exit cleanly. The failing test is preserved in `coneic/tests/validate_native.py` full mode, with a small native C reproducer. `--controls-only` selects the passing MJPEG/WAV scenario; it does not assert that the H.264 embedding failure is fixed.

See `evidence/runtime/known-embedding-failure.json` for the exact shared fixture digest and observed exits. The long generated H.264 reproduction fixture is supplied with the test inputs.

## Not exercised

Physical cameras/tuners/audio devices, optical media/protected discs, external network services/casting, general GPU decode coverage, COM registration and installer shell/file associations were not certified. The portable package makes no registration changes.

## Evidence map

- `evidence/Coneic-VLC-parity.*`: source blob/declaration audit, with its limits.
- `evidence/Coneic-VLC-package-parity.*`: complete baseline plugin/data/dependency/export comparison.
- `evidence/Coneic-VLC-supplied-payload.*`: actual installer payload inventory.
- `evidence/Coneic-native-resource-evidence.json`: REA resource inspection of Coneic.exe, Evidence `ev_34aa8c6d10d1ae48c764710e096daa8cb5af3e6916e4e96e0598dede8e8c1f40`.
- `evidence/vlc-evidence.json`: original installer REA evidence. The installed 3.0.23 analysis is separately named historical reference evidence.
- `evidence/runtime/`: machine-readable native UI/control/CLI/icon/locale results and comparison limitation.
- `screenshots/`: actual native widget captures, not design mockups.

Published copies redact local host paths for portability. Artifact digests and original Evidence IDs remain recorded. These path-redacted copies are derivative reports, not byte-identical original REA envelopes; original local results remain in the delivery outputs.

## Redesign preview evidence

`evidence/redesign/` contains the current 21-check native UI report, 40-check CRT report, Inter font/menu inventory, 1,346 native settings entries, and fresh source/package reports. The source audit retains all declarations and finds no protected-scope flags; 109 of 117 audited Qt workflow files remain byte-identical. The other eight have explicit presentation review allowances. Current screenshots show synthetic test media, not a mockup. Local thumbnails and seek previews were generated successfully using isolated native workers.
