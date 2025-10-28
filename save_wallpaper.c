#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shlobj.h>     // SHGetKnownFolderPath
#include <shellapi.h>
#include <wchar.h>
#include <stdio.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")

// --- Yardımcılar ---
static void join_path(wchar_t *dst, size_t dstsz, const wchar_t *a, const wchar_t *b) {
    wcsncpy(dst, a, dstsz - 1);
    dst[dstsz - 1] = L'\0';
    size_t len = wcslen(dst);
    if (len > 0 && dst[len - 1] != L'\\') {
        if (len + 1 < dstsz) {
            dst[len] = L'\\'; dst[len + 1] = L'\0'; len++;
        }
    }
    wcsncat(dst, b, dstsz - len - 1);
}

static BOOL ensure_dir(const wchar_t *path) {
    return CreateDirectoryW(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

static BOOL file_exists(const wchar_t *path) {
    DWORD a = GetFileAttributesW(path);
    return (a != INVALID_FILE_ATTRIBUTES) && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static BOOL ends_with_icase(const wchar_t *s, const wchar_t *ext) {
    size_t ls = wcslen(s), le = wcslen(ext);
    if (le == 0 || ls < le) return FALSE;
    return _wcsicmp(s + (ls - le), ext) == 0;
}

// --- 1) CachedFiles: SON EKLENEN dosyayı (CreationTime) bul ---
static BOOL find_latest_added_from_cached(const wchar_t *srcDir, wchar_t *outPath, size_t outSize) {
    const wchar_t *allowed_exts[] = { L".jpg", L".jpeg", L".png", L".bmp", L".jfif" };
    const int N_EXT = (int)(sizeof(allowed_exts) / sizeof(allowed_exts[0]));

    wchar_t pattern[MAX_PATH]; pattern[0] = L'\0';
    join_path(pattern, MAX_PATH, srcDir, L"*");

    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return FALSE;

    FILETIME bestCreate = (FILETIME){0,0};
    BOOL found = FALSE;

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        BOOL ok = FALSE;
        for (int i = 0; i < N_EXT; ++i) {
            if (ends_with_icase(fd.cFileName, allowed_exts[i])) { ok = TRUE; break; }
        }
        if (!ok) continue;

        if (!found || CompareFileTime(&fd.ftCreationTime, &bestCreate) > 0) {
            bestCreate = fd.ftCreationTime;
            wchar_t candidate[MAX_PATH]; candidate[0] = L'\0';
            join_path(candidate, MAX_PATH, srcDir, fd.cFileName);
            wcsncpy(outPath, candidate, outSize - 1);
            outPath[outSize - 1] = L'\0';
            found = TRUE;
        }
    } while (FindNextFileW(h, &fd));

    FindClose(h);
    return found;
}

int wmain(void) {
    HRESULT hr;
    PWSTR roaming = NULL, desktop = NULL;
    int rc = 1;

    // Known folders
    if (FAILED(SHGetKnownFolderPath(&FOLDERID_RoamingAppData, 0, NULL, &roaming))) { wprintf(L"[HATA] Roaming yok.\n"); goto done; }
    if (FAILED(SHGetKnownFolderPath(&FOLDERID_Desktop, 0, NULL, &desktop)))       { wprintf(L"[HATA] Desktop yok.\n"); goto done; }

    // Kaynak seçim
    wchar_t src[MAX_PATH] = L"";

    // 1) CachedFiles → Son eklenen
    wchar_t cachedDir[MAX_PATH]; cachedDir[0] = L'\0';
    join_path(cachedDir, MAX_PATH, roaming, L"Microsoft\\Windows\\Themes\\CachedFiles");
    if (find_latest_added_from_cached(cachedDir, src, MAX_PATH)) {
        wprintf(L"[BİLGİ] CachedFiles (son eklenen): %ls\n", src);
    } else {
        // 2) Aktif duvar kâğıdı (SPI)
        wchar_t spi[MAX_PATH] = L"";
        if (SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, spi, 0) && spi[0] != L'\0' && file_exists(spi)) {
            wcsncpy(src, spi, MAX_PATH - 1); src[MAX_PATH - 1] = L'\0';
            wprintf(L"[BİLGİ] Aktif (SPI): %ls\n", src);
        } else {
            // 3) TranscodedWallpaper yedeği
            wchar_t transcoded[MAX_PATH]; transcoded[0] = L'\0';
            join_path(transcoded, MAX_PATH, roaming, L"Microsoft\\Windows\\Themes\\TranscodedWallpaper");
            if (file_exists(transcoded)) {
                wcsncpy(src, transcoded, MAX_PATH - 1); src[MAX_PATH - 1] = L'\0';
                wprintf(L"[BİLGİ] TranscodedWallpaper: %ls\n", src);
            } else {
                wprintf(L"[BİLGİ] Kopyalanacak görsel bulunamadı.\n");
                goto done;
            }
        }
    }

    // Hedef: Desktop\savedwallpapers
    wchar_t destDir[MAX_PATH]; destDir[0] = L'\0';
    join_path(destDir, MAX_PATH, desktop, L"savedwallpapers");
    if (!ensure_dir(destDir)) { wprintf(L"[HATA] Hedef klasör yaratılamadı: %ls (err %lu)\n", destDir, GetLastError()); goto done; }

    // Uzantı
    const wchar_t *ext = wcsrchr(src, L'.'); if (!ext || wcslen(ext) > 10) ext = L".jpg";

    // Zaman damgalı ad
    SYSTEMTIME st; GetLocalTime(&st);
    wchar_t dest[MAX_PATH];
    _snwprintf(dest, MAX_PATH, L"%ls\\wallpaper_%04u-%02u-%02u_%02u-%02u-%02u%ls",
               destDir, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, ext);

    if (!CopyFileW(src, dest, TRUE)) {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_EXISTS) {
            _snwprintf(dest, MAX_PATH, L"%ls\\wallpaper_%04u-%02u-%02u_%02u-%02u-%02u_%03u%ls",
                       destDir, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, ext);
            if (!CopyFileW(src, dest, TRUE)) { wprintf(L"[HATA] Kopyalama hatası %lu\n", GetLastError()); goto done; }
        } else { wprintf(L"[HATA] Kopyalama hatası %lu\n", err); goto done; }
    }

    wprintf(L"[OK] %ls → %ls\n", src, dest);
    rc = 0;

done:
    if (roaming) CoTaskMemFree(roaming);
    if (desktop) CoTaskMemFree(desktop);
    return rc;
}
