#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <wingdi.h>
#include <cmath>
#include <tchar.h>
#include <string>
#include <commctrl.h> // For the Hotkey control

#include "GammaToggler.h"

// --- Global Variables & Constants ---
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_SET_GAMMA 1002
#define ID_TRAY_SET_HOTKEY 1003
#define HOTKEY_ID 1

HINSTANCE g_hInstance = NULL;

// Default values
const int DEFAULT_GAMMA_INT = 280; // Stored as 2.80 * 100
const int DEFAULT_MODIFIER_INT = MOD_NOREPEAT;
const int DEFAULT_KEY_INT = VK_F10; // VK_F10 is 0x79 or 121

// Configurable values (these will hold the active state)
float g_targetGamma = 0.0f;
UINT g_hotkeyModifier = 0;
UINT g_hotkeyKey = 0;

HWND g_hWindow = NULL;
NOTIFYICONDATA g_notifyIconData;
bool g_isGammaCustom = false;
TCHAR g_configPath[MAX_PATH];

// --- Function Prototypes ---
// System Tray and UI Functions
void CreateTrayIcon(HWND hwnd);
void RemoveTrayIcon();
void ShowContextMenu(HWND hwnd);
void UpdateTrayIconTip();
void CenterWindow(HWND hDlg);

// Core Application Logic
void ToggleGamma();
void SetScreenGamma(float gamma);
std::wstring GetHotkeyString(UINT modifiers, UINT vk);
bool RegisterNewHotkey();

// Configuration Load/Save
void LoadConfig();
void SaveConfig();

// Window and Dialog Procedures (Message Handlers)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK GammaDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HotkeyDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
// --- End Function Prototypes ---

// --- Entry Point ---
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    const TCHAR* mutexName = TEXT("Global\\{E1D8A5E0-2A6B-4B3B-8E4A-6B4B3B8E4A6B}"); // Example GUID

    HANDLE hMutex = CreateMutex(NULL, TRUE, mutexName);

    if (hMutex == NULL) {
        // This is a serious error, but unlikely.
        MessageBox(NULL, TEXT("Could not create mutex."), TEXT("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }

    // Check if the mutex already existed.
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(NULL, TEXT("Application already running."), TEXT("Gamma Toggler Error"), MB_OK | MB_ICONERROR);
        // Another instance is already running. Close this new instance.
        CloseHandle(hMutex); // Clean up the handle we just received.

        return 0; // Exit successfully.
    }


    g_hInstance = hInstance;
    // Get path for config.ini in the same directory as the executable
    GetModuleFileName(NULL, g_configPath, MAX_PATH);
    TCHAR* lastSlash = _tcsrchr(g_configPath, '\\');
    if (lastSlash) {
        *(lastSlash + 1) = '\0';
        _tcscat_s(g_configPath, MAX_PATH, TEXT("config.ini"));
    }

    LoadConfig();

    const TCHAR CLASS_NAME[] = TEXT("GammaTogglerWindowClass");
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_GAMMATOGGLER), IMAGE_ICON,
        GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    RegisterClass(&wc);

    g_hWindow = CreateWindowEx(0, CLASS_NAME, TEXT("Gamma Toggler"), 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    if (g_hWindow == NULL) return 1;

    if (!RegisterNewHotkey()) {
        MessageBox(NULL, TEXT("Failed to register hotkey. It might be in use."), TEXT("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }

    CreateTrayIcon(g_hWindow);

    UpdateTrayIconTip();

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

// --- Main Window Procedure ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_HOTKEY:
        if (wParam == HOTKEY_ID) {
            ToggleGamma();
        }
        break;
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            ShowContextMenu(hwnd);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_EXIT:
            DestroyWindow(hwnd);
            break;

        case ID_TRAY_SET_GAMMA: {
            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SET_GAMMA), hwnd, GammaDialogProc);
            break;
        }

        case ID_TRAY_SET_HOTKEY: {
            // Unregister hotkey before opening dialog window
            UnregisterHotKey(g_hWindow, HOTKEY_ID);

            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SET_HOTKEY), hwnd, HotkeyDialogProc);

            // After the dialog closes, register the new key, then save and update
            if (RegisterNewHotkey()) {
                SaveConfig();
                UpdateTrayIconTip();
            }
            else {
                // Handle failure if needed
                MessageBox(hwnd, TEXT("Could not re-register hotkey."), TEXT("Warning"), MB_OK);
            }
            break;
        }
        }
        break;
    case WM_DESTROY:
        RemoveTrayIcon();
        UnregisterHotKey(hwnd, HOTKEY_ID);
        SetScreenGamma(1.0f);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

