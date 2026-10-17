# Coneic MinGW compatibility audit

This review concerns `coneic/mingw_compat.c` and `coneic/build-win64.sh` in the Desktop VLC fork. It does not certify VLC playback or all plugin behavior.

## Finding corrected

The original `ftime64` adapter returned `void`. Newer MinGW declares POSIX `ftime` through a linker alias to **int-returning** `ftime64`; a caller checking success would receive an unspecified register value from the old wrapper. The revised adapter now exactly follows upstream MinGW's [implementation](https://github.com/mingw-w64/mingw-w64/blob/master/mingw-w64-crt/misc/ftime64.c): call `_ftime64`, then return zero. Upstream adds no separate errno or null-pointer handling.

## Struct ABI evidence

The local WSL cross compiler is GCC 13 with the win32 thread model. Its installed `/usr/x86_64-w64-mingw32/include/_mingw_stat64.h` fixed-size structure declarations match the fields in [upstream MinGW's header](https://github.com/mingw-w64/mingw-w64/blob/master/mingw-w64-headers/crt/_mingw_stat64.h). The [timeb header](https://github.com/mingw-w64/mingw-w64/blob/master/mingw-w64-headers/crt/sys/timeb.h) likewise uses the same time64/millisecond/timezone fields. The contrib tree does not include its producing toolchain's `sys/stat.h`; that exact build-time header was unavailable, so this is corroboration against current upstream declarations rather than a recovered complete producer environment.

The revised shim compiles static assertions for this x64 MSVCRT ABI:

| Structure | Size | Relevant offsets |
| --- | ---: | --- |
| `_stat64i32` | 48 bytes | 32-bit `st_size`: 20; 64-bit `st_atime`: 24 |
| `_stat64` | 56 bytes | 64-bit `st_size`: 24; 64-bit `st_atime`: 32 |
| `__timeb64` | 16 bytes | `millitm`: 8 |

The adapters call the fixed-size `_stat64`, `_stat64i32` and `_fstat64i32` variants, avoiding a generic `stat` alias whose layout can vary with `_FILE_OFFSET_BITS` or time settings. A compile-time guard rejects non-x64, UCRT or 32-bit-time configurations. This does not establish compatibility with every possible MinGW release.

## Build-driver changes

Each of thirteen required symbols is now linked independently with its exact assembly spelling, avoiding header macros that could silently redirect a probe to another symbol. Only missing symbols enable corresponding `CONEIC_NEED_*` definitions. Compatibility fallback requires explicit `CONEIC_ENABLE_MINGW_COMPAT=1`; otherwise a matching upstream toolchain is requested. The driver records compiler identity, selected definitions and each failed-probe linker log.

The adapters cover `fstat64i32`, `fstat64`, `stat64`, `stat64i32`, `ftime64`, and import-pointer spellings for `wcrtomb`, `mbrtowc`, `ftello64`, `fseeko64`, `_set_errno`, `__timezone`, `__daylight` and `__p___argc`. Compiler/layout guards and per-symbol gating reduce accidental reuse; they are not a general ABI translation layer.

The new timezone/daylight/argument adapters return the CRT's actual imported data addresses, matching upstream implementations for [timezone](https://github.com/mingw-w64/mingw-w64/blob/master/mingw-w64-crt/misc/__timezone.c), [daylight](https://github.com/mingw-w64/mingw-w64/blob/master/mingw-w64-crt/misc/__daylight.c), and [argc](https://github.com/mingw-w64/mingw-w64/blob/master/mingw-w64-crt/misc/__p___argc.c). They do not return pointers to cached copies. The Windows probe checks address identity and updates across process-local UTC/PST timezone changes, then restores its environment.

The supplied contrib hash is checked only when `CONEIC_CONTRIB_ARCHIVE` is provided. For reproducible builds, use that local hash-verified input; the upstream download fallback itself does not perform this wrapper's pinned hash check. Retain the compiler/packages manifest, fetched source/submodule revisions, configure log and final package inventory. A matching upstream CI toolchain remains the preferred way to avoid compatibility adapters entirely.

## Validation performed

- `bash -n` accepted the revised driver.
- All thirteen gated adapters compiled into a fresh audit object using `-std=c11 -Wall -Wextra -Werror -D__MSVCRT_VERSION__=0x700` with the WSL cross compiler.
- The static assertions above passed compilation.
- `nm` confirmed the initial nine definitions and expected underlying CRT references; the four subsequent definitions were confirmed by linking and the extended runtime probe.
- The independent Windows runtime probe linked successfully against that audit object.

No running build was interrupted, and no files in `<BUILD_SOURCE>` were changed by this audit.

The revised standalone probe subsequently ran on Windows with **34 passing assertions, zero failures, exit code 0**. Results are saved in `Coneic-VLC-validation/mingw-runtime-results.txt`. Measured ABI sizes were stat64i32=48, stat64=56, timeb64=16 and mbstate=4. The first probe fixture queried path metadata while its creation handle remained open; closing that handle before path queries and adding an underlying-CRT baseline resolved those fixture failures without changing the production stat adapters. The PowerShell reporter now flattens output lines correctly.

Installed package identities: `mingw-w64-common` and `mingw-w64-x86-64-dev` version **11.0.1-3build1**; `gcc-mingw-w64-x86-64-win32` version **13.2.0-6ubuntu1+26.1**.

## Prepared runtime probe

Workspace files:

- `work/vlc-fork/mingw_runtime_probe.c`
- `work/vlc-fork/mingw_runtime_probe.exe`
- `work/vlc-fork/run-mingw-runtime-probe.ps1`

Run the PowerShell script on Windows to capture an executable hash, timestamp, exit code and assertions. It creates a uniquely named scratch file with exclusive-create semantics and removes that file. It checks stat sizes and surrounding buffer guards, missing-path failure, ftime return/time fields, import-pointer calls, errno assignment, ASCII multibyte conversion, and a seek/tell beyond 4 GiB without writing at that offset.

After adapter checks pass, meaningful application checks still include GnuTLS-backed HTTPS playback, non-ASCII paths/metadata, large-media seeking, playlist parsing and plugin-loading errors. A passing adapter probe does not prove those application workflows.
