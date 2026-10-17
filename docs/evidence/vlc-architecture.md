# VLC evidence and Coneic architecture

Investigation date: 16 October 2026. Static analysis only; no supplied installer was executed. This report separates the supplied installer, an existing installed VLC reference, public source documentation, and Coneic design choices.

Update: the supplied installer was later successfully unpacked with full Linux 7-Zip 23.01 in isolated WSL. See `Coneic-VLC-supplied-payload.md` and its JSON inventory for the actual 3.0.24 payload: 584 files, including 360 playback plugin DLLs. The extraction limitation below records the earlier 7za boundary and is now resolved; installed-3.0.23 observations remain separately attributed.

## Artifact identity and evidence ledger

| Question | Finding and evidence | Coverage |
| --- | --- | --- |
| What did the user supply? | `<DESKTOP>\vlc-3.0.24-win64.exe`, 46,269,360 bytes, SHA-256 `d711e1e1fe52052748c39080c7dce63f6b7e4c315efedf1774a3f1957b782ff3`. REA identifies an x86 PE installer. | Installer identity established; filename does not prove payload version. |
| What does REA establish about its UI resources? | Evidence `ev_771fecc32aae294172f95596574d568ed997fcfa564d825213d2c66ab22d7c66`: 121 entries examined, 58 resources, including six icons, 49 dialogs, one icon group, bitmap and manifest. See `vlc-evidence.json`. | Complete bounded resource inventory; playback behavior unobserved. |
| Was the installer unpacked? | Shell attempt using bundled 7za 21.07 returned `Cannot open the file as archive`. This executable lacks the necessary archive support for this artifact. | Supplied payload inventory unresolved. No claim is made about its internal modules. |
| What reference was independently inspected? | Existing `C:\Program Files\VideoLAN\VLC\vlc.exe` reports FileVersion `3.0.23`, ProductVersion `3,0,23,0`. SHA-256 `bfa5740f028a8f310ca01c779bbc5f2f1b435c41539c54de592f715bb0c10389`. | A separate installation; never equate it with the supplied 3.0.24 installer. |
| Is its engine a separate library? | Shell PE import parser observes `vlc.exe` imports `libvlc.dll`, which imports `libvlccore.dll`. `plugins/gui/libqt_plugin.dll` imports `libvlccore.dll`. | Import tables establish library dependencies, not runtime call order. |
| Is the engine x64? | REA evidence `ev_9625a00b7ec135db069881983ec28ca9b2322cd7acc8bed1130aa32e95fd0b68` identifies installed `libvlc.dll` as PE32+, machine 34404 (0x8664), SHA-256 `8ae9f16a72441f43fb4ae8f72c843736726e067ea4a8def2646748631cc4e872`, 192,408 bytes. | Complete bounded resources, two resources; no decompilation. Raw response in `vlc-reference-rea-evidence.json`. |
| Can a new UI control this engine? | Shell export parser observes 316 named exports in installed `libvlc.dll`, including play/pause/stop, media creation, seek, native HWND rendering, volume, tracks, subtitles, snapshots, rate, equalizer and media-list symbols. | Entry points exist; successful invocation requires runtime testing. Full names/RVAs in `vlc-static-inventory.json`. |
| How broad is the reference? | Installed `plugins` contains 363 DLL files, two JAR files and `plugins.dat`; files and hashes are recorded. | Filesystem presence only; no claim that every plugin loads or every feature works. |

## How the parts fit

The installed dependency tables show an application entry point, a public engine DLL, a core DLL and a Qt GUI plugin. Installed README describes `lib/` as the libVLC source, `src/` as libvlccore, and `modules/` as plugins. These observations support separating Coneic's own interface from a reused media engine.

The conceptual playback pipeline below is an interpretation of VideoLAN's module categories and source, corroborated by installed filenames. It is **not** a recovered execution trace from the supplied installer:

```text
Coneic controls and playlist
         | public libVLC API
         v
file/network access -> stream handling -> container demux -> codec decode
                                                           |         |
                                                   audio output   video output
                                                                 + subtitles
```

