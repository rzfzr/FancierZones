#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <stdio.h>
using namespace Gdiplus;

#include <iostream>

using namespace Gdiplus;
#pragma comment(lib, "Gdiplus.lib")
VOID OnPaint(HDC hdc)
{
    Graphics graphics(hdc);
    Pen pen(Color(255, 0, 0, 0), 5);
    graphics.DrawLine(&pen, 0, 0, 200, 100);
}

int main()
{

    // GDI+ startup incantation
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Untitled - Notepad
    HWND hWnd = FindWindow(NULL, TEXT("Untitled - Notepad"));
    if (hWnd == 0)
    {
        std::cout << "[-] - Unable to locate window!\n";
        return 0;
    }
    std::cout << "[+] - Located Window, starting hook.\n";
    HDC hdc;
    hdc = GetDC(hWnd);
    std::cout << hdc;
    OnPaint(hdc);
    std::cout << "Finished Drawing\n";
}