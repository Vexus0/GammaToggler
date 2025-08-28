#pragma once
#include <windows.h>
#include <string>
#include "resource.h"

// Dialog IDs
#define IDD_SET_GAMMA   101
#define IDD_SET_HOTKEY  102

#define IDC_GAMMA_EDIT  1001 // Control ID for Gamma Dialog
#define IDC_HOTKEY_CTRL 1002 // Control ID for Hotkey Dialog

// System Tray and UI Functions
void CenterWindow(HWND hDlg);
void ShowContextMenu(HWND hwnd);
void CreateTrayIcon(HWND hwnd);
void RemoveTrayIcon();
void UpdateTrayIconTip();

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