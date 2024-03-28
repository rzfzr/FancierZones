#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <stdio.h>
using namespace Gdiplus;

#include <iostream>

using namespace Gdiplus;
#pragma comment(lib, "Gdiplus.lib")

int width = 2160;
int height = 3840;

int cols = 10;
int rows = 10;

void drawVerticalLine(int x, HDC hdc)
{
    Graphics graphics(hdc);
    Pen pen(Color(255, 0, 0, 0), 5);

    graphics.DrawLine(&pen, x, 0, x, height);
}

int main()
{
    std::cout << "-> Starting FancierZones \n";

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    HDC hdc;
    hdc = GetDC(NULL);

    for (int i = 0; i < cols; i++)
    {
        drawVerticalLine(i * width / cols, hdc);
    }

    std::cout << "Finished Drawing\n";
}