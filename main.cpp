#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <atomic>

using namespace Gdiplus;

#pragma comment(lib, "Gdiplus.lib")

enum class ResizingDirection
{
    None = 0,
    Left,
    Right,
    Top,
    Bottom,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

const int width = 2160;
const int height = 3840;

const int cols = 16;
const int rows = 40;

const int colWidth = width / cols;
const int rowHeight = height / rows;

HHOOK hKeyboardHook;
HHOOK hMouseHook;
bool isSpaceDown = false;
std::atomic<bool> isCustomDragging(false);
std::atomic<bool> isCustomResizing(false);

std::atomic<ResizingDirection> resizingDirection(ResizingDirection::None);

GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;
HDC hdc;

Graphics *graphics = nullptr;
Pen *pen = nullptr;

POINT cursorPoint;
int cursorToWindowOffset = -1;

ResizingDirection GetResizingDirection(HWND hwnd, POINT cursorPos)
{
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);

    const int EDGE_THRESHOLD = 10;
    ResizingDirection direction = ResizingDirection::None;

    bool left = cursorPos.x <= (windowRect.left + EDGE_THRESHOLD);
    bool right = cursorPos.x >= (windowRect.right - EDGE_THRESHOLD);
    bool top = cursorPos.y <= (windowRect.top + EDGE_THRESHOLD);
    bool bottom = cursorPos.y >= (windowRect.bottom - EDGE_THRESHOLD);

    if (top && left)
    {
        direction = ResizingDirection::TopLeft;
    }
    else if (top && right)
    {
        direction = ResizingDirection::TopRight;
    }
    else if (bottom && left)
    {
        direction = ResizingDirection::BottomLeft;
    }
    else if (bottom && right)
    {
        direction = ResizingDirection::BottomRight;
    }
    else if (left)
    {
        direction = ResizingDirection::Left;
    }
    else if (right)
    {
        direction = ResizingDirection::Right;
    }
    else if (top)
    {
        direction = ResizingDirection::Top;
    }
    else if (bottom)
    {
        direction = ResizingDirection::Bottom;
    }

    return direction;
}

