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

const int cols = 10;
const int rows = 20;

const int colWidth = width / cols;
const int rowHeight = height / rows;

HHOOK hKeyboardHook;
HHOOK hMouseHook;
bool isSpaceDown = false;
std::atomic<bool> isCustomDragging(false);

GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;
HDC hdc;

Graphics *graphics = nullptr;
Pen *pen = nullptr;

POINT cursorPoint;

void TrackMousePosition()
{
    while (isCustomDragging.load())
    {

        GetCursorPos(&cursorPoint);
        HWND foregroundWindow = GetForegroundWindow();

        RECT windowRect;
        if (GetWindowRect(foregroundWindow, &windowRect))
        {
            const int windowWidth = windowRect.right - windowRect.left;
            const int windowHeight = windowRect.bottom - windowRect.top;

            const int adjustedWindowWidth = ((windowWidth + colWidth / 2) / colWidth) * colWidth;
            const int adjustedWindowHeight = ((windowHeight + rowHeight / 2) / rowHeight) * rowHeight;

            const int horizontalZone = cursorPoint.x / colWidth;
            const int verticalZone = cursorPoint.y / rowHeight;

            const int startX = horizontalZone * colWidth;
            const int startY = verticalZone * rowHeight;

            MoveWindow(foregroundWindow, startX, startY, adjustedWindowWidth, adjustedWindowHeight, TRUE);
        }
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

void SimulateMouseRelease()
{
    INPUT input = {0};                     // Initialize INPUT structure
    input.type = INPUT_MOUSE;              // Indicate we are sending mouse input
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP; // Indicate a left-button release

    SendInput(1, &input, sizeof(INPUT)); // Send the input event
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        // Check if the event is a mouse button release
        if (wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP)
        {

            std::cout << "Mouse button release\n";
            isCustomDragging = false;
            UnhookWindowsHookEx(hMouseHook);
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

bool isNearEdge(HWND hwnd, POINT pt)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);

    const int EDGE_THRESHOLD = 10; // Define proximity to edge to consider

    if (pt.x - rect.left < EDGE_THRESHOLD || rect.right - pt.x < EDGE_THRESHOLD ||
        pt.y - rect.top < EDGE_THRESHOLD || rect.bottom - pt.y < EDGE_THRESHOLD)
    {
        return true; // Cursor is near an edge
    }
    return false; // Cursor is not near an edge
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

        if (isNearEdge(hwnd, cursorPos))
        {
        }
        else
        {
            SimulateMouseRelease();
            HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            isCustomDragging = true;
            // Start the mouse tracking thread if not already running
            if (mouseTracker.joinable())
            {
                mouseTracker.join(); // Ensure the previous thread is joined before starting a new one
            }
            mouseTracker = std::thread(TrackMousePosition);

            hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
            if (hMouseHook == NULL)
            {
                std::cout << "Failed to set mouse hook" << std::endl;
            }

            std::thread gridThread([hMonitor]()
                                   { drawGrid(hMonitor); });
            gridThread.detach(); // Allows the thread to run independently
        }
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

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hKeyboardHook);
    UnhookWinEvent(moveStartHook);

    CoUninitialize();

    std::cout << "-> Finished Drawing\n";
}