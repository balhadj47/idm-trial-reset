#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <wininet.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include "core.h"
#include "resource.h"


WCHAR g_setacl[MAX_PATH];
WCHAR g_tempDir[MAX_PATH];

static const LPCWSTR allkeys[] = {
    L"{6DDF00DB-1234-46EC-8356-27E7B2051192}",
    L"{7B8E9164-324D-4A2E-A46D-0165FB2000EC}",
    L"{D5B91409-A8CA-4973-9A0B-59F713D25671}",
    L"{5ED60779-4DE2-4E07-B862-974CA4FF2E9C}",
    NULL, /* placeholder for dynamic key */
    L"{07999AC3-058B-40BF-984F-69EB1E554CA7}",
};
#define NKEYS (sizeof(allkeys)/sizeof(allkeys[0]))

static WCHAR g_dynkey[128]; /* allkeys[4] */

static BOOL ExtractResource(HMODULE hMod, int resId, LPCWSTR destPath)
{
    HRSRC   hRes  = FindResourceW(hMod, MAKEINTRESOURCEW(resId), MAKEINTRESOURCEW(10) /* RT_RCDATA */);
    if (!hRes) return FALSE;
    HGLOBAL hData = LoadResource(hMod, hRes);
    if (!hData) return FALSE;
    DWORD   size  = SizeofResource(hMod, hRes);
    LPVOID  ptr   = LockResource(hData);
    if (!ptr) return FALSE;

    HANDLE hf = CreateFileW(destPath, GENERIC_WRITE, 0, NULL,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hf == INVALID_HANDLE_VALUE) return FALSE;
    DWORD written;
    WriteFile(hf, ptr, size, &written, NULL);
    CloseHandle(hf);
    return written == size;
}

BOOL ExtractResources(void)
{
    GetTempPathW(MAX_PATH, g_tempDir);

    WCHAR path[MAX_PATH];
    HMODULE hMod = GetModuleHandleW(NULL);

    /* Determine SetACL path based on OS arch */
    BOOL isX64 = FALSE;
    IsWow64Process(GetCurrentProcess(), &isX64);

    if (isX64) {
        swprintf_s(path, MAX_PATH, L"%sSetACLx64.exe", g_tempDir);
        ExtractResource(hMod, IDR_SETACL64, path);
    } else {
        swprintf_s(path, MAX_PATH, L"%sSetACLx32.exe", g_tempDir);
        ExtractResource(hMod, IDR_SETACL32, path);
    }
    wcscpy_s(g_setacl, MAX_PATH, path);

    swprintf_s(path, MAX_PATH, L"%sidm_reset.reg", g_tempDir);
    if (!ExtractResource(hMod, IDR_IDM_RESET, path)) return FALSE;

    swprintf_s(path, MAX_PATH, L"%sidm_trial.reg", g_tempDir);
    if (!ExtractResource(hMod, IDR_IDM_TRIAL, path)) return FALSE;

    swprintf_s(path, MAX_PATH, L"%sidm_reg.reg", g_tempDir);
    if (!ExtractResource(hMod, IDR_IDM_REG, path)) return FALSE;

    return TRUE;
}

void ClearTemp(void)
{
    WCHAR path[MAX_PATH];
    const LPCWSTR files[] = {
        L"idm_reset.reg", L"idm_trial.reg", L"idm_reg.reg",
        L"SetACLx32.exe", L"SetACLx64.exe", L"reg_query.tmp"
    };
    for (int i = 0; i < 6; i++) {
        swprintf_s(path, MAX_PATH, L"%s%s", g_tempDir, files[i]);
        DeleteFileW(path);
    }
}

