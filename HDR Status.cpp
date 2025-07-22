// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "HDR Status.h"
#include "resource.h"

// we need commctrl v6 for LoadIconMetric()
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comsuppw.lib")
#pragma comment (lib, "dxgi.lib")

HINSTANCE g_hInst = NULL;
UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
wchar_t const szWindowClass[] = L"HDR Status";
// Use a guid to uniquely identify our icon
class __declspec(uuid("1f97e126-cf00-466f-bd3a-10f45a024802")) NotifIcon;
HWND main_hwnd = NULL;

int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE,
    _In_ LPWSTR,
    _In_ int
)
{
    // Get class id as string
    LPOLESTR className;
    HRESULT hr = StringFromCLSID(__uuidof(NotifIcon), &className);
    if (hr != S_OK)
        return -1;

    // convert to CString
    CString c = (char*)(_bstr_t)className;
    // then release the memory used by the class name
    CoTaskMemFree(className);

    CreateMutex(0, FALSE, c); // try to create a named mutex
    if (GetLastError() == ERROR_ALREADY_EXISTS) // did the mutex already exist?
        return -1; // quit; mutex is released automatically

    SetProcessDPIAware();
    g_hInst = hInstance;
    RegisterWindowClass(szWindowClass, WndProc);

    // Create the main window. This could be a hidden window if you don't need
    // any UI other than the notification icon.
    main_hwnd = CreateWindow(szWindowClass, szWindowClass, 0, 0, 0, 0, 0, 0, 0, g_hInst, 0);
    if (main_hwnd)
        MainLoop();

    return 0;
}

void MainLoop()
{
    bool CanRun = true;
    while (CanRun)
    {
        Sleep(500);

        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                CanRun = false;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        DISPMODE curr_mode = CheckDisplayMode();
        UpdateIcon(curr_mode);
    }
}

DISPMODE CheckDisplayMode()
{
    HRESULT hr;
    DXGI_OUTPUT_DESC1 desc1;

    IDXGIOutput6* output6;
    IDXGIFactory* factory;
    IDXGIAdapter* adapter;
    IDXGIOutput* adapterOutput;

    hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
    if (FAILED(hr))
        return INVALID;
    hr = factory->EnumAdapters(0, &adapter);
    if (FAILED(hr))
    {
        factory->Release();
        return INVALID;
    }
    hr = adapter->EnumOutputs(0, &adapterOutput);
    if (FAILED(hr))
    {
        factory->Release();
        adapter->Release();
        return INVALID;
    }
    hr = adapterOutput->QueryInterface(__uuidof(IDXGIOutput6), (void**)&output6);
    if (FAILED(hr))
    {
        factory->Release();
        adapter->Release();
        adapterOutput->Release();
        return INVALID;
    }
    hr = output6->GetDesc1(&desc1);

    factory->Release();
    adapter->Release();
    adapterOutput->Release();
    output6->Release();

    if (FAILED(hr))
        return INVALID;

    return desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 ? HDR : SDR;
}

void RegisterWindowClass(PCWSTR pszClassName, WNDPROC lpfnWndProc)
{
    WNDCLASSEX wcex = { sizeof(wcex) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = lpfnWndProc;
    wcex.hInstance = g_hInst;
    wcex.lpszClassName = pszClassName;
    RegisterClassEx(&wcex);
}

BOOL AddIcon(HWND hwnd)
{
    NOTIFYICONDATA nid = { sizeof(nid) };
    nid.hWnd = hwnd;
    // add the icon, setting the icon, tooltip, and callback message.
    // the icon will be identified with the GUID
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_GUID;
    nid.guidItem = __uuidof(NotifIcon);
    nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
    LoadIconMetric(g_hInst, MAKEINTRESOURCE(IDI_NOTIFICATIONICONIDX + (int)SDR), LIM_SMALL, &nid.hIcon);
    BOOL ret = Shell_NotifyIcon(NIM_ADD, &nid);
    if (ret != TRUE)
        return FALSE;

    // NOTIFYICON_VERSION_4 is prefered
    nid.uVersion = NOTIFYICON_VERSION_4;
    return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

void UpdateIcon(DISPMODE mode)
{
    static DISPMODE old_mode = PADDING;
    if (mode == old_mode)
        return;

    int IDX = (int)mode + IDI_NOTIFICATIONICONIDX;
    NOTIFYICONDATA nid = { sizeof(nid) };
    nid.uFlags = NIF_ICON | NIF_GUID;
    nid.guidItem = __uuidof(NotifIcon);

    HRESULT hr = LoadIconMetric(g_hInst, MAKEINTRESOURCE(IDX), LIM_SMALL, &nid.hIcon);
    if (hr != S_OK)
        return;
    BOOL ret = Shell_NotifyIcon(NIM_MODIFY, &nid);
    if (ret != TRUE)
    {
        DeleteIcon();
        AddIcon(main_hwnd);
        old_mode = PADDING;
        return;
    }

    old_mode = mode;

}

BOOL DeleteIcon()
{
    NOTIFYICONDATA nid = { sizeof(nid) };
    nid.uFlags = NIF_GUID;
    nid.guidItem = __uuidof(NotifIcon);
    return Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ShowContextMenu(HWND hwnd)
{
    HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDC_CONTEXTMENU));
    if (hMenu)
    {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu)
        {
            // our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
            SetForegroundWindow(hwnd);

            // respect menu drop alignment
            UINT uFlags = TPM_RIGHTBUTTON;
            if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
                uFlags |= TPM_RIGHTALIGN;
            else
                uFlags |= TPM_LEFTALIGN;
            POINT pt;
            GetCursorPos(&pt);
            TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
        }
        DestroyMenu(hMenu);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        // add the notification icon
        if (!AddIcon(hwnd))
            return -1;
        break;
    case WM_COMMAND:
    {
        // Parse the menu selections:
        switch (LOWORD(wParam))
        {
        case IDM_EXIT:
            DestroyWindow(hwnd);
            break;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
    }
    break;

    case WMAPP_NOTIFYCALLBACK:
        switch (LOWORD(lParam))
        {
        case WM_CONTEXTMENU:
            ShowContextMenu(hwnd);
            break;
        }
        break;

    case WM_DESTROY:
        DeleteIcon();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}
