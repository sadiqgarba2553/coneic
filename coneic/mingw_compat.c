/* SPDX-License-Identifier: GPL-2.0-or-later
 * Opt-in adapters for precisely probed missing MinGW symbols.
 * Compile with only the CONEIC_NEED_* definitions selected by build-win64.sh.
 */
#include <sys/stat.h>
#include <sys/timeb.h>
#include <wchar.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#if !defined(_WIN64) || defined(_UCRT) || defined(_USE_32BIT_TIME_T)
#error Coneic compatibility adapters require x64 MSVCRT with 64-bit time
#endif
_Static_assert(sizeof(struct _stat64i32) == 48, "Unexpected _stat64i32 ABI");
_Static_assert(offsetof(struct _stat64i32, st_size) == 20, "Unexpected stat size offset");
_Static_assert(offsetof(struct _stat64i32, st_atime) == 24, "Unexpected stat time offset");
_Static_assert(sizeof(struct _stat64) == 56, "Unexpected _stat64 ABI");
_Static_assert(offsetof(struct _stat64, st_size) == 24, "Unexpected stat64 size offset");
_Static_assert(offsetof(struct _stat64, st_atime) == 32, "Unexpected stat64 time offset");
_Static_assert(sizeof(struct __timeb64) == 16, "Unexpected __timeb64 ABI");
_Static_assert(offsetof(struct __timeb64, millitm) == 8, "Unexpected millisecond offset");
#ifdef CONEIC_NEED_FSTAT64I32
#undef fstat64i32
int __cdecl fstat64i32(int fd, struct _stat64i32 *st) { return _fstat64i32(fd, st); }
#endif
#ifdef CONEIC_NEED_STAT64
#undef stat64
int __cdecl stat64(const char *path, struct _stat64 *st) { return _stat64(path, st); }
#endif
#ifdef CONEIC_NEED_FSTAT64
#undef fstat64
int __cdecl fstat64(int fd, struct _stat64 *st) { return _fstat64(fd, st); }
#endif
#ifdef CONEIC_NEED_STAT64I32
#undef stat64i32
int __cdecl stat64i32(const char *path, struct _stat64i32 *st) { return _stat64i32(path, st); }
#endif
#ifdef CONEIC_NEED_FTIME64
#undef ftime64
/* Same int-return contract as upstream mingw-w64-crt/misc/ftime64.c. */
int __cdecl ftime64(struct __timeb64 *value) { _ftime64(value); return 0; }
#endif
#ifdef CONEIC_NEED_IMP_WCRTOMB
size_t (__cdecl *__imp_wcrtomb)(char *, wchar_t, mbstate_t *) = wcrtomb;
#endif
#ifdef CONEIC_NEED_IMP_MBRTOWC
size_t (__cdecl *__imp_mbrtowc)(wchar_t *, const char *, size_t, mbstate_t *) = mbrtowc;
#endif
#ifdef CONEIC_NEED_IMP_MBRLEN
size_t (__cdecl *__imp_mbrlen)(const char *, size_t, mbstate_t *) = mbrlen;
#endif
#ifdef CONEIC_NEED_IMP_FTELLO64
__int64 (__cdecl *__imp_ftello64)(FILE *) = ftello64;
#endif
#ifdef CONEIC_NEED_IMP_FSEEKO64
int (__cdecl *__imp_fseeko64)(FILE *, __int64, int) = fseeko64;
#endif
#ifdef CONEIC_NEED_IMP_SET_ERRNO
errno_t (__cdecl *__imp__set_errno)(int) = _set_errno;
#endif
#ifdef CONEIC_NEED_IMP_TIMEZONE
/* Mirrors MinGW's __timezone adapter: return the CRT's imported data
 * address, not a cached value or a pointer to a separate storage object. */
extern long *__imp__timezone;
static long *__cdecl coneic_timezone_address(void) { return __imp__timezone; }
long *(__cdecl *__imp___timezone)(void) = coneic_timezone_address;
#endif
#ifdef CONEIC_NEED_IMP_DAYLIGHT
extern int *__imp__daylight;
static int *__cdecl coneic_daylight_address(void) { return __imp__daylight; }
int *(__cdecl *__imp___daylight)(void) = coneic_daylight_address;
#endif
#ifdef CONEIC_NEED_IMP_ARGC
extern int *__imp___argc;
static int *__cdecl coneic_argc_address(void) { return __imp___argc; }
int *(__cdecl *__imp___p___argc)(void) = coneic_argc_address;
#endif
