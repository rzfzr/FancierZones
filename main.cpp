#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <atomic>

using namespace Gdiplus;

#pragma comment(lib, "Gdiplus.lib")

int width = 2160;
int height = 3840;

int cols = 10;
int rows = 20;

HHOOK hKeyboardHook;
HHOOK hMouseHook;
bool isSpaceDown = false;
std::atomic<bool> isDragging(false);

GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;
HDC hdc;

Graphics *graphics = nullptr;
Pen *pen = nullptr;

POINT cursorPoint;

void TrackMousePosition()
{
    POINT cursorPoint;
    while (isDragging.load())
    {
        GetCursorPos(&cursorPoint);
        std::cout << "Mouse position: " << cursorPoint.x << ", " << cursorPoint.y << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void drawVerticalLine(int x, Graphics &graphics, Pen &pen)
{
    graphics.DrawLine(&pen, x, 0, x, height);
}

void drawHorizontalLine(int y, Graphics &graphics, Pen &pen)
{
    graphics.DrawLine(&pen, 0, y, width, y);
}

void drawGrid(HMONITOR hMonitor)
{
    MONITORINFOEX miex;
    miex.cbSize = sizeof(MONITORINFOEX);

    if (GetMonitorInfo(hMonitor, (LPMONITORINFO)&miex))
    {

        hdc = CreateDC(TEXT("DISPLAY"), miex.szDevice, NULL, NULL);

        graphics = new Graphics(hdc);
        pen = new Pen(Color(255, 0, 0, 0), 5);
        std::cout << "-> Drawing \n";

        for (int i = 0; i < cols; i++)
        {
            graphics->DrawLine(pen, i * width / cols, 0, i * width / cols, height);
        }
        for (int i = 0; i < rows; i++)
        {
            graphics->DrawLine(pen, 0, i * height / rows, width, i * height / rows);
        }

        DeleteDC(hdc);
    }
}

void CALLBACK WinEventProc(HWINEVENTHOOK hook, DWORD event, HWND hwnd,
                           LONG idObject, LONG idChild,
                           DWORD dwEventThread, DWORD dwmsEventTime)
{
    static std::thread mouseTracker; // Keep track of the thread

    if (event == EVENT_SYSTEM_MOVESIZESTART)
    {
        HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        std::cout << "Window move/size start." << std::endl;
        drawGrid(hMonitor);
        isDragging = true;
        // Start the mouse tracking thread if not already running
        if (mouseTracker.joinable())
        {
            mouseTracker.join(); // Ensure the previous thread is joined before starting a new one
        }
        mouseTracker = std::thread(TrackMousePosition);
    }
    else if (event == EVENT_SYSTEM_MOVESIZEEND)
    {
        std::cout << "Window move/size end." << std::endl;
        isDragging = false;
        if (mouseTracker.joinable())
        {
            mouseTracker.join(); // Wait for the mouse tracking thread to finish
        }
    }
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (!isDragging)
    {
        return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
    }

    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;
        if (p->vkCode == VK_SPACE)
        {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
            {
                isSpaceDown = true;
                GetCursorPos(&cursorPoint);
                std::cout << "-> Working " << cursorPoint.x << " " << cursorPoint.y << "\n";
            }
            else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
            {
                isSpaceDown = false;
            }
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

int main()
{
    std::cout << "-> Starting FancierZones \n";
    CoInitialize(nullptr);

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, nullptr, 0);

    HWINEVENTHOOK moveStartHook = SetWinEventHook(
        EVENT_SYSTEM_MOVESIZESTART, EVENT_SYSTEM_MOVESIZESTART, // Range of events
        NULL,                                                   // Handle to DLL with the callback function, NULL means current process
        WinEventProc,                                           // Pointer to the callback function
        0, 0,                                                   // Process and thread ID, 0 = all processes and threads
        WINEVENT_OUTOFCONTEXT                                   // Events are ASYNC
    );

    HWINEVENTHOOK moveEndHook = SetWinEventHook(
        EVENT_SYSTEM_MOVESIZEEND, EVENT_SYSTEM_MOVESIZEEND, // Range of events
        NULL,                                               // Handle to DLL with the callback function, NULL means current process
        WinEventProc,                                       // Pointer to the callback function
        0, 0,                                               // Process and thread ID, 0 = all processes and threads
        WINEVENT_OUTOFCONTEXT                               // Events are ASYNC
    );
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hKeyboardHook);
    UnhookWinEvent(moveStartHook);
    UnhookWinEvent(moveEndHook);

    CoUninitialize();

    std::cout << "-> Finished Drawing\n";
}