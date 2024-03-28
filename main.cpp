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

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;
        if (p->vkCode == VK_SPACE)
        {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
                isSpaceDown = true;
            else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
                isSpaceDown = false;
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        MOUSEHOOKSTRUCT *mouseStruct = (MOUSEHOOKSTRUCT *)lParam;

        switch (wParam)
        {
        case WM_LBUTTONDOWN:
        {
            std::cout << "-> isDragging = true\n";

            // Check if the click is on a window caption
            // This requires additional logic to determine if it's the title bar.
            // For simplicity, we're assuming any click could start a drag.
            isDragging = true;
            break;
        }
        case WM_MOUSEMOVE:
        {
            if (isDragging && isSpaceDown)
            {
                std::cout << "-> Window dragging with space held down.\n";
            }
            break;
        }
        case WM_LBUTTONUP:
        {
            std::cout << "-> isDragging = false\n";

            isDragging = false;
            break;
        }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main()
{
    std::cout << "-> Starting FancierZones \n";

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    hdc = GetDC(NULL);
    graphics = new Graphics(hdc);
    pen = new Pen(Color(255, 0, 0, 0), 5);

    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, nullptr, 0);
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, nullptr, 0);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hKeyboardHook);
    UnhookWindowsHookEx(hMouseHook);

    GdiplusShutdown(gdiplusToken);

    std::cout << "-> Finished Drawing\n";
}