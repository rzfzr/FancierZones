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
int rows = 20;

void drawVerticalLine(int x, Graphics &graphics, Pen &pen)
{
    graphics.DrawLine(&pen, x, 0, x, height);
}

void drawHorizontalLine(int y, Graphics &graphics, Pen &pen)
{
    graphics.DrawLine(&pen, 0, y, width, y);
}

int main()
{
    std::cout << "-> Starting FancierZones \n";

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    HDC hdc;
    hdc = GetDC(NULL);

    Graphics graphics(hdc);
    Pen pen(Color(255, 0, 0, 0), 5);

    for (int i = 0; i < cols; i++)
    {
        drawVerticalLine(i * width / cols, graphics, pen);
    }
    for (int i = 0; i < rows; i++)
    {
        drawHorizontalLine(i * height / rows, graphics, pen);
    }

    std::cout << "Finished Drawing\n";
}