std::wstring GetHotkeyString(UINT modifiers, UINT vk) {
    if (vk == 0) {
        return L"None";
    }

    std::wstring hotkeyStr = L"";

    if (modifiers & MOD_CONTROL) {
        hotkeyStr += L"Ctrl + ";
    }
    if (modifiers & MOD_ALT) {
        hotkeyStr += L"Alt + ";
    }
    if (modifiers & MOD_SHIFT) {
        hotkeyStr += L"Shift + ";
    }
    if (modifiers & MOD_WIN) {
        hotkeyStr += L"Win + ";
    }

    // --- THE FIX IS HERE: Get the scan code and build a proper lParam ---

    // For simple alphanumeric keys, we can just append the character. This is reliable.
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) {
        hotkeyStr += (TCHAR)vk;
    }
    // For all other keys (F-keys, navigation, etc.), we must use the API correctly.
    else {
        // 1. Get the scan code for the virtual key.
        UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);

        // 2. Build the lParam with both the scan code and the virtual key code.
        LONG lparam = (scanCode << 16);

        // For F-keys and other special keys, we may need to set the "extended key" flag.
        switch (vk) {
        case VK_INSERT:
        case VK_DELETE:
        case VK_HOME:
        case VK_END:
        case VK_PRIOR: // Page Up
        case VK_NEXT:  // Page Down
        case VK_UP:
        case VK_DOWN:
        case VK_LEFT:
        case VK_RIGHT:
            lparam |= (1 << 24); // Set the extended-key flag
            break;
        }

        TCHAR keyName[50] = { 0 };
        // 3. Call GetKeyNameText with the complete lParam.
        if (GetKeyNameText(lparam, keyName, sizeof(keyName) / sizeof(TCHAR))) {
            hotkeyStr += keyName;
        }
        else {
            hotkeyStr += L"?"; // Fallback if it still fails
        }
    }
    // --- END FIX ---

    return hotkeyStr;
}

void CenterWindow(HWND hDlg) {
    RECT rcDlg;
    GetWindowRect(hDlg, &rcDlg); // Get the dialog's dimensions

    // Get the screen's dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Calculate the centered position
    int x = (screenWidth - (rcDlg.right - rcDlg.left)) / 2;
    int y = (screenHeight - (rcDlg.bottom - rcDlg.top)) / 2;

    // Move the dialog to the calculated position
    SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

// --- Dialog Procedures ---

INT_PTR CALLBACK GammaDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        CenterWindow(hDlg);

        TCHAR buffer[32];
        _stprintf_s(buffer, TEXT("%.2f"), g_targetGamma);
        SetDlgItemText(hDlg, IDC_GAMMA_EDIT, buffer);
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            TCHAR buffer[32];
            GetDlgItemText(hDlg, IDC_GAMMA_EDIT, buffer, 32);
            float newGamma = _tstof(buffer);

            // Validation: Ensure gamma is within a reasonable, non-zero range.
            if (newGamma >= 0.1f && newGamma <= 10.0f) {
                g_targetGamma = newGamma;
                SaveConfig();
                UpdateTrayIconTip();
                EndDialog(hDlg, IDOK);
            }
            else {
                MessageBox(hDlg, TEXT("Invalid gamma value. Please enter a number between 0.1 and 10.0."), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
            }
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// --- Corrected HotkeyDialogProc ---
INT_PTR CALLBACK HotkeyDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        CenterWindow(hDlg);

        UINT dialogModifiers = 0;
        if (g_hotkeyModifier & MOD_CONTROL) dialogModifiers |= HOTKEYF_CONTROL;
        if (g_hotkeyModifier & MOD_ALT)     dialogModifiers |= HOTKEYF_ALT;
        if (g_hotkeyModifier & MOD_SHIFT)   dialogModifiers |= HOTKEYF_SHIFT;

        SendDlgItemMessage(hDlg, IDC_HOTKEY_CTRL, HKM_SETHOTKEY, MAKEWORD(g_hotkeyKey, dialogModifiers), 0);
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK: {
            LRESULT result = SendDlgItemMessage(hDlg, IDC_HOTKEY_CTRL, HKM_GETHOTKEY, 0, 0);
            UINT keyFromDialog = LOBYTE(result);
            UINT modifiersFromDialog = HIBYTE(result);

            if (result == 0) {
                g_hotkeyKey = 0;
                g_hotkeyModifier = 0;
            }
            else {
                // --- THE FIX IS HERE: Explicitly initialize to 0 ---
                UINT newModifiers = 0; // This guarantees a clean start in all build modes.
                // --- END FIX ---

                if (modifiersFromDialog & HOTKEYF_CONTROL) newModifiers |= MOD_CONTROL;
                if (modifiersFromDialog & HOTKEYF_ALT)     newModifiers |= MOD_ALT;
                if (modifiersFromDialog & HOTKEYF_SHIFT)   newModifiers |= MOD_SHIFT;

                // We also need to add back the MOD_NOREPEAT flag
                g_hotkeyModifier = newModifiers | MOD_NOREPEAT;
                g_hotkeyKey = keyFromDialog;
            }

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}





// --- Core & Helper Functions ---

void ToggleGamma() {
    g_isGammaCustom = !g_isGammaCustom;
    SetScreenGamma(g_isGammaCustom ? g_targetGamma : 1.0f);
    UpdateTrayIconTip();
}

void SetScreenGamma(float gamma) {
    if (gamma <= 0) return; // Safety check
    float exponent = 1.0f / gamma;
    HDC hdc = GetDC(NULL);
    if (!hdc) return;
    WORD ramp[256 * 3];
    for (int i = 0; i < 256; i++) {
        float correctedValue = pow((float)i / 255.0f, exponent) * 65535.0f;
        WORD finalValue = (WORD)min(65535, max(0, correctedValue));
        ramp[i] = ramp[i + 256] = ramp[i + 512] = finalValue;
    }
    SetDeviceGammaRamp(hdc, ramp);
    ReleaseDC(NULL, hdc);
}

bool RegisterNewHotkey() {
    UnregisterHotKey(g_hWindow, HOTKEY_ID); // Always unregister the old one first
    if (g_hotkeyKey == 0) return true; // Don't register if no key is set
    return RegisterHotKey(g_hWindow, HOTKEY_ID, g_hotkeyModifier, g_hotkeyKey);
}

void ShowContextMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    if (hMenu) {
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_TRAY_SET_GAMMA, TEXT("Set Gamma..."));
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_TRAY_SET_HOTKEY, TEXT("Set Hotkey..."));
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));

        SetForegroundWindow(hwnd);
        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }
}

