#include <cmath>
#include <cstdlib>
#include "common.h"

// ---------- Midpoint circle (Lab 2) ----------
static void plot8(int cx, int cy, int x, int y)
{
    canvasPixel(cx + x, cy + y);  canvasPixel(cx + y, cy + x);
    canvasPixel(cx + y, cy - x);  canvasPixel(cx + x, cy - y);
    canvasPixel(cx - x, cy - y);  canvasPixel(cx - y, cy - x);
    canvasPixel(cx - y, cy + x);  canvasPixel(cx - x, cy + y);
}

void midpointCircle(int cx, int cy, int r)
{
    int x = 0, y = r;
    int d = 1 - r;                        // integer form of the lab's 5/4 - r
    plot8(cx, cy, x, y);
    while (y > x)
    {
        if (d < 0) d += 2 * x + 3;
        else     { d += 2 * (x - y) + 5; y--; }
        x++;
        plot8(cx, cy, x, y);
    }
}

// Same loop, but joins mirrored points with horizontal spans -> solid disc
void fillCircle(int cx, int cy, int r)
{
    int x = 0, y = r;
    int d = 1 - r;
    for (;;)
    {
        canvasHLine(cx - x, cx + x, cy + y);
        canvasHLine(cx - x, cx + x, cy - y);
        canvasHLine(cx - y, cx + y, cy + x);
        canvasHLine(cx - y, cx + y, cy - x);
        if (y <= x) break;
        if (d < 0) d += 2 * x + 3;
        else     { d += 2 * (x - y) + 5; y--; }
        x++;
    }
}

// ---------- Scan-line polygon fill (Lab 4) ----------
#define MAXH 2048
static float leX[MAXH], reX[MAXH];        // left / right edge per scanline

void scanFillPolygon(const int px[], const int py[], int n)
{
    if (n < 3) return;
    int H = canvasH();

    int ymin = py[0], ymax = py[0];
    for (int k = 1; k < n; k++)
    {
        if (py[k] < ymin) ymin = py[k];
        if (py[k] > ymax) ymax = py[k];
    }
    int yStart = (ymin < 0) ? 0 : ymin;
    int yEnd   = (ymax > H) ? H : ymax;            // scanlines yStart .. yEnd-1
    for (int y = yStart; y < yEnd; y++) { leX[y] = 1e9f; reX[y] = -1e9f; }

    // edge detection: walk every edge, update le/re (this is Lab 4's edgedetect)
    for (int k = 0; k < n; k++)
    {
        int x1 = px[k],           y1 = py[k];
        int x2 = px[(k + 1) % n], y2 = py[(k + 1) % n];
        if (y1 == y2) continue;                     // horizontal edges add nothing
        if (y1 > y2) { int t; t = x1; x1 = x2; x2 = t;  t = y1; y1 = y2; y2 = t; }

        for (int y = y1; y < y2; y++)               // top end excluded -> no double rows
        {
            if (y < 0 || y >= H) continue;
            float x = x1 + (float)(x2 - x1) * (y - y1) / (float)(y2 - y1);
            if (x < leX[y]) leX[y] = x;
            if (x > reX[y]) reX[y] = x;
        }
    }

    // fill from left edge up to (not including) right edge
    for (int y = yStart; y < yEnd; y++)
    {
        if (leX[y] > reX[y]) continue;
        int xs = (int)floorf(leX[y] + 0.5f);
        int xe = (int)floorf(reX[y] + 0.5f);
        if (xe > xs) canvasHLine(xs, xe - 1, y);
    }
}

// ---------- Test pattern: proves every octant, circle and fill works ----------
void drawTestPattern(int w, int h)
{
    int cx = w / 2, cy = h / 2;
    int R = ((w < h) ? w : h) / 2 - 30;

    // 24 rays, every 15 degrees -> each of the 8 octants gets 3 lines
    for (int a = 0; a < 360; a += 15)
    {
        float rad = a * 3.14159265f / 180.0f;
        int ex = cx + (int)(R * cosf(rad));
        int ey = cy + (int)(R * sinf(rad));
        if ((a / 15) % 2 == 0) canvasColor(0.85f, 0.1f, 0.1f);
        else                   canvasColor(0.1f, 0.2f, 0.85f);
        midpointLine(cx, cy, ex, ey);
    }

    canvasColor(0.0f, 0.0f, 0.0f);
    midpointCircle(cx, cy, R / 4);
    midpointCircle(cx, cy, R / 2);
    midpointCircle(cx, cy, R);

    canvasColor(0.2f, 0.7f, 0.3f);
    fillCircle(cx, cy, R / 10);

    // filled triangle (bottom-left) and filled disc (bottom-right)
    int tx[3] = {20, 150, 70};
    int ty[3] = {20, 30, 120};
    canvasColor(0.9f, 0.6f, 0.1f);
    scanFillPolygon(tx, ty, 3);

    canvasColor(0.5f, 0.2f, 0.7f);
    fillCircle(w - 60, 70, 40);
}