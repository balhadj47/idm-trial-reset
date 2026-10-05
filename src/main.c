#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include "core.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

/* Control IDs */
#define IDC_TAB         200
#define IDC_BTN_RESET   201
#define IDC_CHK_AUTO    202
#define IDC_BTN_REG     203
#define IDC_LBL_REG     204
#define IDC_LBL_HELP    205
#define IDC_BTN_FORUM   206
#define IDC_BTN_UPDATE  207

static HWND g_hTab, g_hBtnReset, g_hChkAuto;
static HWND g_hBtnReg, g_hLblReg;
static HWND g_hLblHelp, g_hBtnForum, g_hBtnUpdate;
static HWND g_hWnd;

static void ShowTab(int idx)
{
    /* Tab 0: Trial reset */
    BOOL t0 = (idx == 0);
    BOOL t1 = (idx == 1);
    BOOL t2 = (idx == 2);
    ShowWindow(g_hBtnReset,  t0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hChkAuto,   t0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hBtnReg,    t1 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hLblReg,    t1 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hLblHelp,   t2 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hBtnForum,  t2 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hBtnUpdate, t2 ? SW_SHOW : SW_HIDE);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_TAB_CLASSES };
        InitCommonControlsEx(&icc);

        g_hTab = CreateWindowExW(0, WC_TABCONTROLW, NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            0, 0, 325, 112, hWnd, (HMENU)IDC_TAB, NULL, NULL);

        TCITEMW tie = { TCIF_TEXT };
        tie.pszText = L"Trial reset"; TabCtrl_InsertItem(g_hTab, 0, &tie);
        tie.pszText = L"Register";    TabCtrl_InsertItem(g_hTab, 1, &tie);
        tie.pszText = L"Help";        TabCtrl_InsertItem(g_hTab, 2, &tie);

        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        /* Tab 0 controls */
        g_hBtnReset = CreateWindowExW(0, L"BUTTON",
            L"Reset the IDM trial now",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            78, 38, 180, 35, hWnd, (HMENU)IDC_BTN_RESET, NULL, NULL);
        SendMessage(g_hBtnReset, WM_SETFONT, (WPARAM)hFont, TRUE);

        g_hChkAuto = CreateWindowExW(0, L"BUTTON",
            L"Automatically",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            128, 78, 80, 20, hWnd, (HMENU)IDC_CHK_AUTO, NULL, NULL);
        SendMessage(g_hChkAuto, WM_SETFONT, (WPARAM)hFont, TRUE);
        if (IsAuto())
            SendMessage(g_hChkAuto, BM_SETCHECK, BST_CHECKED, 0);

        /* Tab 1 controls */
        g_hBtnReg = CreateWindowExW(0, L"BUTTON",
            L"Register IDM now",
            WS_CHILD | BS_PUSHBUTTON,
            78, 38, 180, 35, hWnd, (HMENU)IDC_BTN_REG, NULL, NULL);
        SendMessage(g_hBtnReg, WM_SETFONT, (WPARAM)hFont, TRUE);

        g_hLblReg = CreateWindowExW(0, L"STATIC",
            L"If IDM is blocked after Register then Register again or use Trial reset",
            WS_CHILD | SS_CENTER,
            27, 78, 282, 17, hWnd, (HMENU)IDC_LBL_REG, NULL, NULL);
        SendMessage(g_hLblReg, WM_SETFONT, (WPARAM)hFont, TRUE);

        /* Tab 2 controls */
        g_hLblHelp = CreateWindowExW(0, L"STATIC",
            L"Trial reset ---> Reset the IDM trial, fix blocked, fake serial...\r\n"
            L"Register -----> Register IDM",
            WS_CHILD | SS_LEFT,
            15, 33, 308, 50, hWnd, (HMENU)IDC_LBL_HELP, NULL, NULL);
        SendMessage(g_hLblHelp, WM_SETFONT, (WPARAM)hFont, TRUE);

        g_hBtnForum = CreateWindowExW(0, L"BUTTON",
            L"Chat about this tool",
            WS_CHILD | BS_PUSHBUTTON,
            45, 71, 115, 25, hWnd, (HMENU)IDC_BTN_FORUM, NULL, NULL);
        SendMessage(g_hBtnForum, WM_SETFONT, (WPARAM)hFont, TRUE);

        g_hBtnUpdate = CreateWindowExW(0, L"BUTTON",
            L"Check for update",
            WS_CHILD | BS_PUSHBUTTON,
            166, 71, 105, 25, hWnd, (HMENU)IDC_BTN_UPDATE, NULL, NULL);
        SendMessage(g_hBtnUpdate, WM_SETFONT, (WPARAM)hFont, TRUE);

        ShowTab(0);
        break;
    }

    case WM_NOTIFY: {
        LPNMHDR nmh = (LPNMHDR)lp;
        if (nmh->idFrom == IDC_TAB && nmh->code == TCN_SELCHANGE)
            ShowTab(TabCtrl_GetCurSel(g_hTab));
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_BTN_RESET:
            SetWindowTextW(g_hBtnReset, L"Please wait...");
            EnableWindow(g_hBtnReset, FALSE);
            Trial();
            EnableWindow(g_hBtnReset, TRUE);
            SetWindowTextW(g_hBtnReset, L"Reset the IDM trial now");
            MessageBoxW(hWnd, L"You have 30 day trial now!",
                        L"Reset IDM trial", MB_OK | MB_TOPMOST);
            break;

        case IDC_CHK_AUTO: {
            BOOL checked = (SendMessage(g_hChkAuto, BM_GETCHECK, 0, 0) == BST_CHECKED);
            if (checked) {
                SetWindowTextW(g_hBtnReset, L"Please wait...");
                EnableWindow(g_hBtnReset, FALSE);
                Trial();
                Autorun(L"trial");
                EnableWindow(g_hBtnReset, TRUE);
                SetWindowTextW(g_hBtnReset, L"Reset the IDM trial now");
                MessageBoxW(hWnd, L"The IDM trial will be reset automatically.",
                            L"Auto reset", MB_OK | MB_TOPMOST);
            } else {
                Autorun(L"off");
                MessageBoxW(hWnd, L"The IDM trial will NOT be reset automatically.",
                            L"Auto reset", MB_OK | MB_TOPMOST);
            }
            break;
        }

        case IDC_BTN_REG: {
            wcscpy_s(g_inputResult, 256, L"IDM trial reset");
            INT_PTR r = ShowInputBox(hWnd);
            if (r != IDOK) break;
            SetWindowTextW(g_hBtnReg, L"Please wait...");
            EnableWindow(g_hBtnReg, FALSE);
            Register(g_inputResult);
            SendMessage(g_hChkAuto, BM_SETCHECK, BST_UNCHECKED, 0);
            EnableWindow(g_hBtnReg, TRUE);
            SetWindowTextW(g_hBtnReg, L"Register IDM now");
            MessageBoxW(hWnd, L"IDM is registered now!",
                        L"Register IDM", MB_OK | MB_TOPMOST);
            break;
        }

        case IDC_BTN_FORUM:
            ShellExecuteW(NULL, L"open", L"" URL_FORUM, NULL, NULL, SW_SHOW);
            break;

        case IDC_BTN_UPDATE:
            SetWindowTextW(g_hBtnUpdate, L"Please wait...");
            EnableWindow(g_hBtnUpdate, FALSE);
            if (GotUpdate()) {
                int r = MessageBoxW(hWnd, L"Update me now?",
                                    L"IDM trial reset", MB_YESNO | MB_TOPMOST);
                if (r == IDYES)
                    ShellExecuteW(NULL, L"open", L"" URL_DOWNLOAD, NULL, NULL, SW_SHOW);
            } else {
                MessageBoxW(hWnd, L"No update was found!",
                            L"IDM trial reset", MB_OK | MB_TOPMOST);
            }
            EnableWindow(g_hBtnUpdate, TRUE);
            SetWindowTextW(g_hBtnUpdate, L"Check for update");
            break;
        }
        break;

    case WM_DESTROY:
        ClearTemp();
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hWnd, msg, wp, lp);
}

