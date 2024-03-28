#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <stdio.h>
using namespace Gdiplus;

#include <iostream>

using namespace Gdiplus;
#pragma comment(lib, "Gdiplus.lib")

int width = 800;
int height = 600;

void draw(int x1, int y1, int x2, int y2)
{
    HDC hdc;
    hdc = GetDC(NULL);
    std::cout << hdc;
    Graphics graphics(hdc);
    Pen pen(Color(255, 0, 0, 0), 5);
    graphics.DrawLine(&pen, x1, y1, x1, y2);
}

int main()
{
    std::cout << "-> Starting FancierZones \n";

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    draw(100, 0, 0, 3840);
    std::cout << "Finished Drawing\n";
}