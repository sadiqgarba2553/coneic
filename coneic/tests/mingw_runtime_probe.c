/* SPDX-License-Identifier: GPL-2.0-or-later
 * Tests only Coneic's MinGW adapters. It does not certify libVLC/plugin parity.
 * Run with one dedicated scratch file path; that file is created then removed.
 */
#include <windows.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <wchar.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <locale.h>
#include <io.h>
#include <fcntl.h>
#include <stddef.h>
#include <time.h>
#include <stdlib.h>

extern int coneic_fstat64i32(int, struct _stat64i32 *) __asm__("fstat64i32");
extern int coneic_fstat64(int, struct _stat64 *) __asm__("fstat64");
extern int coneic_stat64(const char *, struct _stat64 *) __asm__("stat64");
extern int coneic_stat64i32(const char *, struct _stat64i32 *) __asm__("stat64i32");
extern int coneic_ftime64(struct __timeb64 *) __asm__("ftime64");
extern size_t (__cdecl *__imp_wcrtomb)(char *, wchar_t, mbstate_t *);
extern size_t (__cdecl *__imp_mbrtowc)(wchar_t *, const char *, size_t, mbstate_t *);
extern size_t (__cdecl *__imp_mbrlen)(const char *, size_t, mbstate_t *);
extern __int64 (__cdecl *__imp_ftello64)(FILE *);
extern int (__cdecl *__imp_fseeko64)(FILE *, __int64, int);
extern errno_t (__cdecl *__imp__set_errno)(int);
extern long *(__cdecl *__imp___timezone)(void);
extern int *(__cdecl *__imp___daylight)(void);
extern int *(__cdecl *__imp___p___argc)(void);