/* Simple input dialog for register name */
static WCHAR g_inputResult[256];
static HWND  g_hInputEdit;

static INT_PTR CALLBACK InputDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    (void)lp;
    switch (msg) {
    case WM_INITDIALOG:
        SetDlgItemTextW(hDlg, 101, L"IDM trial reset");
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            GetDlgItemTextW(hDlg, 101, g_inputResult, 256);
            if (g_inputResult[0] == L'\0')
                wcscpy_s(g_inputResult, 256, L"IDM trial reset");
            EndDialog(hDlg, IDOK);
        } else if (LOWORD(wp) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
        }
        return TRUE;
    }
    return FALSE;
}

static INT_PTR ShowInputBox(HWND hParent)
{
    /* Build dialog template in memory */
    #pragma pack(push, 1)
    typedef struct {
        DLGTEMPLATE tmpl;
        WORD menu, cls, title;
        /* controls follow */
    } DlgHdr;
    #pragma pack(pop)

    BYTE dlgBuf[512] = {0};
    DLGTEMPLATE *dt = (DLGTEMPLATE *)dlgBuf;
    dt->style = DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_SETFONT;
    dt->dwExtendedStyle = 0;
    dt->cdit = 3;
    dt->x = 0; dt->y = 0; dt->cx = 200; dt->cy = 70;

    WORD *p = (WORD *)(dt + 1);
    *p++ = 0; /* menu */
    *p++ = 0; /* class */
    /* title */
    const WCHAR *title = L"Register IDM";
    while (*title) *p++ = *title++;
    *p++ = 0;
    /* font */
    *p++ = 8;
    const WCHAR *font = L"MS Shell Dlg";
    while (*font) *p++ = *font++;
    *p++ = 0;

    /* Align to DWORD */
    if ((ULONG_PTR)p % 4) p = (WORD *)((BYTE *)p + 2);

    /* Static label */
    DLGITEMTEMPLATE *di = (DLGITEMTEMPLATE *)p;
    di->style = WS_CHILD | WS_VISIBLE | SS_LEFT;
    di->dwExtendedStyle = 0;
    di->x = 7; di->y = 7; di->cx = 186; di->cy = 12;
    di->id = 100;
    p = (WORD *)(di + 1);
    *p++ = 0xFFFF; *p++ = 0x0082; /* STATIC */
    const WCHAR *lbl = L"Type your name here:";
    while (*lbl) *p++ = *lbl++;
    *p++ = 0; *p++ = 0;
    if ((ULONG_PTR)p % 4) p = (WORD *)((BYTE *)p + 2);

    /* Edit */
    di = (DLGITEMTEMPLATE *)p;
    di->style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
    di->dwExtendedStyle = 0;
    di->x = 7; di->y = 20; di->cx = 186; di->cy = 14;
    di->id = 101;
    p = (WORD *)(di + 1);
    *p++ = 0xFFFF; *p++ = 0x0081; /* EDIT */
    *p++ = 0; *p++ = 0;
    if ((ULONG_PTR)p % 4) p = (WORD *)((BYTE *)p + 2);

    /* OK button */
    di = (DLGITEMTEMPLATE *)p;
    di->style = WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON;
    di->dwExtendedStyle = 0;
    di->x = 65; di->y = 48; di->cx = 50; di->cy = 14;
    di->id = IDOK;
    p = (WORD *)(di + 1);
    *p++ = 0xFFFF; *p++ = 0x0080; /* BUTTON */
    const WCHAR *ok = L"OK";
    while (*ok) *p++ = *ok++;
    *p++ = 0; *p++ = 0;

    return DialogBoxIndirectW(GetModuleHandleW(NULL),
                              (DLGTEMPLATE *)dlgBuf,
                              hParent, InputDlgProc);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
    (void)hPrev; (void)lpCmd;

    /* Singleton: allow only one instance */
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"IDMTrialReset_Singleton");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        return 0;
    }

    /* Require admin — manifest should handle this, but check anyway */

    if (!ExtractResources()) {
        MessageBoxW(NULL, L"Failed to extract resources.",
                    L"Error", MB_ICONERROR);
        return 1;
    }

    /* Handle /trial command line */
    LPWSTR cmdLine = GetCommandLineW();
    if (wcsstr(cmdLine, L"/trial")) {
        TrialSilent();
        ClearTemp();
        return 0;
    }

    /* Create window */
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hIcon         = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN_ICON));
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"IDMTrialReset";
    RegisterClassExW(&wc);

    RECT rc = {0, 0, 325, 112};
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

    g_hWnd = CreateWindowExW(0, L"IDMTrialReset",
        L"IDM trial reset",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInst, NULL);

    ShowWindow(g_hWnd, nShow);
    UpdateWindow(g_hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CloseHandle(hMutex);
    return (int)msg.wParam;
}
