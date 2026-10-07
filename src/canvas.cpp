#include <GL/glut.h>
#include <cstdlib>
#include <cstring>
#include "common.h"

#define MAXW 2048
#define MAXH 2048

static unsigned char buf[MAXW * MAXH * 3];     // RGB, row 0 = BOTTOM row (like OpenGL)
static int cw = 0, ch = 0;
static int brush = 1;
static unsigned char cr = 0, cg = 0, cb = 0;
static int  clx0 = 0, cly0 = 0, clx1 = 0, cly1 = 0;
static bool clipOn = false;

void canvasSetClip(int x0, int y0, int x1, int y1)
{
    clx0 = x0; cly0 = y0; clx1 = x1; cly1 = y1;
    clipOn = true;
}
void canvasClearClip() { clipOn = false; }


int canvasW() { return cw; }
int canvasH() { return ch; }

void canvasBegin(int w, int h)
{
    cw = (w > MAXW) ? MAXW : w;
    ch = (h > MAXH) ? MAXH : h;
    memset(buf, 255, (size_t)cw * ch * 3);       // white background
    brush = 1;
    clipOn = false;
}

static unsigned char toByte(float v)
{
    if (v < 0) v = 0;
    if (v > 1) v = 1;
    return (unsigned char)(v * 255.0f + 0.5f);
}

void canvasColor(float r, float g, float b) { cr = toByte(r); cg = toByte(g); cb = toByte(b); }
void canvasBrush(int size)                  { brush = (size < 1) ? 1 : size; }

// The only function that writes pixels. The bounds check is a SAFETY net
// (so we never write outside the array). It is NOT clipping: real clipping is Phase 5.
void canvasPixel(int x, int y)
{
    int x0 = x - brush / 2;
    int y0 = y - brush / 2;
    for (int dy = 0; dy < brush; dy++)
        for (int dx = 0; dx < brush; dx++)
        {
            int px = x0 + dx, py = y0 + dy;
            if (px < 0 || px >= cw || py < 0 || py >= ch) continue;
                        if (clipOn && (px < clx0 || px > clx1 || py < cly0 || py > cly1)) continue;
            unsigned char *p = &buf[((size_t)py * cw + px) * 3];
            p[0] = cr; p[1] = cg; p[2] = cb;
        }
}

void canvasHLine(int x0, int x1, int y)
{
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    for (int x = x0; x <= x1; x++) canvasPixel(x, y);
}

void canvasFlush()
{
    glDisable(GL_DEPTH_TEST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);       // rows are not padded to 4 bytes
    glRasterPos2i(0, 0);                         // bottom-left of the panel (ortho = pixel units)
    glDrawPixels(cw, ch, GL_RGB, GL_UNSIGNED_BYTE, buf);
}

// ---------- Midpoint line, all octants (generalised Lab 1) ----------
void midpointLine(int x0, int y0, int x1, int y1)
{
    bool steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) { int t; t = x0; x0 = y0; y0 = t;  t = x1; x1 = y1; y1 = t; }   // swap x and y
    if (x0 > x1) { int t; t = x0; x0 = x1; x1 = t;  t = y0; y0 = y1; y1 = t; } // left to right

    int dx = x1 - x0;
    int dy = abs(y1 - y0);
    int ystep = (y0 < y1) ? 1 : -1;

    int d = 2 * dy - dx;                 // decision variable (Lab 1)
    int incrE  = 2 * dy;                 // choose E : y stays
    int incrNE = 2 * (dy - dx);          // choose NE: y moves one step
    int y = y0;

    for (int x = x0; x <= x1; x++)
    {
        if (steep) canvasPixel(y, x); else canvasPixel(x, y);   // swap back when plotting
        if (d <= 0) d += incrE;
        else      { d += incrNE; y += ystep; }
    }
}