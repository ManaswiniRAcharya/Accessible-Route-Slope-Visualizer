#include "common.h"

enum { OC_IN = 0, OC_LEFT = 1, OC_RIGHT = 2, OC_BOTTOM = 4, OC_TOP = 8 };

// Horizontal and vertical bits are set independently, so corners get two bits
static int outcode(double x, double y, double xmin, double ymin, double xmax, double ymax)
{
    int c = OC_IN;
    if      (x < xmin) c |= OC_LEFT;
    else if (x > xmax) c |= OC_RIGHT;
    if      (y < ymin) c |= OC_BOTTOM;
    else if (y > ymax) c |= OC_TOP;
    return c;
}

int clipLine(double &x0, double &y0, double &x1, double &y1,
             double xmin, double ymin, double xmax, double ymax)
{
    int c0 = outcode(x0, y0, xmin, ymin, xmax, ymax);
    int c1 = outcode(x1, y1, xmin, ymin, xmax, ymax);
    bool clipped = false;

    for (;;)
    {
        if (!(c0 | c1)) return clipped ? 2 : 1;      // both inside: accept
        if (c0 & c1)    return 0;                    // same outside side: reject

        int out = c0 ? c0 : c1;                      // pick an endpoint that is outside
        double x, y;
        if (out & OC_TOP)         { x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0); y = ymax; }
        else if (out & OC_BOTTOM) { x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0); y = ymin; }
        else if (out & OC_RIGHT)  { y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0); x = xmax; }
        else                      { y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0); x = xmin; }

        if (out == c0) { x0 = x; y0 = y; c0 = outcode(x0, y0, xmin, ymin, xmax, ymax); }
        else           { x1 = x; y1 = y; c1 = outcode(x1, y1, xmin, ymin, xmax, ymax); }
        clipped = true;
    }
}

// ---------- Clipping demo (key C) ----------
static int nAcc = 0, nRej = 0, nClip = 0;

void clipDemoStats(int *a, int *r, int *c) { *a = nAcc; *r = nRej; *c = nClip; }

static unsigned seed;
static int rnd(int lo, int hi)           // tiny deterministic generator -> same lines every frame
{
    seed = seed * 1103515245u + 12345u;
    return lo + (int)((seed >> 16) & 0x7fff) % (hi - lo + 1);
}

void drawClipDemo(int w, int h)
{
    nAcc = nRej = nClip = 0;
    seed = 7;

    int xmin = w / 4, xmax = 3 * w / 4;
    int ymin = h * 3 / 10, ymax = h * 7 / 10;

    for (int k = 0; k < 16; k++)
    {
        int ax = rnd(10, w - 10), ay = rnd(60, h - 60);
        int bx = rnd(10, w - 10), by = rnd(60, h - 60);

        // 1. the original line, thin grey
        canvasBrush(1);
        canvasColor(0.72f, 0.72f, 0.72f);
        midpointLine(ax, ay, bx, by);

        // 2. the clipped line, thick colour
        double x0 = ax, y0 = ay, x1 = bx, y1 = by;
        int r = clipLine(x0, y0, x1, y1, xmin, ymin, xmax, ymax);
        if (r == 0) { nRej++; continue; }

        int cx0 = (int)(x0 + 0.5), cy0 = (int)(y0 + 0.5);
        int cx1 = (int)(x1 + 0.5), cy1 = (int)(y1 + 0.5);
        canvasBrush(3);
        if (r == 1) { nAcc++;  canvasColor(0.1f, 0.6f, 0.2f); }    // inside: green
        else        { nClip++; canvasColor(0.85f, 0.15f, 0.15f); } // clipped: red
        midpointLine(cx0, cy0, cx1, cy1);
        canvasBrush(1);

        // 3. blue dots where the clipper created a new endpoint
        canvasColor(0.1f, 0.2f, 0.9f);
        if (cx0 != ax || cy0 != ay) fillCircle(cx0, cy0, 4);
        if (cx1 != bx || cy1 != by) fillCircle(cx1, cy1, 4);
    }

    // the clip window
    canvasColor(0, 0, 0);
    midpointLine(xmin, ymin, xmax, ymin);  midpointLine(xmax, ymin, xmax, ymax);
    midpointLine(xmax, ymax, xmin, ymax);  midpointLine(xmin, ymax, xmin, ymin);
}