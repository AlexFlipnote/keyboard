#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include "resource.h"

typedef struct {
    DWORD from;
    WORD  to;
} Remap;

// https://learn.microsoft.com/windows/win32/inputdev/virtual-key-codes
static const Remap REMAPS[] = {
    { VK_LAUNCH_MEDIA_SELECT, VK_MEDIA_PREV_TRACK }, // 1st key | Previous track
    { VK_LAUNCH_MAIL,         VK_MEDIA_PLAY_PAUSE }, // 2nd key | Play/Pause
    { VK_LAUNCH_APP2,         VK_MEDIA_NEXT_TRACK }, // 3rd key | Next track
    { VK_LAUNCH_APP1,         VK_F13              }, // 4th key | F13 aka. clip button
};

#define COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#define OWN_INPUT_TAG ((ULONG_PTR)0x4B455942) // "KEYB"

static HHOOK g_hook;

static void send_key(WORD vk, BOOL key_up) {
    UINT scan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC_EX);

    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.wScan = (WORD)(scan & 0xFF);
    input.ki.dwFlags = key_up ? KEYEVENTF_KEYUP : 0;
    if ((scan >> 8) == 0xE0) { input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY; }
    input.ki.dwExtraInfo = OWN_INPUT_TAG;
    SendInput(1, &input, sizeof(INPUT));
}

static LRESULT CALLBACK keyboard_hook(int code, WPARAM wparam, LPARAM lparam) {
    if (code == HC_ACTION) {
        const KBDLLHOOKSTRUCT *kb = (const KBDLLHOOKSTRUCT *)lparam;

        // Skip only our own output, keys injected by (example) SteelSeries GG still get remapped.
        if (kb->dwExtraInfo != OWN_INPUT_TAG) {
            BOOL key_up = (wparam == WM_KEYUP || wparam == WM_SYSKEYUP);

            for (size_t i = 0; i < COUNT(REMAPS); i++) {
                if (kb->vkCode == REMAPS[i].from) {
                    send_key(REMAPS[i].to, key_up);
                    return 1;
                }
            }
        }
    }

    return CallNextHookEx(g_hook, code, wparam, lparam);
}

#define WM_TRAY (WM_APP + 1)
#define ID_EXIT 1

static NOTIFYICONDATAW g_tray;
static UINT g_taskbar_created;

static void tray_add(void) {
    Shell_NotifyIconW(NIM_ADD, &g_tray);
}

static void tray_show_menu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, ID_EXIT, L"Exit");

    // Lets the menu close when clicking elsewhere.
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    PostMessageW(hwnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_TRAY:
            if (lparam == WM_RBUTTONUP) { tray_show_menu(hwnd); }
            return 0;

        case WM_COMMAND:
            if (LOWORD(wparam) == ID_EXIT) { DestroyWindow(hwnd); }
            return 0;

        case WM_DESTROY:
            Shell_NotifyIconW(NIM_DELETE, &g_tray);
            PostQuitMessage(0);
            return 0;
    }

    // Explorer restarted, so the tray is empty again.
    if (msg == g_taskbar_created) {
        tray_add();
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev, LPSTR cmdline, int show) {
    (void)prev; (void)cmdline; (void)show;

    HANDLE mutex = CreateMutexW(NULL, TRUE, L"Local\\alexflipnote.keyboard");
    if (GetLastError() == ERROR_ALREADY_EXISTS) { return 0; }

    g_hook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboard_hook, instance, 0);
    if (!g_hook) {
        MessageBoxW(NULL, L"Failed to install keyboard hook.", L"keyboard", MB_ICONERROR);
        return 1;
    }

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.lpszClassName = L"alexflipnote.keyboard";
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"keyboard", 0, 0, 0, 0, 0, NULL, NULL, instance, NULL);

    g_taskbar_created = RegisterWindowMessageW(L"TaskbarCreated");

    g_tray.cbSize = sizeof(g_tray);
    g_tray.hWnd = hwnd;
    g_tray.uID = 1;
    g_tray.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_tray.uCallbackMessage = WM_TRAY;
    g_tray.hIcon = LoadImageW(
        instance, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0
    );
    lstrcpynW(g_tray.szTip, L"keyboard", COUNT(g_tray.szTip));
    tray_add();

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnhookWindowsHookEx(g_hook);
    CloseHandle(mutex);
    return 0;
}