void UpdateTrayIconTip() {
    TCHAR tip[128];
    float currentGamma = g_isGammaCustom ? g_targetGamma : 1.0f;

    // Get the human-readable hotkey string
    std::wstring hotkeyText = GetHotkeyString(g_hotkeyModifier, g_hotkeyKey);

    // Format the final tooltip string including the hotkey
    _stprintf_s(tip, ARRAYSIZE(tip), TEXT("Gamma Toggler\nGamma: %.2f\nHotkey: %s"), currentGamma, hotkeyText.c_str());

    lstrcpy(g_notifyIconData.szTip, tip);
    Shell_NotifyIcon(NIM_MODIFY, &g_notifyIconData);
}

void CreateTrayIcon(HWND hwnd) {
    g_notifyIconData = {};
    g_notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    g_notifyIconData.hWnd = hwnd;
    g_notifyIconData.uID = 1;
    g_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_notifyIconData.uCallbackMessage = WM_TRAYICON;
    g_notifyIconData.hIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_GAMMATOGGLER), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

    lstrcpy(g_notifyIconData.szTip, TEXT("Gamma Toggler"));

    Shell_NotifyIcon(NIM_ADD, &g_notifyIconData);
}

void RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &g_notifyIconData);
}

// --- Config File Functions ---
void LoadConfig() {
    // Read from config, using our integer defaults if the file/keys don't exist
    int loadedGammaInt = GetPrivateProfileInt(TEXT("Settings"), TEXT("GammaX100"), DEFAULT_GAMMA_INT, g_configPath);
    int loadedModifier = GetPrivateProfileInt(TEXT("Settings"), TEXT("Modifier"), DEFAULT_MODIFIER_INT, g_configPath);
    int loadedKey = GetPrivateProfileInt(TEXT("Settings"), TEXT("Key"), DEFAULT_KEY_INT, g_configPath);

    // Performing necessary type conversions
    g_targetGamma = loadedGammaInt / 100.0f;
    g_hotkeyModifier = (UINT)loadedModifier;
    g_hotkeyKey = (UINT)loadedKey;

    // Add validation for the loaded gamma value
    if (g_targetGamma < 0.1f || g_targetGamma > 10.0f) {
        g_targetGamma = DEFAULT_GAMMA_INT / 100.0f;
    }
}

void SaveConfig() {
    TCHAR buffer[32];
    _stprintf_s(buffer, TEXT("%d"), (int)(g_targetGamma * 100));
    WritePrivateProfileString(TEXT("Settings"), TEXT("GammaX100"), buffer, g_configPath);

    _stprintf_s(buffer, TEXT("%d"), g_hotkeyModifier);
    WritePrivateProfileString(TEXT("Settings"), TEXT("Modifier"), buffer, g_configPath);

    _stprintf_s(buffer, TEXT("%d"), g_hotkeyKey);
    WritePrivateProfileString(TEXT("Settings"), TEXT("Key"), buffer, g_configPath);
}

