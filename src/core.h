#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define VERSION       21
#define URL_FORUM     "https://github.com/J2TEAM/idm-trial-reset/discussions"
#define URL_DOWNLOAD  "https://github.com/J2TEAM/idm-trial-reset/releases"
#define UPDATE_URL    "http://pastebin.com/raw/uYr0cstV"

extern WCHAR g_setacl[MAX_PATH];
extern WCHAR g_tempDir[MAX_PATH];

/* Embedded resource extraction */
BOOL ExtractResources(void);
void ClearTemp(void);

/* Registry helpers */
void RegSearch(LPCWSTR value, LPWSTR outKey, int outLen);
BOOL IsAuto(void);

/* Core operations */
void SetOwner(LPCWSTR owner);       /* "everyone" or "nobody" */
void SetPermission(LPCWSTR perm);   /* "read" or "full" */
void Reset(void);
void Trial(void);
void TrialSilent(void);
void Register(LPCWSTR fname);
void Autorun(LPCWSTR mode);         /* "trial" or "off" */
BOOL GotUpdate(void);