void TrackMousePosition()
{
    while (isCustomDragging.load())
    {
        GetCursorPos(&cursorPoint);
        HWND foregroundWindow = GetForegroundWindow();
        RECT windowRect;
        if (GetWindowRect(foregroundWindow, &windowRect))
        {
            if (cursorToWindowOffset == -1)
            {
                cursorToWindowOffset = cursorPoint.x - windowRect.left;
            }

            const int oldWindowWidth = windowRect.right - windowRect.left;
            const int oldWindowHeight = windowRect.bottom - windowRect.top;

            const int newWindowWidth = ((oldWindowWidth + colWidth / 2) / colWidth) * colWidth;
            const int newWindowHeight = ((oldWindowHeight + rowHeight / 2) / rowHeight) * rowHeight;

            const int horizontalZone = (cursorPoint.x - cursorToWindowOffset) / colWidth;
            const int verticalZone = cursorPoint.y / rowHeight;

            const int newCursorX = horizontalZone * colWidth;
            const int newCursorY = verticalZone * rowHeight;

            MoveWindow(foregroundWindow,
                       newCursorX,
                       newCursorY,
                       newWindowWidth,
                       newWindowHeight,
                       TRUE);
        }
    }
    while (isCustomResizing.load())
    {
        GetCursorPos(&cursorPoint);
        HWND foregroundWindow = GetForegroundWindow();
        RECT windowRect;

        if (resizingDirection == ResizingDirection::None)
        {
            resizingDirection = GetResizingDirection(foregroundWindow, cursorPoint);
        }

        if (GetWindowRect(foregroundWindow, &windowRect))
        {
            const int oldWindowWidth = windowRect.right - windowRect.left;
            const int oldWindowHeight = windowRect.bottom - windowRect.top;

            int newWindowX = windowRect.left;
            int newWindowY = windowRect.top;

            int newWindowWidth = ((oldWindowWidth + colWidth / 2) / colWidth) * colWidth;
            int newWindowHeight = ((oldWindowHeight + rowHeight / 2) / rowHeight) * rowHeight;

            int horizontalZone = cursorPoint.x / colWidth;
            int verticalZone = cursorPoint.y / rowHeight;

            int newCursorX = horizontalZone * colWidth;
            int newCursorY = verticalZone * rowHeight;

            switch (resizingDirection) // First Vertically
            {
            case ResizingDirection::Top:
            case ResizingDirection::TopLeft:
            case ResizingDirection::TopRight:
                newWindowY = newCursorY;
                newWindowHeight = newWindowHeight - (newCursorY - windowRect.top);
                break;
            case ResizingDirection::Bottom:
            case ResizingDirection::BottomLeft:
            case ResizingDirection::BottomRight:
                verticalZone = (cursorPoint.y - windowRect.top) / rowHeight;
                newCursorY = verticalZone * rowHeight;
                newWindowHeight = newCursorY;
                break;
            }
            switch (resizingDirection) // Then Horizontally
            {
            case ResizingDirection::Left:
            case ResizingDirection::TopLeft:
            case ResizingDirection::BottomLeft:
                newWindowX = newCursorX;
                newWindowWidth = newWindowWidth - (newCursorX - windowRect.left);
                break;
            case ResizingDirection::Right:
            case ResizingDirection::TopRight:
            case ResizingDirection::BottomRight:
                horizontalZone = (cursorPoint.x - windowRect.left) / colWidth;
                newCursorX = horizontalZone * colWidth;
                newWindowWidth = newCursorX;
                break;
            }

            MoveWindow(foregroundWindow,
                       newWindowX,
                       newWindowY,
                       newWindowWidth,
                       newWindowHeight,
                       TRUE);
        }
    }
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

void SimulateMouseRelease()
{
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(1, &input, sizeof(INPUT));
}

bool isNearEdge(HWND hwnd, POINT pt)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);

    const int EDGE_THRESHOLD = 10;

    if (pt.x - rect.left < EDGE_THRESHOLD || rect.right - pt.x < EDGE_THRESHOLD ||
        pt.y - rect.top < EDGE_THRESHOLD || rect.bottom - pt.y < EDGE_THRESHOLD)
    {
        return true;
    }
    return false;
}

void CALLBACK WinEventProc(HWINEVENTHOOK hook, DWORD event, HWND hwnd,
                           LONG idObject, LONG idChild,
                           DWORD dwEventThread, DWORD dwmsEventTime)
{
    static std::thread mouseTracker;
    if (event == EVENT_SYSTEM_MOVESIZESTART)
    {
        std::cout << "Native window move/size start." << std::endl;
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        SimulateMouseRelease();
        HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

        if (isNearEdge(hwnd, cursorPos))
        {
            std::cout << "Custom Resizing" << std::endl;
            isCustomResizing = true;
        }
        else
        {
            std::cout << "Custom Dragging" << std::endl;
            isCustomDragging = true;
        }
        if (mouseTracker.joinable())
        {
            mouseTracker.join();
        }
        mouseTracker = std::thread(TrackMousePosition);

        std::thread gridThread([hMonitor]()
                               { drawGrid(hMonitor); });
        gridThread.detach(); // Allows the thread to run independently
    }
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (!isCustomDragging)
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

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        if (wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP) // mouse button release
        {
            isCustomDragging = false;
            isCustomResizing = false;
            cursorToWindowOffset = -1;
            resizingDirection = ResizingDirection::None;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main()
{
    std::cout << "-> Starting FancierZones \n";
    CoInitialize(nullptr);

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, nullptr, 0);
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, nullptr, 0);

    HWINEVENTHOOK moveStartHook = SetWinEventHook(
        EVENT_SYSTEM_MOVESIZESTART, EVENT_SYSTEM_MOVESIZESTART,
        NULL,
        WinEventProc,
        0, 0,
        WINEVENT_OUTOFCONTEXT);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hKeyboardHook);
    UnhookWinEvent(moveStartHook);
    UnhookWindowsHookEx(hMouseHook);

    CoUninitialize();

    std::cout << "-> Exiting FancierZones\n";
}