static void RunHidden(LPCWSTR cmd)
{
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {0};
    WCHAR buf[4096];
    wcscpy_s(buf, 4096, cmd);
    if (CreateProcessW(NULL, buf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

void SetOwner(LPCWSTR owner)
{
    LPCWSTR sid = L"S-1-1-0"; /* everyone */
    if (wcscmp(owner, L"nobody") == 0) sid = L"S-1-0-0";

    WCHAR cmd[2048];
    for (int i = 0; i < (int)NKEYS; i++) {
        LPCWSTR k = (i == 4) ? g_dynkey : allkeys[i];
        if (!k || k[0] == L'\0') continue;

        const LPCWSTR roots[] = {
            L"HKCU\\Software\\Classes\\CLSID\\",
            L"HKCU\\Software\\Classes\\Wow6432Node\\CLSID\\",
            L"HKLM\\Software\\Classes\\CLSID\\",
            L"HKLM\\Software\\Classes\\Wow6432Node\\CLSID\\"
        };
        for (int r = 0; r < 4; r++) {
            swprintf_s(cmd, 2048,
                L"\"%s\" -on %s%s -ot reg -actn setowner -ownr \"n:%s\" -silent",
                g_setacl, roots[r], k, sid);
            RunHidden(cmd);
        }
    }
}

void SetPermission(LPCWSTR perm)
{
    WCHAR cmd[2048];
    for (int i = 0; i < (int)NKEYS; i++) {
        LPCWSTR k = (i == 4) ? g_dynkey : allkeys[i];
        if (!k || k[0] == L'\0') continue;

        const LPCWSTR roots[] = {
            L"HKCU\\Software\\Classes\\CLSID\\",
            L"HKCU\\Software\\Classes\\Wow6432Node\\CLSID\\",
            L"HKLM\\Software\\Classes\\CLSID\\",
            L"HKLM\\Software\\Classes\\Wow6432Node\\CLSID\\"
        };
        for (int r = 0; r < 4; r++) {
            swprintf_s(cmd, 2048,
                L"\"%s\" -on %s%s -ot reg -actn ace -ace \"n:everyone;p:%s\""
                L" -actn setprot -op \"dacl:p_nc;sacl:p_nc\" -silent",
                g_setacl, roots[r], k, perm);
            RunHidden(cmd);
        }
    }
}

/* Search HKCR\CLSID for a value string, return the CLSID key name */
void RegSearch(LPCWSTR value, LPWSTR outKey, int outLen)
{
    outKey[0] = L'\0';

    WCHAR tmpFile[MAX_PATH];
    swprintf_s(tmpFile, MAX_PATH, L"%sreg_query.tmp", g_tempDir);

    WCHAR cmd[512];
    swprintf_s(cmd, 512, L"reg query hkcr\\clsid /s > \"%s\"", tmpFile);

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {0};
    WCHAR shell[600];
    swprintf_s(shell, 600, L"cmd.exe /c %s", cmd);
    if (CreateProcessW(NULL, shell, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    /* Read the file and search for 'value' */
    HANDLE hf = CreateFileW(tmpFile, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, 0, NULL);
    if (hf == INVALID_HANDLE_VALUE) return;

    DWORD size = GetFileSize(hf, NULL);
    char *buf = (char *)malloc(size + 2);
    if (!buf) { CloseHandle(hf); return; }
    DWORD read;
    ReadFile(hf, buf, size, &read, NULL);
    CloseHandle(hf);
    buf[read] = '\0';

    /* Convert value to narrow for search */
    char narrow[128];
    WideCharToMultiByte(CP_ACP, 0, value, -1, narrow, 128, NULL, NULL);

    char *found = strstr(buf, narrow);
    if (found) {
        /* Walk back to find the preceding [HKEY...{CLSID}] line */
        char *lineStart = found;
        while (lineStart > buf && *(lineStart-1) != '\n') lineStart--;
        /* Search backward for a line containing '{' and '}' */
        char *search = lineStart;
        while (search > buf) {
            search--;
            if (*search == '\n' || search == buf) {
                char *ls = (*search == '\n') ? search + 1 : search;
                char *open  = strchr(ls, '{');
                char *close = strchr(ls, '}');
                if (open && close && close > open) {
                    int len = (int)(close - open - 1);
                    if (len > 0 && len < outLen - 3) {
                        outKey[0] = L'{';
                        MultiByteToWideChar(CP_ACP, 0, open+1, len,
                                            outKey+1, outLen-2);
                        outKey[len+1] = L'}';
                        outKey[len+2] = L'\0';
                    }
                    break;
                }
            }
        }
    }
    free(buf);
}

void Reset(void)
{
    RegSearch(L"cDTvBFquXk0", g_dynkey, 128);

    SetOwner(L"everyone");
    SetPermission(L"full");

    WCHAR cmd[512];
    swprintf_s(cmd, 512, L"reg import \"%sidm_reset.reg\"", g_tempDir);
    RunHidden(cmd);

    if (g_dynkey[0] != L'\0') {
        const LPCWSTR bases[] = {
            L"HKEY_CURRENT_USER\\Software\\Classes\\CLSID\\",
            L"HKEY_CURRENT_USER\\Software\\Classes\\Wow6432Node\\CLSID\\",
            L"HKEY_LOCAL_MACHINE\\Software\\Classes\\CLSID\\",
            L"HKEY_LOCAL_MACHINE\\Software\\Classes\\Wow6432Node\\CLSID\\"
        };
        for (int i = 0; i < 4; i++) {
            WCHAR delCmd[512];
            swprintf_s(delCmd, 512,
                L"reg delete \"%s%s\" /f", bases[i], g_dynkey);
            RunHidden(delCmd);
        }
    }
}

void Trial(void)
{
    Reset();
    WCHAR cmd[512];
    swprintf_s(cmd, 512, L"reg import \"%sidm_trial.reg\"", g_tempDir);
    RunHidden(cmd);
    SetPermission(L"read");
    SetOwner(L"nobody");
}

void TrialSilent(void)
{
    WCHAR dateVal[64] = {0};
    DWORD sz = sizeof(dateVal);
    RegGetValueW(HKEY_CURRENT_USER,
                 L"Software\\DownloadManager",
                 L"auto_reset_trial",
                 RRF_RT_REG_SZ, NULL, dateVal, &sz);

    /* dateVal format: YYYY/MM/DD */
    int ty, tm, td;
    if (swscanf_s(dateVal, L"%d/%d/%d", &ty, &tm, &td) != 3) return;

    SYSTEMTIME st;
    GetLocalTime(&st);

    /* Simple day difference: just compare date fields */
    int diff = (ty - st.wYear) * 365 + (tm - st.wMonth) * 30 + (td - st.wDay);
    if (diff <= 0) {
        Trial();
        Autorun(L"trial");

        if (GotUpdate()) {
            int r = MessageBoxW(NULL, L"Update me now?",
                                L"IDM trial reset", MB_YESNO);
            if (r == IDYES)
                ShellExecuteW(NULL, L"open", L"" URL_DOWNLOAD, NULL, NULL, SW_SHOW);
        }
    }
}

void Register(LPCWSTR fname)
{
    Reset();
    Autorun(L"off");

    WCHAR cmd[512];
    swprintf_s(cmd, 512, L"reg import \"%sidm_reg.reg\"", g_tempDir);
    RunHidden(cmd);

    if (g_dynkey[0] != L'\0') {
        const LPCWSTR bases[] = {
            L"HKEY_CURRENT_USER\\Software\\Classes\\CLSID\\",
            L"HKEY_CURRENT_USER\\Software\\Classes\\Wow6432Node\\CLSID\\",
            L"HKEY_LOCAL_MACHINE\\Software\\Classes\\CLSID\\",
            L"HKEY_LOCAL_MACHINE\\Software\\Classes\\Wow6432Node\\CLSID\\"
        };
        for (int i = 0; i < 4; i++) {
            WCHAR addCmd[512];
            swprintf_s(addCmd, 512, L"reg add \"%s%s\" /f", bases[i], g_dynkey);
            RunHidden(addCmd);
        }
    }

    WCHAR addName[512];
    swprintf_s(addName, 512,
        L"reg add \"HKCU\\Software\\DownloadManager\" /v \"FName\""
        L" /t REG_SZ /d \"%s\" /f", fname);
    RunHidden(addName);

    SetPermission(L"read");
    SetOwner(L"nobody");
}

void Autorun(LPCWSTR mode)
{
    if (wcscmp(mode, L"off") == 0) {
        RunHidden(L"reg delete \"HKCU\\Software\\DownloadManager\""
                  L" /v \"auto_reset_trial\" /f");
        RunHidden(L"reg delete"
                  L" \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\""
                  L" /v \"IDM trial reset\" /f");
    } else if (wcscmp(mode, L"trial") == 0) {
        /* Compute date 15 days from now */
        SYSTEMTIME st;
        GetLocalTime(&st);
        FILETIME ft;
        SystemTimeToFileTime(&st, &ft);
        ULARGE_INTEGER ul;
        ul.LowPart  = ft.dwLowDateTime;
        ul.HighPart = ft.dwHighDateTime;
        ul.QuadPart += (ULONGLONG)15 * 24 * 3600 * 10000000ULL;
        ft.dwLowDateTime  = ul.LowPart;
        ft.dwHighDateTime = ul.HighPart;
        FileTimeToSystemTime(&ft, &st);

        WCHAR dateStr[32], cmd[512];
        swprintf_s(dateStr, 32, L"%04d/%02d/%02d",
                   st.wYear, st.wMonth, st.wDay);
        swprintf_s(cmd, 512,
            L"reg add \"HKCU\\Software\\DownloadManager\""
            L" /v \"auto_reset_trial\" /t REG_SZ /d \"%s\" /f", dateStr);
        RunHidden(cmd);

        WCHAR selfPath[MAX_PATH];
        GetModuleFileNameW(NULL, selfPath, MAX_PATH);
        swprintf_s(cmd, 512,
            L"reg add"
            L" \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\""
            L" /v \"IDM trial reset\" /t REG_SZ"
            L" /d \"\\\"%s\\\" /trial\" /f", selfPath);
        RunHidden(cmd);
    }
}

BOOL IsAuto(void)
{
    WCHAR dateVal[64] = {0};
    DWORD sz = sizeof(dateVal);
    LSTATUS s = RegGetValueW(HKEY_CURRENT_USER,
                             L"Software\\DownloadManager",
                             L"auto_reset_trial",
                             RRF_RT_REG_SZ, NULL, dateVal, &sz);
    if (s != ERROR_SUCCESS || dateVal[0] == L'\0') return FALSE;

    int ty, tm, td;
    if (swscanf_s(dateVal, L"%d/%d/%d", &ty, &tm, &td) != 3) return FALSE;

    WCHAR runVal[MAX_PATH * 2] = {0};
    sz = sizeof(runVal);
    s = RegGetValueW(HKEY_CURRENT_USER,
                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                     L"IDM trial reset",
                     RRF_RT_REG_SZ, NULL, runVal, &sz);
    if (s != ERROR_SUCCESS) return FALSE;

    /* Extract path from quoted string and verify it exists */
    WCHAR exePath[MAX_PATH] = {0};
    if (runVal[0] == L'"') {
        WCHAR *end = wcschr(runVal + 1, L'"');
        if (end) {
            int len = (int)(end - runVal - 1);
            wcsncpy_s(exePath, MAX_PATH, runVal + 1, len);
        }
    } else {
        wcscpy_s(exePath, MAX_PATH, runVal);
    }

    return PathFileExistsW(exePath);
}

BOOL GotUpdate(void)
{
    HINTERNET hInet = InternetOpenW(L"IDMReset", INTERNET_OPEN_TYPE_PRECONFIG,
                                    NULL, NULL, 0);
    if (!hInet) return FALSE;

    HINTERNET hUrl = InternetOpenUrlW(hInet, L"" UPDATE_URL,
                                      NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) { InternetCloseHandle(hInet); return FALSE; }

    char buf[512] = {0};
    DWORD read;
    InternetReadFile(hUrl, buf, sizeof(buf)-1, &read);
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInet);

    buf[read] = '\0';
    char *open  = strstr(buf, "<version>");
    char *close = strstr(buf, "</version>");
    if (!open || !close || close <= open) return FALSE;

    int latest = atoi(open + 9);
    return latest > VERSION;
}
