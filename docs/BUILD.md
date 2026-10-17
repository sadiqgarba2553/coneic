# Windows x64 build

The tested build used WSL2 Ubuntu Base 24.04.5, GCC 13.2, MinGW headers/runtime 11.0.1, Qt and the exact VLC 3.0 contrib dependency archive. Keep source and intermediates on the Linux filesystem; a Windows-mounted checkout is much slower.

## Prerequisites

Install `build-essential mingw-w64 git curl wget ca-certificates autoconf automake libtool libtool-bin pkg-config gettext autopoint bison flex gperf texinfo nasm yasm cmake python3 python3-setuptools meson ninja-build xz-utils bzip2 zip unzip zstd patch dos2unix ant default-jdk-headless help2man protobuf-compiler`. The native tools bootstrap builds the upstream-selected protoc 3.4.1 and jobserver-capable Ninja when needed.

```sh
CONEIC_ENABLE_MINGW_COMPAT=1 CONEIC_JOBS=4 bash coneic/build-win64.sh
```

The wrapper keeps upstream release optimization, GUI, skins, translations and media defaults. It pins:

`https://artifacts.videolan.org/vlc-3.0/win64/vlc-contrib-x86_64-w64-mingw32-910cc9cada6080dd777fbafb2ebdd0c24f702c30.tar.zst`

The observed archive SHA-256 is `49f7690b3d09760f842f9f8e9612c28de8cbb1d027bcb3f6aaa25767611eea8c`. This is a local observation, not a signed publisher checksum. A matching local cache can be supplied through `CONEIC_CONTRIB_ARCHIVE`.

## Compiler compatibility

The contrib archive and Ubuntu toolchain differ. Fourteen exact CRT symbol spellings are probed separately. With the explicit compatibility switch, only missing symbols receive adapters to the existing CRT implementations. Static assertions guard x64 time/stat layouts. The Windows probe passed 36 checks, including metadata buffers, time, multibyte state, errno, 64-bit seek and timezone/argument-storage identity.

`coneic/rebuild-matroska.sh` rebuilds pinned libebml 1.4.6 and libmatroska 1.7.2 when the observed conversion-operator ABI mismatch occurs. `coneic/rebuild-vpx.sh` rebuilds pinned libvpx 1.16.0 when the prebuilt archive's AVX512 symbol prevents codec detection. Upstream contrib SHA-512 manifests verify those downloads. AVX512 dispatch and VP9 high bit depth remain enabled.

The wrapper requires `ENABLE_NLS` and all four VP8/VP9 encode/decode flags. After changing those flags it invalidates the VPX source object's timestamp: changing Makefile flags alone can leave an old object in an incremental build. Do not infer codec availability from a DLL filename.

## Package

The upstream portable package is staged under `win64/vlc-3.0.24`. Debug ZIP generation is provided by the wrapper. For the delivered preview, debug data was stripped from copied player binaries, `vlc.exe` was copied to `Coneic.exe`, and the supplied installer baseline's `axvlc.dll` was retained byte-for-byte. The ActiveX component was not rebuilt or registered.

Use the original installer as a local comparison input, not as a source file to commit. The portable package does not install shell extensions or file associations. Generate the final Windows plugin cache with `vlc-cache-gen.exe plugins` after staging DLLs.

## Local checks

```sh
git fetch --no-tags --depth=1 https://github.com/videolan/vlc.git 6de05adcbaf2e8b85fe86aad4169393098628119
python3 coneic/tests/verify_parity.py --repo . --allow modules/gui/qt/coneic_smoke.hpp --json validation/source.json --markdown validation/source.md
```

On Windows, with the portable app in `App`:

```powershell
python coneic/tests/validate_cli.py --app App --fixtures coneic/tests/fixtures --output validation/native
python coneic/tests/validate_native.py --app App --fixtures coneic/tests/fixtures --output validation/controls --controls-only
```

The full embedding mode deliberately retains the known H.264 failure reproducer. See VALIDATION.md before interpreting it. Native GUI checks opt in through `CONEIC_TEST_OUTPUT`; normal launch does not enable the harness. To test German UI, clear a host `LC_ALL=C.UTF-8` override for the child and pass `--language de`.
