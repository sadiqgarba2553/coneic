# Coneic VLC source parity

Generated: 2026-10-17T18:29:45.245513+00:00

Baseline: VLC **3.0.24**, commit `6de05adcbaf2e8b85fe86aad4169393098628119`.

**SOURCE CHECKS PASS; PRESENTATION CHANGES REQUIRE REVIEW**

This report compares source files. Build configuration, packaged plugins and runtime behavior remain unverified.

| Measure | Count |
| --- | ---: |
| baseline core option declarations | 405 |
| baseline export symbol declarations | 1080 |
| baseline module registrations | 479 |
| baseline module source files | 1901 |
| baseline tracked files | 4470 |
| current core option declarations | 405 |
| current export symbol declarations | 1080 |
| current module registrations | 479 |
| current module source files | 1914 |
| current tracked and unignored files | 4729 |
| qt workflow source files | 117 |
| unchanged qt workflow source files | 109 |

## Critical entry points

| File | Byte-identical to upstream |
| --- | --- |
| `modules/gui/qt/menus.cpp` | yes |
| `modules/gui/qt/menus.hpp` | yes |
| `modules/gui/qt/dialogs_provider.cpp` | yes |
| `modules/gui/qt/dialogs_provider.hpp` | yes |
| `modules/gui/qt/actions_manager.cpp` | yes |
| `modules/gui/qt/actions_manager.hpp` | yes |
| `lib/libvlc.sym` | yes |
| `include/vlc/libvlc.h` | yes |
| `src/libvlc-module.c` | yes |
| `modules/control/hotkeys.c` | yes |

## Changes and review flags

21 changed baseline files; 259 added files; 0 removed files.

- `.gitignore`: outside presentation allowance
- `bin/vlc_win32_rc.rc.in`: explicit presentation allowance; review required
- `bin/winvlc.c`: explicit presentation allowance; review required
- `modules/gui/qt/Makefile.am`: explicit presentation allowance; review required
- `modules/gui/qt/components/controller.cpp`: explicit presentation allowance; review required
- `modules/gui/qt/components/controller.hpp`: explicit presentation allowance; review required
- `modules/gui/qt/components/interface_widgets.cpp`: explicit presentation allowance; review required
- `modules/gui/qt/components/simple_preferences.cpp`: explicit presentation allowance; review required
- `modules/gui/qt/components/simple_preferences.hpp`: explicit presentation allowance; review required
- `modules/gui/qt/dialogs/help.cpp`: explicit presentation allowance; review required
- `modules/gui/qt/dialogs/preferences.cpp`: explicit presentation allowance; review required
- `modules/gui/qt/dialogs/preferences.hpp`: explicit presentation allowance; review required
- `modules/gui/qt/main_interface.cpp`: explicit presentation allowance; review required
- `modules/gui/qt/qt.cpp`: explicit presentation allowance; review required
- `modules/gui/qt/util/input_slider.cpp`: explicit presentation allowance; review required
- `modules/gui/qt/util/timetooltip.cpp`: explicit presentation allowance; review required
- `modules/gui/qt/util/timetooltip.hpp`: explicit presentation allowance; review required
- `modules/gui/qt/vlc.qrc`: explicit presentation allowance; review required
- `share/Makefile.am`: explicit presentation allowance; review required
- `share/icons/vlc.ico`: explicit presentation allowance; review required
- `src/win32/dirs.c`: explicit presentation allowance; review required

Semantic loss candidates: **0**. API loss candidates: **0**.

Full per-file hashes, workflow inventory, explicit allowances and potential losses are in the companion JSON.

## Limits

- Static source comparison only; no build or runtime verification.
- Presentation allowlist permits edits but does not prove their behavioral safety.
- Token extraction is a bounded syntax heuristic, not a C/C++ parser or complete option inventory.
- Source preservation does not establish configure options, linked dependencies or packaged modules.
- Untracked ignored files are excluded; no unrelated filesystem locations are read.

## Reproduce

Run the workspace script using Python 3; it reads the repository and writes only the two selected reports:

```text
python work/vlc-fork/verify_parity.py --repo "<SOURCE>" --json outputs/Coneic-VLC-parity.json --markdown outputs/Coneic-VLC-parity.md
```

Exit status 0 means the bounded static checks found no protected-source changes or loss candidates; allowed presentation changes still require review. Exit status 1 means review flags, removals or loss candidates were found.