Access obtains bytes from a file or network; demux separates container tracks; decoders turn compressed tracks into audio/video; output modules present them. Subtitle decoding/rendering supplies overlays. VideoLAN's [VLC 3.0 module list](https://github.com/videolan/vlc-3.0/blob/master/modules/MODULES_LIST) describes these module roles, and its [input decoder source](https://github.com/videolan/vlc-3.0/blob/master/src/input/decoder.c) contains the decoder/output coordination. Those branch sources are documentation references, not proof of byte-for-byte correspondence to either inspected binary.

## Capabilities mapped to implementation

All binary observations below refer to installed VLC 3.0.23. Symbol and plugin inventories are independently reproducible with `work/vlc-static-evidence.cjs`; they are shell observations, not REA Evidence IDs.

| User-visible feature | Observed static evidence | Coneic implementation approach |
| --- | --- | --- |
| Local audio/video playback | `libfilesystem_plugin.dll`, `libmp4_plugin.dll`, `libmkv_plugin.dll`, `libavcodec_plugin.dll`; `libvlc_media_new_path`, `libvlc_media_player_play` | Use libVLC via C# bindings; provide file/folder picker and drop targets. |
| Native Windows video | `libdirect3d11_plugin.dll`, `libdirect3d9_plugin.dll`; `libvlc_media_player_set_hwnd` | WPF application with embedded native video surface. |
| Audio output and volume | `libwasapi_plugin.dll`, `libdirectsound_plugin.dll`; `libvlc_audio_set_volume` | Device playback plus volume/mute controls. |
| Network playback | `libhttp_plugin.dll`, `libhttps_plugin.dll`; `libvlc_media_new_location` | URL opening with error reporting and network buffering handled by engine. |
| Seeking and speed | `libvlc_media_player_set_time`, `set_position`, `set_rate` | Seek bar, time display, jump commands and speed menu. |
| Playlists | `libplaylist_plugin.dll`; `libvlc_media_list_*` and `libvlc_media_list_player_*` exports | Coneic can own queue/order/repeat/shuffle and save portable playlist files. Queue UI is app behavior. |
| Subtitles and tracks | `libsubsdec_plugin.dll`, `libfreetype_plugin.dll`; `libvlc_media_player_add_slave`, `libvlc_video_set_spu`, `libvlc_video_set_spu_delay`, `libvlc_audio_set_track` | External subtitle opening, track choice and delay adjustment. |
| Image sizing and snapshots | `libvlc_video_set_aspect_ratio`, `libvlc_video_set_crop_geometry`, `libvlc_video_take_snapshot` | Aspect menu and save-frame action. |
| Equalizer | `libvlc_audio_equalizer_*` exports; 25 audio-filter plugin files | Engine equalizer with app preset controls. |
| Convert, broadcast, capture and discovery | Installed directories include 20 `stream_out`, nine `mux`, eight `access_output`, six `services_discovery` files and a WASAPI input plugin. | Separate advanced workflows need explicit UI, option validation and real media/network/device testing. Directory presence does not implement these screens. |

## Reuse and distribution evidence

The installed `COPYING.txt` begins with GNU GPL Version 2, while VideoLAN describes libVLC as LGPL 2.1 in its [libVLC overview](https://www.videolan.org/vlc/libvlc.html) and [relicensing announcement](https://www.videolan.org/press/lgpl-libvlc.html). These are component-level distinctions: the presence of `COPYING.txt` must not be interpreted as a complete license inventory for every plugin or for a new package. Coneic should retain actual package notices and provide corresponding source/license materials appropriate to the components it redistributes. This report is a software-component observation, not a legal determination.

## What remains unverified

- Supplied 3.0.24 installer payload extraction, code paths, signature trust and installer side effects.
- Native decompilation: no Ghidra or IDA provider is configured; no engine was installed.
- Runtime behavior of the existing VLC app: no playback, UI interaction, network request or device access was performed here.
- Screenshot store data: ratings and publisher are supplied image contents; no live store listing was verified.
- Exact codec/DRM/disc/device/protocol coverage, plugin load success and support differences between reference 3.0.23 and other versions.
- Coneic's implemented UI/features and runtime results must be recorded separately by its build/testing work. This report does not certify a complete VLC replacement.

The architecture and engine API boundary are sufficiently evidenced to build an independent player interface. Whole-app feature parity remains a larger checklist, especially conversion/broadcasting, capture, disc menus, renderer discovery, extensions and all advanced VLC settings.
