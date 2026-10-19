# Microsoft Store packaging and certification recovery

The 17 October 2026 report for Coneic (product ID
`81ac01d7-80ad-423e-b8e5-6822b54e2253`) identifies an **unsigned EXE**
under policy 10.2.9. The generic certificate-abuse wording is not evidence that
Coneic contains malware or used an abused certificate.

## Confirmed submission problem

The live download at `https://bluecloudai.online/coneic/downloads/1.0.0/Coneic.exe`
was checked without execution on 18 October 2026:

- 120,974 bytes; SHA256 `d32e719c770e96de47e9f0423248e210475a9a36a3303b98c43c76b409575d21`.
- Windows Authenticode status `NotSigned`.
- Exactly matches the previously tested portable Coneic player executable.
- Imports `libvlc.dll`: this single file is not a self-contained installer.
- Embedded product version was VLC's `3,0,24,0`, rather than Coneic's product version.

Do not replace the bytes at the submitted URL. Microsoft requires immutable,
versioned installer URLs. Installer return-code documentation does not make a
player executable into an NSIS installer.

## Preferred recovery: complete MSIX

`coneic/windows/package-msix.ps1` stages the entire supplied runtime, preserves
all plugin DLLs, language files and license notices, corrects only launcher
VERSIONINFO, generates Start icons from Coneic's existing logo, declares media
file associations, and invokes MakeAppx with validation enabled. It then checks
every staged file against the resulting archive and requires SHA256 block hashes.
It does not sign, upload, change trust settings, delete app reservations or alter
the input portable application.

Requirements: Windows, Windows SDK (MakeAppx), PowerShell, Python 3.12 or later.
Start with the full tested Windows app directory, not the website's single EXE.

```powershell
# Local validation only; deliberately cannot be submitted as Coneic's Store identity.
.\coneic\windows\package-msix.ps1 -App C:\build\Coneic `
  -OutputDirectory C:\build\coneic-msix-test -Mode Test

# Copy store-identity.example.json and fill in exact Partner Center values first.
.\coneic\windows\package-msix.ps1 -App C:\build\Coneic `
  -OutputDirectory C:\build\coneic-msix-store -Mode Store `
  -IdentityFile C:\build\coneic-store-identity.json
```

The identity JSON requires `Name` (Package/Identity/Name), `Publisher`
(Package/Identity/Publisher, normally a Microsoft-assigned `CN=...`) and
`PublisherDisplayName`. The product GUID and report publisher display name are
insufficient. Store mode rejects example and validation identities.

A Win32 EXE/MSI listing may need a separate MSIX product/name reservation flow.
Check the existing product type in Partner Center. Do not delete the existing
reservation merely to try packaging; any migration or reservation deletion needs
the owner's explicit decision. A test identity cannot resolve name ownership.

The product stays **0.2.1 Alpha** and launcher FileVersion is **0.2.1.0**.
The separate MSIX package version is **1.2.1.0**: Store package rules require a
nonzero first component and reserve the last component as zero. This packaging
version is not a claim that the Alpha has become a stable 1.2.1 release. Engine
DLL versions retain VLC/third-party attribution. Future packages must use a version
greater than the last submitted package for the same identity.

For Microsoft Store MSIX distribution, Microsoft signs the submitted package.
An unsigned Store-upload MSIX is expected; this does **not** make an unsigned EXE
valid in the EXE/MSI submission flow, or enable ordinary public MSIX sideloading.

Before submission, validate registration, Start activation, file activation with
spaces, playback, user-library persistence and uninstall in a clean Windows VM.
Run the Windows App Certification Kit on the real-identity candidate. Packaging
checks are not a guarantee that Microsoft will certify the app. This package
uses the existing tested Windows Alpha engine; it does not add the later Linux
Annex-B metadata fix to the Windows engine.

## Alternative: retain the existing EXE/MSI listing

Build a full offline installer. Sign the complete PE payload with SHA256 or
stronger using a CA-trusted code-signing identity, then build/sign the final
installer and timestamp signatures. Preserve already valid third-party
signatures when appropriate. Test silent install and uninstall without downloads
in a clean VM. A self-signed certificate or a published SHA256 checksum is not a
substitute for trusted Authenticode signing.

```powershell
.\coneic\windows\verify-exe-submission.ps1 -Installer C:\build\Coneic-Setup.exe `
  -Payload C:\build\Coneic -Report C:\build\exe-store-audit.json `
  -OfflineSilentInstallationTestPassed
```

The flag records a completed independent VM test; it does not perform one.
The gate rejects the standalone player, incomplete payload, untrusted/unsigned
PE files, and unrecognized or weak file digests. It fails closed if SignTool's
output cannot establish the file digest (English SDK output expected). It never
loads signing secrets or publishes a download. A passing gate is not a Store
certification result. Use a new versioned download URL and update Partner Center
only after the installer and its signatures are verified.

## Microsoft references

- [EXE/MSI installer requirements](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msi/app-package-requirements)
- [MSIX signing guide](https://learn.microsoft.com/en-us/windows/msix/package/sign-msix-package-guide)
- [Manual desktop MSIX packaging](https://learn.microsoft.com/en-us/windows/msix/desktop/desktop-to-uwp-manual-conversion)
- [MSIX package and version requirements](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msix/app-package-requirements)

Publisher: BLUECLOUD, <https://bluecloudai.online>.
