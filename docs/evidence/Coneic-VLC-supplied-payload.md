# Supplied VLC 3.0.24 payload evidence

Observed: 2026-10-16T22:46:11.109934+00:00

**The previously unresolved installer payload is now statically extracted and inventoried.** The installer was not executed. This is ordinary shell extraction/static parsing, separate from REA evidence.

Installer: `/mnt/c/Users/Garba Ibrahim/Desktop/vlc-3.0.24-win64.exe`; **46,269,360 bytes**; SHA-256 `d711e1e1fe52052748c39080c7dce63f6b7e4c315efedf1774a3f1957b782ff3`.

7-Zip 23.01 recognized NSIS-3 Unicode/LZMA and extracted **584 files**, **196,933,784 bytes**, into `/root/coneic/supplied-vlc-reference`.

Windows FileVersionInfo read from extracted `vlc.exe`: FileVersion **3.0.24**, ProductVersion **3,0,24,0**, ProductName **VLC media player**, CompanyName **VideoLAN**. This is the supplied installer payload, distinct from installed VLC 3.0.23.

## Baseline for the Coneic fork

Plugin files: **362**; plugin DLLs: **360**. The companion JSON includes every relative filename, byte size, SHA-256, grouped plugins and selected PE imports/exports. Compare packaged plugin paths to identify missing/added modules; compare capabilities and build configuration before treating differences as regressions. DLL hashes will normally differ in a rebuilt/styled fork.

| Image | Architecture | Imports | Named exports |
| --- | --- | --- | ---: |
| `vlc.exe` | PE32+, 0x8664 | `libvlc.dll`, `ADVAPI32.dll`, `KERNEL32.dll`, `msvcrt.dll`, `SHELL32.dll`, `USER32.dll`, `WININET.dll` | 1 |
| `libvlc.dll` | PE32+, 0x8664 | `libvlccore.dll`, `ADVAPI32.dll`, `KERNEL32.dll`, `msvcrt.dll` | 316 |
| `libvlccore.dll` | PE32+, 0x8664 | `ADVAPI32.dll`, `KERNEL32.dll`, `msvcrt.dll`, `SHELL32.dll`, `USER32.dll`, `WS2_32.dll` | 764 |
| `axvlc.dll` | PE32+, 0x8664 | `libvlc.dll`, `ADVAPI32.dll`, `GDI32.dll`, `KERNEL32.dll`, `msvcrt.dll`, `ole32.dll`, `OLEAUT32.dll`, `SHLWAPI.dll`, `USER32.dll` | 5 |
| `plugins/gui/libqt_plugin.dll` | PE32+, 0x8664 | `libvlccore.dll`, `ADVAPI32.dll`, `COMCTL32.dll`, `GDI32.dll`, `IMM32.dll`, `KERNEL32.dll`, `msvcrt.dll`, `ole32.dll`, `OLEAUT32.dll`, `SHELL32.dll`, `USER32.dll`, `WINMM.dll`, `WS2_32.dll` | 3 |

## Optional integrations

`axvlc.dll` **is present** (ActiveX integration). A file named `npvlc.dll` **is absent from all 584 extracted files**; this bounded absence must not be described as missing desktop playback functionality. NSIS helper DLLs are under `$PLUGINSDIR/` and are installer helpers, not VLC playback plugins.

## Plugin groups and exact files

