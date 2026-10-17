# Provenance

| Input | Identity |
| --- | --- |
| Source | VideoLAN VLC tag 3.0.24 |
| Exact upstream commit | `6de05adcbaf2e8b85fe86aad4169393098628119` |
| Official source tar SHA-256 | `e7cab503d1d7d5849b89d2cf0e1ee60d0ef6d012407791b644b9cfc0cc225fdf` |
| Supplied Windows installer SHA-256 | `d711e1e1fe52052748c39080c7dce63f6b7e4c315efedf1774a3f1957b782ff3` |
| Supplied SVG SHA-256 | `90fd204077e6d96fe22f4644cd3d24bd324bd35fda7fb539478a82a5da2c5b93` |
| Matching contrib revision | `910cc9cada6080dd777fbafb2ebdd0c24f702c30` |
| Observed contrib archive SHA-256 | `49f7690b3d09760f842f9f8e9612c28de8cbb1d027bcb3f6aaa25767611eea8c` |

The supplied installer was statically extracted with Linux 7-Zip 23.01 without executing the installer. Its wrapper is x86; the actual player payload is Windows x64 and declares version 3.0.24. The extracted baseline has 584 files, including installer internals. Desktop component comparisons exclude NSIS helper files.

The player/core/plugins are rebuilt from the source fork. The portable preview retains the original supplied `axvlc.dll` unchanged to preserve that separately built integration. It is not described as a newly rebuilt Coneic component. Original credits/notices remain. The source tree's contrib recipes identify third-party sources and patches.

REA evidence and ordinary extraction/source/runtime evidence are distinguished in VALIDATION.md. The earlier installed VLC 3.0.23 investigation is historical context; the final package comparison uses the supplied 3.0.24 payload.

This repository contains a complete release source import, plus Coneic changes and documentation. It is an independent new repository, not a claim to have imported every upstream Git commit. The exact baseline remains available for comparison from VideoLAN.