static int failures;
#define CHECK(test, label) do { int ok = !!(test); printf("%s %s\n", ok ? "PASS" : "FAIL", label); if (!ok) failures++; } while (0)
#define CANARY UINT64_C(0xa5a5a5a5a5a5a5a5)

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "Usage: probe.exe <new scratch file path>\n"); return 2; }
    /* Never overwrite a caller's existing file. */
    int fd = _open(argv[1], _O_CREAT | _O_EXCL | _O_RDWR | _O_BINARY, _S_IREAD | _S_IWRITE);
    if (fd < 0) { perror("create scratch file"); return 2; }
    CHECK(_write(fd, "abc", 3) == 3, "scratch write");
    CHECK(_close(fd) == 0, "close fixture before path metadata queries");
    fd = -1;
    struct { uint64_t a; struct _stat64 v; uint64_t z; } s64;
    struct { uint64_t a; struct _stat64i32 v; uint64_t z; } s32;
    struct { uint64_t a; struct __timeb64 v; uint64_t z; } tb;
    memset(&s64, 0xa5, sizeof s64); memset(&s32, 0xa5, sizeof s32); memset(&tb, 0xa5, sizeof tb);
    struct _stat64 baseline_stat;
    CHECK(_stat64(argv[1], &baseline_stat) == 0 && baseline_stat.st_size == 3, "underlying CRT stat64 fixture size");
    CHECK(coneic_stat64(argv[1], &s64.v) == 0 && s64.v.st_size == 3, "stat64 size");
    CHECK(s64.a == CANARY && s64.z == CANARY, "stat64 buffer guards");
    CHECK(coneic_stat64i32(argv[1], &s32.v) == 0 && s32.v.st_size == 3, "stat64i32 size");
    CHECK(s32.a == CANARY && s32.z == CANARY, "stat64i32 buffer guards");
    fd = _open(argv[1], _O_RDWR | _O_BINARY);
    CHECK(fd >= 0, "reopen fixture for descriptor queries");
    CHECK(fd >= 0 && coneic_fstat64i32(fd, &s32.v) == 0 && s32.v.st_size == 3, "fstat64i32 size");
    CHECK(s32.a == CANARY && s32.z == CANARY, "fstat64i32 buffer guards");
    CHECK(fd >= 0 && coneic_fstat64(fd, &s64.v) == 0 && s64.v.st_size == 3, "fstat64 size");
    CHECK(s64.a == CANARY && s64.z == CANARY, "fstat64 buffer guards");
    CHECK(coneic_stat64("Z:\\Coneic-no-such-path-7d94d1\\missing", &s64.v) == -1, "stat64 missing path");
    CHECK(coneic_ftime64(&tb.v) == 0, "ftime64 int success return");
    CHECK(tb.v.time > INT64_C(1500000000) && tb.v.millitm <= 999, "ftime64 plausible time");
    CHECK(tb.a == CANARY && tb.z == CANARY, "ftime64 buffer guards");
    CHECK(__imp__set_errno(EINVAL) == 0 && errno == EINVAL, "import errno adapter");
    setlocale(LC_CTYPE, "C");
    struct { uint64_t a; mbstate_t v; uint64_t z; } state;
    memset(&state, 0, sizeof state); state.a = CANARY; state.z = CANARY;
    char mb[16] = {0}; wchar_t wide = 0;
    CHECK(__imp_wcrtomb(mb, L'A', &state.v) == 1 && mb[0] == 'A', "wcrtomb ASCII");
    memset(&state.v, 0, sizeof state.v);
    CHECK(__imp_mbrtowc(&wide, "A", 1, &state.v) == 1 && wide == L'A', "mbrtowc ASCII");
    memset(&state.v, 0, sizeof state.v);
    CHECK(__imp_mbrlen("A", 1, &state.v) == 1, "mbrlen ASCII");
    CHECK(__imp_mbrlen("", 1, &state.v) == 0, "mbrlen null character");
    CHECK(state.a == CANARY && state.z == CANARY, "multibyte state guards");
    char *old_tz = getenv("TZ");
    char *saved_tz = old_tz ? _strdup(old_tz) : NULL;
    _tzset();
    CHECK(__imp___timezone() == &_timezone, "timezone adapter returns actual CRT storage");
    CHECK(__imp___daylight() == &_daylight, "daylight adapter returns actual CRT storage");
    CHECK(_putenv_s("TZ", "UTC0") == 0, "set process-only UTC timezone");
    _tzset();
    CHECK(*__imp___timezone() == 0 && *__imp___daylight() == 0, "timezone/daylight UTC values");
    CHECK(_putenv_s("TZ", "PST8PDT") == 0, "set process-only daylight timezone");
    _tzset();
    CHECK(*__imp___timezone() == 28800 && *__imp___daylight() != 0, "timezone/daylight reflect CRT updates");
    CHECK(_putenv_s("TZ", saved_tz ? saved_tz : "") == 0, "restore process timezone environment");
    free(saved_tz);
    _tzset();
    CHECK(__imp___p___argc() == &__argc, "argc adapter returns actual CRT storage");
    CHECK(*__imp___p___argc() == argc, "argc adapter value matches process arguments");
    FILE *stream = fd >= 0 ? _fdopen(fd, "r+b") : NULL;
    CHECK(stream != NULL, "fdopen");
    if (stream) {
        const __int64 offset = INT64_C(4294967327);
        CHECK(__imp_fseeko64(stream, offset, SEEK_SET) == 0, "fseeko64 beyond 4 GiB without writing");
        CHECK(__imp_ftello64(stream) == offset, "ftello64 preserves 64-bit position");
        CHECK(__imp_fseeko64(stream, 0, SEEK_SET) == 0 && fgetc(stream) == 'a', "seek back and read");
        fclose(stream);
    } else if (fd >= 0) _close(fd);
    CHECK(DeleteFileA(argv[1]) != 0, "scratch cleanup");
    printf("ABI sizes: stat64i32=%zu stat64=%zu timeb64=%zu mbstate=%zu\n",
        sizeof(struct _stat64i32), sizeof(struct _stat64), sizeof(struct __timeb64), sizeof(mbstate_t));
    printf("RESULT failures=%d\n", failures);
    return failures ? 1 : 0;
}
