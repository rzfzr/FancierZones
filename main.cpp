#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <stdio.h>
#include <iostream>

using namespace Gdiplus;

#pragma comment(lib, "Gdiplus.lib")

int width = 2160;
int height = 3840;

int cols = 10;
int rows = 20;

HHOOK hKeyboardHook;
HHOOK hMouseHook;
bool isSpaceDown = false;
bool isDragging = false;

GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;
HDC hdc;

Graphics *graphics = nullptr;
Pen *pen = nullptr;

void drawVerticalLine(int x, Graphics &graphics, Pen &pen)
{
    graphics.DrawLine(&pen, x, 0, x, height);
}

void drawHorizontalLine(int y, Graphics &graphics, Pen &pen)
{
    graphics.DrawLine(&pen, 0, y, width, y);
}

void drawGrid()
{
    if (graphics && pen)
    {
        std::cout << "-> Drawing \n";

        for (int i = 0; i < cols; i++)
        {
            graphics->DrawLine(pen, i * width / cols, 0, i * width / cols, height);
        }
        for (int i = 0; i < rows; i++)
        {
            graphics->DrawLine(pen, 0, i * height / rows, width, i * height / rows);
        }
    }
    else
    {
        std::cout << "-> Not Drawing \n";
    }
}

void CALLBACK WinEventProc(HWINEVENTHOOK hook, DWORD event, HWND hwnd,
                           LONG idObject, LONG idChild,
                           DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (event == EVENT_SYSTEM_MOVESIZESTART)
    {
        std::cout << "Window move/size start." << std::endl;
        isDragging = true;
        drawGrid();
    }
    else if (event == EVENT_SYSTEM_MOVESIZEEND)
    {
        std::cout << "Window move/size end." << std::endl;
        isDragging = false;
    }
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;
        if (p->vkCode == VK_SPACE)
        {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
            {
                isSpaceDown = true;
                std::cout << "-> Working \n";
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

    hdc = GetDC(NULL);
    graphics = new Graphics(hdc);
    pen = new Pen(Color(255, 0, 0, 0), 5);

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