| Group | Files | Names |
| --- | ---: | --- |
| `access` | 37 | `libaccess_concat_plugin.dll`, `libaccess_imem_plugin.dll`, `libaccess_mms_plugin.dll`, `libaccess_srt_plugin.dll`, `libaccess_wasapi_plugin.dll`, `libattachment_plugin.dll`, `libbluray-awt-j2se-1.4.1.jar`, `libbluray-j2se-1.4.1.jar`, `libcdda_plugin.dll`, `libdcp_plugin.dll`, `libdshow_plugin.dll`, `libdtv_plugin.dll`, `libdvdnav_plugin.dll`, `libdvdread_plugin.dll`, `libfilesystem_plugin.dll`, `libftp_plugin.dll`, `libhttp_plugin.dll`, `libhttps_plugin.dll`, `libidummy_plugin.dll`, `libimem_plugin.dll`, `liblibbluray_plugin.dll`, `liblive555_plugin.dll`, `libnfs_plugin.dll`, `librist_plugin.dll`, `librtp_plugin.dll`, `libsatip_plugin.dll`, `libscreen_plugin.dll`, `libsdp_plugin.dll`, `libsftp_plugin.dll`, `libshm_plugin.dll`, `libsmb_plugin.dll`, `libtcp_plugin.dll`, `libtimecode_plugin.dll`, `libudp_plugin.dll`, `libvcd_plugin.dll`, `libvdr_plugin.dll`, `libvnc_plugin.dll` |
| `access_output` | 8 | `libaccess_output_dummy_plugin.dll`, `libaccess_output_file_plugin.dll`, `libaccess_output_http_plugin.dll`, `libaccess_output_livehttp_plugin.dll`, `libaccess_output_rist_plugin.dll`, `libaccess_output_shout_plugin.dll`, `libaccess_output_srt_plugin.dll`, `libaccess_output_udp_plugin.dll` |
| `audio_filter` | 25 | `libaudio_format_plugin.dll`, `libaudiobargraph_a_plugin.dll`, `libchorus_flanger_plugin.dll`, `libcompressor_plugin.dll`, `libdolby_surround_decoder_plugin.dll`, `libequalizer_plugin.dll`, `libgain_plugin.dll`, `libheadphone_channel_mixer_plugin.dll`, `libkaraoke_plugin.dll`, `libmad_plugin.dll`, `libmono_plugin.dll`, `libnormvol_plugin.dll`, `libparam_eq_plugin.dll`, `libremap_plugin.dll`, `libsamplerate_plugin.dll`, `libscaletempo_pitch_plugin.dll`, `libscaletempo_plugin.dll`, `libsimple_channel_mixer_plugin.dll`, `libspatialaudio_plugin.dll`, `libspatializer_plugin.dll`, `libspeex_resampler_plugin.dll`, `libstereo_widen_plugin.dll`, `libtospdif_plugin.dll`, `libtrivial_channel_mixer_plugin.dll`, `libugly_resampler_plugin.dll` |
| `audio_mixer` | 2 | `libfloat_mixer_plugin.dll`, `libinteger_mixer_plugin.dll` |
| `audio_output` | 7 | `libadummy_plugin.dll`, `libafile_plugin.dll`, `libamem_plugin.dll`, `libdirectsound_plugin.dll`, `libmmdevice_plugin.dll`, `libwasapi_plugin.dll`, `libwaveout_plugin.dll` |
| `codec` | 56 | `libadpcm_plugin.dll`, `libaes3_plugin.dll`, `libaom_plugin.dll`, `libaraw_plugin.dll`, `libaribsub_plugin.dll`, `libavcodec_plugin.dll`, `libcc_plugin.dll`, `libcdg_plugin.dll`, `libcrystalhd_plugin.dll`, `libcvdsub_plugin.dll`, `libd3d11va_plugin.dll`, `libdav1d_plugin.dll`, `libddummy_plugin.dll`, `libdmo_plugin.dll`, `libdvbsub_plugin.dll`, `libdxva2_plugin.dll`, `libedummy_plugin.dll`, `libfaad_plugin.dll`, `libflac_plugin.dll`, `libfluidsynth_plugin.dll`, `libg711_plugin.dll`, `libjpeg_plugin.dll`, `libkate_plugin.dll`, `liblibass_plugin.dll`, `liblpcm_plugin.dll`, `libmft_plugin.dll`, `libmpg123_plugin.dll`, `liboggspots_plugin.dll`, `libopus_plugin.dll`, `libpng_plugin.dll`, `libqsv_plugin.dll`, `librawvideo_plugin.dll`, `librtpvideo_plugin.dll`, `libscte18_plugin.dll`, `libscte27_plugin.dll`, `libspdif_plugin.dll`, `libspeex_plugin.dll`, `libspudec_plugin.dll`, `libstl_plugin.dll`, `libsubsdec_plugin.dll`, `libsubstx3g_plugin.dll`, `libsubsusf_plugin.dll`, `libsvcdsub_plugin.dll`, `libt140_plugin.dll`, `libtextst_plugin.dll`, `libtheora_plugin.dll`, `libttml_plugin.dll`, `libtwolame_plugin.dll`, `libuleaddvaudio_plugin.dll`, `libvorbis_plugin.dll`, `libvpx_plugin.dll`, `libwebvtt_plugin.dll`, `libx26410b_plugin.dll`, `libx264_plugin.dll`, `libx265_plugin.dll`, `libzvbi_plugin.dll` |
| `control` | 8 | `libdummy_plugin.dll`, `libgestures_plugin.dll`, `libhotkeys_plugin.dll`, `libnetsync_plugin.dll`, `libntservice_plugin.dll`, `liboldrc_plugin.dll`, `libwin_hotkeys_plugin.dll`, `libwin_msg_plugin.dll` |
| `d3d11` | 1 | `libdirect3d11_filters_plugin.dll` |
| `d3d9` | 1 | `libdirect3d9_filters_plugin.dll` |
| `demux` | 47 | `libadaptive_plugin.dll`, `libaiff_plugin.dll`, `libasf_plugin.dll`, `libau_plugin.dll`, `libavi_plugin.dll`, `libcaf_plugin.dll`, `libdemux_cdg_plugin.dll`, `libdemux_chromecast_plugin.dll`, `libdemux_stl_plugin.dll`, `libdemuxdump_plugin.dll`, `libdiracsys_plugin.dll`, `libdirectory_demux_plugin.dll`, `libdmxmus_plugin.dll`, `libes_plugin.dll`, `libflacsys_plugin.dll`, `libgme_plugin.dll`, `libh26x_plugin.dll`, `libimage_plugin.dll`, `libmjpeg_plugin.dll`, `libmkv_plugin.dll`, `libmod_plugin.dll`, `libmp4_plugin.dll`, `libmpc_plugin.dll`, `libmpgv_plugin.dll`, `libnoseek_plugin.dll`, `libnsc_plugin.dll`, `libnsv_plugin.dll`, `libnuv_plugin.dll`, `libogg_plugin.dll`, `libplaylist_plugin.dll`, `libps_plugin.dll`, `libpva_plugin.dll`, `librawaud_plugin.dll`, `librawdv_plugin.dll`, `librawvid_plugin.dll`, `libreal_plugin.dll`, `libsid_plugin.dll`, `libsmf_plugin.dll`, `libsubtitle_plugin.dll`, `libts_plugin.dll`, `libtta_plugin.dll`, `libty_plugin.dll`, `libvc1_plugin.dll`, `libvobsub_plugin.dll`, `libvoc_plugin.dll`, `libwav_plugin.dll`, `libxa_plugin.dll` |
| `gui` | 2 | `libqt_plugin.dll`, `libskins2_plugin.dll` |
| `keystore` | 2 | `libfile_keystore_plugin.dll`, `libmemory_keystore_plugin.dll` |
| `logger` | 2 | `libconsole_logger_plugin.dll`, `libfile_logger_plugin.dll` |
| `lua` | 1 | `liblua_plugin.dll` |
| `meta_engine` | 2 | `libfolder_plugin.dll`, `libtaglib_plugin.dll` |
| `misc` | 10 | `libaddonsfsstorage_plugin.dll`, `libaddonsvorepository_plugin.dll`, `libaudioscrobbler_plugin.dll`, `libexport_plugin.dll`, `libfingerprinter_plugin.dll`, `libgnutls_plugin.dll`, `liblogger_plugin.dll`, `libstats_plugin.dll`, `libvod_rtsp_plugin.dll`, `libxml_plugin.dll` |
| `mux` | 9 | `libmux_asf_plugin.dll`, `libmux_avi_plugin.dll`, `libmux_dummy_plugin.dll`, `libmux_mp4_plugin.dll`, `libmux_mpjpeg_plugin.dll`, `libmux_ogg_plugin.dll`, `libmux_ps_plugin.dll`, `libmux_ts_plugin.dll`, `libmux_wav_plugin.dll` |
| `packetizer` | 14 | `libpacketizer_a52_plugin.dll`, `libpacketizer_av1_plugin.dll`, `libpacketizer_copy_plugin.dll`, `libpacketizer_dirac_plugin.dll`, `libpacketizer_dts_plugin.dll`, `libpacketizer_flac_plugin.dll`, `libpacketizer_h264_plugin.dll`, `libpacketizer_hevc_plugin.dll`, `libpacketizer_mlp_plugin.dll`, `libpacketizer_mpeg4audio_plugin.dll`, `libpacketizer_mpeg4video_plugin.dll`, `libpacketizer_mpegaudio_plugin.dll`, `libpacketizer_mpegvideo_plugin.dll`, `libpacketizer_vc1_plugin.dll` |
| `services_discovery` | 6 | `libmediadirs_plugin.dll`, `libmicrodns_plugin.dll`, `libpodcast_plugin.dll`, `libsap_plugin.dll`, `libupnp_plugin.dll`, `libwindrive_plugin.dll` |
| `spu` | 7 | `libaudiobargraph_v_plugin.dll`, `liblogo_plugin.dll`, `libmarq_plugin.dll`, `libmosaic_plugin.dll`, `libremoteosd_plugin.dll`, `librss_plugin.dll`, `libsubsdelay_plugin.dll` |
| `stream_extractor` | 1 | `libarchive_plugin.dll` |
| `stream_filter` | 9 | `libadf_plugin.dll`, `libaribcam_plugin.dll`, `libcache_block_plugin.dll`, `libcache_read_plugin.dll`, `libhds_plugin.dll`, `libinflate_plugin.dll`, `libprefetch_plugin.dll`, `librecord_plugin.dll`, `libskiptags_plugin.dll` |
| `stream_out` | 20 | `libstream_out_autodel_plugin.dll`, `libstream_out_bridge_plugin.dll`, `libstream_out_chromaprint_plugin.dll`, `libstream_out_chromecast_plugin.dll`, `libstream_out_cycle_plugin.dll`, `libstream_out_delay_plugin.dll`, `libstream_out_description_plugin.dll`, `libstream_out_display_plugin.dll`, `libstream_out_dummy_plugin.dll`, `libstream_out_duplicate_plugin.dll`, `libstream_out_es_plugin.dll`, `libstream_out_gather_plugin.dll`, `libstream_out_mosaic_bridge_plugin.dll`, `libstream_out_record_plugin.dll`, `libstream_out_rtp_plugin.dll`, `libstream_out_setid_plugin.dll`, `libstream_out_smem_plugin.dll`, `libstream_out_standard_plugin.dll`, `libstream_out_stats_plugin.dll`, `libstream_out_transcode_plugin.dll` |
| `text_renderer` | 3 | `libfreetype_plugin.dll`, `libsapi_plugin.dll`, `libtdummy_plugin.dll` |
| `video_chroma` | 19 | `libchain_plugin.dll`, `libgrey_yuv_plugin.dll`, `libi420_10_p010_plugin.dll`, `libi420_nv12_plugin.dll`, `libi420_rgb_mmx_plugin.dll`, `libi420_rgb_plugin.dll`, `libi420_rgb_sse2_plugin.dll`, `libi420_yuy2_mmx_plugin.dll`, `libi420_yuy2_plugin.dll`, `libi420_yuy2_sse2_plugin.dll`, `libi422_i420_plugin.dll`, `libi422_yuy2_mmx_plugin.dll`, `libi422_yuy2_plugin.dll`, `libi422_yuy2_sse2_plugin.dll`, `librv32_plugin.dll`, `libswscale_plugin.dll`, `libyuvp_plugin.dll`, `libyuy2_i420_plugin.dll`, `libyuy2_i422_plugin.dll` |
| `video_filter` | 41 | `libadjust_plugin.dll`, `libalphamask_plugin.dll`, `libanaglyph_plugin.dll`, `libantiflicker_plugin.dll`, `libball_plugin.dll`, `libblend_plugin.dll`, `libblendbench_plugin.dll`, `libbluescreen_plugin.dll`, `libcanvas_plugin.dll`, `libcolorthres_plugin.dll`, `libcroppadd_plugin.dll`, `libdeinterlace_plugin.dll`, `libedgedetection_plugin.dll`, `liberase_plugin.dll`, `libextract_plugin.dll`, `libfps_plugin.dll`, `libfreeze_plugin.dll`, `libgaussianblur_plugin.dll`, `libgradfun_plugin.dll`, `libgradient_plugin.dll`, `libgrain_plugin.dll`, `libhqdn3d_plugin.dll`, `libinvert_plugin.dll`, `libmagnify_plugin.dll`, `libmirror_plugin.dll`, `libmotionblur_plugin.dll`, `libmotiondetect_plugin.dll`, `liboldmovie_plugin.dll`, `libposterize_plugin.dll`, `libpostproc_plugin.dll`, `libpsychedelic_plugin.dll`, `libpuzzle_plugin.dll`, `libripple_plugin.dll`, `librotate_plugin.dll`, `libscale_plugin.dll`, `libscene_plugin.dll`, `libsepia_plugin.dll`, `libsharpen_plugin.dll`, `libtransform_plugin.dll`, `libvhs_plugin.dll`, `libwave_plugin.dll` |
| `video_output` | 15 | `libcaca_plugin.dll`, `libdirect3d11_plugin.dll`, `libdirect3d9_plugin.dll`, `libdirectdraw_plugin.dll`, `libdrawable_plugin.dll`, `libflaschen_plugin.dll`, `libgl_plugin.dll`, `libglinterop_dxva2_plugin.dll`, `libglwin32_plugin.dll`, `libvdummy_plugin.dll`, `libvmem_plugin.dll`, `libwgl_plugin.dll`, `libwingdi_plugin.dll`, `libwinhibit_plugin.dll`, `libyuv_plugin.dll` |
| `video_splitter` | 3 | `libclone_plugin.dll`, `libpanoramix_plugin.dll`, `libwall_plugin.dll` |
| `visualization` | 4 | `libglspectrum_plugin.dll`, `libgoom_plugin.dll`, `libprojectm_plugin.dll`, `libvisual_plugin.dll` |

## Limits

- Static presence and PE imports/exports do not establish load success or runtime behavior.
- NSIS extraction reflects embedded payload, not installer choices, generated files, registration or side effects.
- Version resource strings are declarations, not an independent publisher signature verification.
- Plugin names/hashes form a supplied3.0.24 comparison baseline; different toolchains produce different hashes.

The earlier `vlc-architecture.md` extraction limitation records the earlier tool boundary; this report supersedes that unresolved payload inventory. Prior installed-3.0.23 observations remain separate reference evidence.
