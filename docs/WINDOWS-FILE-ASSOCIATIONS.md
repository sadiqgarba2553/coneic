# Windows file associations

Coneic registers 134 audio, video and playlist extensions from its media file filters. Each extension has a distinct labelled icon, with orange audio waveforms, blue video play marks or purple playlist marks. Icons include 16, 20, 24, 32, 48, 64, 128 and 256 pixel sizes. Generic archive/configuration formats and subtitle sidecars are not registered as independently playable media.

In General Settings, choose **Set Coneic as default player**. The helper registers the current portable location for the current Windows user and opens Coneic's Default apps page. Windows asks the user to select their defaults. The same action is available through **Set Coneic as default.cmd** beside the executable. After moving the portable folder, run it again to repair the registered paths.

The native full-preferences association button uses the same flow. It no longer opens VLC's association UI. Registration supplies Capabilities, RegisteredApplications, OpenWithProgids, application SupportedTypes, quoted launch commands and a per-format DefaultIcon. It does not alter UserChoice or extension defaults. Unregistration checks installation ownership and preserves other apps' entries. The removal command must run before deleting a portable folder.

BlueCloud Launcher automatically registers payloads containing the helper during installation and startup repair, updates paths after app updates and unregisters before app removal. Isolated launcher test profiles skip real registrations. Older payloads without the helper remain compatible. Release preparation stages the helper and icons before archiving. MSIX packaging declares each format and its PNG icon separately; packaged associations are managed by Windows.

## Validation on 19 October 2026

- 1,077 isolated registry checks passed across 134 formats: repeated registration, command quoting, format icon loading, unique icon hashes, default preservation, ownership protection and removal.
- 24 native interface checks passed, including the visible and enabled default-player button, all settings categories, retained dialogs, playback and seek previews.
- Eight native decode/encode checks passed: MJPEG, WAV, H.264, VP8 and VP9.
- Five launcher tests passed, including helper lifecycle, compatibility with older payloads and isolated-profile behavior.
- MakeAppx schema/archive validation passed with all 134 per-format declarations. This test package uses a local validation identity and is not a signed distribution build or a Store certification result.
- Live Windows association queries resolved WAV, MP3, FLAC, MP4, MKV and M3U8 to the new desktop copy. All 134 Capabilities entries were checked.
- The supplied Alpha-r2 folder remains intact. The new desktop copy retains 700 original files byte-for-byte. Only the Qt presentation plugin and its generated plugin cache differ among original payload files. All 360 plugin DLLs are present.
- The portable ZIP passed archive verification. Existing user data and website files were not modified.

The generated reports and screenshots are under `artifacts/coneic-associations-validation/`. The portable output is in the desktop folder `Coneic - File Associations`. Build and test commands are documented in [BUILD.md](BUILD.md).

Windows registration follows Microsoft's [Default Programs guidance](https://learn.microsoft.com/en-us/windows/win32/shell/default-programs) and [Default Apps settings launch guidance](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/launch-default-apps-settings).
