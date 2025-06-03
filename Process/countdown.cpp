#include <windows.h>
#include <tchar.h>
#include <cstdio>

#define IDT_TIMER1 1

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static int counter = 10;
    static HFONT hFont;

    switch (uMsg)
    {
    case WM_CREATE:
        SetTimer(hwnd, IDT_TIMER1, 1000, NULL);
        hFont = CreateFontW(72, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS,
                            L"Arial");
        break;
    case WM_TIMER:
        if (wParam == IDT_TIMER1)
        {
            counter--;
            InvalidateRect(hwnd, NULL, TRUE);
            if (counter < 0)
            {
                KillTimer(hwnd, IDT_TIMER1);
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
        }
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        SetBkColor(hdc, RGB(255, 255, 255));
        SetTextColor(hdc, RGB(0, 0, 0));
        SelectObject(hdc, hFont);
        WCHAR szCounter[16];
        swprintf(szCounter, 16, L"%d", counter);
        RECT rect;
        GetClientRect(hwnd, &rect);
        DrawTextW(hdc, szCounter, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        EndPaint(hwnd, &ps);
    }
    break;
    case WM_DESTROY:
        DeleteObject(hFont);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    const wchar_t szWindowClass[] = L"CountdownApp";
    const wchar_t szTitle[] = L"Countdown Timer";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = szWindowClass;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!hwnd)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
