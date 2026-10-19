# App-owned releases

Update package/product versions, `release.config.json` and release notes before building. Published assets are immutable; choose a new version for changed binaries. Source and assets belong only in this app repository, never in the website repository.

Build the native Qt presentation with the documented pinned WSL toolchain. Stage the full preserved runtime with `coneic/windows/prepare_payload.py`, run native/glass/resume and file-association checks in isolated profiles, then build the installer with `coneic/windows/package-installer.ps1`. Run `node tools/package-zip.mjs`. See `docs/BUILD.md` and `docs/WINDOWS-SIGNING.md`.

Commit and push the exact source to this repository, then run `node tools/release.mjs prepare` and `node tools/release.mjs publish` with Git credentials, or dispatch the Windows workflow where available. The publisher checks repository ownership, source commit and file hashes. Private signing keys never enter source archives or release assets.

After every planned app release is public, build, verify and promote the signed feed from the Launcher repository. Website deployment is separate.
