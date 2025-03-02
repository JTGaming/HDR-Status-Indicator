#pragma once
#include <windows.h>
#include <commctrl.h>
#include <dxgi1_6.h>
#include <atlstr.h>
#include <comutil.h>

enum DISPMODE : UINT
{
    PADDING = 0,
    SDR,
    HDR,
    INVALID
};

void                RegisterWindowClass(PCWSTR pszClassName, WNDPROC lpfnWndProc);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void                ShowContextMenu(HWND hwnd);
BOOL                AddIcon(HWND hwnd);
void                UpdateIcon(DISPMODE channel);
BOOL                DeleteIcon();
DISPMODE            CheckDisplayMode();
void                MainLoop();
