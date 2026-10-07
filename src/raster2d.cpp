#include <GL/glut.h>
#include <cstdio>
#include <cmath>
#include "common.h"

static bool hillshade   = true;
static bool showGrid    = true;
static bool testPattern = false;
static bool clipDemo    = false;

void toggleHillshade()   { hillshade = !hillshade; }
void toggleGrid()        { showGrid = !showGrid; }
void toggleTestPattern() { testPattern = !testPattern; if (testPattern) clipDemo = false; }
void toggleClipDemo()    { clipDemo = !clipDemo;       if (clipDemo) testPattern = false; }

// ---------- window (world, in grid units) and viewport (screen, in pixels) ----------
#define NCELL  (GRID_N - 1)
#define MIN_WS 6.0f                       // most zoomed-in window: 6 cells = 12 m

static float wx0 = 0, wz0 = 0, ws = NCELL;   // window: lower-left corner and (square) size
static float mapX = 0, mapY = 0, S = 0;      // viewport: lower-left corner and size in pixels

static void clampWindow()
{
    if (ws < MIN_WS) ws = MIN_WS;
    if (ws > NCELL)  ws = NCELL;
    if (wx0 < 0) wx0 = 0;
    if (wx0 > NCELL - ws) wx0 = NCELL - ws;
    if (wz0 < 0) wz0 = 0;
    if (wz0 > NCELL - ws) wz0 = NCELL - ws;
}

// WINDOW -> VIEWPORT  (+z points DOWN the screen, as in the 3D view)
static float vx(float gx) { return mapX + (gx - wx0) * (S / ws); }
static float vy(float gz) { return mapY + (wz0 + ws - gz) * (S / ws); }
static int   rnd(float v) { return (int)floorf(v + 0.5f); }

void mapZoomAt(float px, float py, float factor)
{
    if (S <= 0) return;
    float sc = S / ws;
    float gx = wx0 + (px - mapX) / sc;                // grid point under the cursor
    float gz = wz0 + ws - (py - mapY) / sc;

    ws *= factor;
    if (ws < MIN_WS) ws = MIN_WS;
    if (ws > NCELL)  ws = NCELL;

    float sc2 = S / ws;                               // keep that point under the cursor
    wx0 = gx - (px - mapX) / sc2;
    wz0 = gz - ws + (py - mapY) / sc2;
    clampWindow();
}

void mapZoomCenter(float factor) { mapZoomAt(mapX + S / 2, mapY + S / 2, factor); }

void mapPanPixels(float dx, float dy)
{
    if (S <= 0) return;
    float sc = S / ws;
    wx0 -= dx / sc;          // the map follows the cursor, so the window moves the other way
    wz0 += dy / sc;
    clampWindow();
}

void mapResetView() { wx0 = 0; wz0 = 0; ws = NCELL; }

// ---------- helpers ----------
static void fillRect(int x0, int y0, int x1, int y1)
{
    int xs[4] = {x0, x1, x1, x0};
    int ys[4] = {y0, y0, y1, y1};
    scanFillPolygon(xs, ys, 4);
}

// Clip a grid-space line to the window (Cohen-Sutherland), map it to pixels, draw it
static void gridLine(double ax, double az, double bx, double bz)
{
    if (clipLine(ax, az, bx, bz, wx0, wz0, wx0 + ws, wz0 + ws) == 0) return;
    midpointLine(rnd((float)vx((float)ax)), rnd(vy((float)az)),
                 rnd((float)vx((float)bx)), rnd(vy((float)bz)));
}

static void text(float x, float y, const char *s)
{
    glRasterPos2f(x, y);
    for (; *s; s++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s);
}

// ---------- the slope map, drawn through the window ----------
static void drawMap()
{
    canvasSetClip((int)mapX, (int)mapY, (int)(mapX + S), (int)(mapY + S));   // pixel scissor

    float L[3];
    getSunDir(L);

    // only visit the cells that touch the window
    int i0 = (int)floorf(wx0), i1 = (int)ceilf(wx0 + ws) - 1;
    int j0 = (int)floorf(wz0), j1 = (int)ceilf(wz0 + ws) - 1;
    if (i0 < 0) i0 = 0;   if (i1 > NCELL - 1) i1 = NCELL - 1;
    if (j0 < 0) j0 = 0;   if (j1 > NCELL - 1) j1 = NCELL - 1;

    for (int j = j0; j <= j1; j++)
        for (int i = i0; i <= i1; i++)
        {
            float c[3];
            slopeClassColor(getCellClass(i, j), c);
            if (hillshade)
            {
                float n[3];
                getCellNormal(i, j, n);
                float s = 0.35f + 0.65f * lambert(n, L);
                c[0] *= s;  c[1] *= s;  c[2] *= s;
            }
            canvasColor(c[0], c[1], c[2]);

            // clip the cell rectangle to the window (clamp), then window -> viewport
            float gx0 = fmaxf((float)i,       wx0), gx1 = fminf((float)(i + 1), wx0 + ws);
            float gz0 = fmaxf((float)j,       wz0), gz1 = fminf((float)(j + 1), wz0 + ws);
            int px0 = rnd(vx(gx0)), px1 = rnd(vx(gx1));
            int pyTop = rnd(vy(gz0)), pyBot = rnd(vy(gz1));
            if (px1 <= px0 || pyTop <= pyBot) continue;
            fillRect(px0, pyBot, px1, pyTop);
        }

    // grid: every 10 m, or every cell (2 m) when zoomed in
    if (showGrid)
    {
        canvasColor(0.25f, 0.25f, 0.25f);
        int step = (ws <= 15.0f) ? 1 : 5;
        for (int k = 0; k < GRID_N; k += step)
        {
            gridLine(k, 0, k, NCELL);
            gridLine(0, k, NCELL, k);
        }
    }

    // terrain border
    canvasColor(0, 0, 0);
    gridLine(0, 0, NCELL, 0);          gridLine(NCELL, 0, NCELL, NCELL);
    gridLine(NCELL, NCELL, 0, NCELL);  gridLine(0, NCELL, 0, 0);

    // placeholder start -> goal (Phase 6 replaces these)
    canvasColor(0.1f, 0.1f, 0.1f);
    canvasBrush(2);
    gridLine(4, 8, 32, 9);
    canvasBrush(1);

    int sx = rnd(vx(4)),  sy = rnd(vy(8));
    int gx = rnd(vx(32)), gy = rnd(vy(9));
    canvasColor(0.1f, 0.35f, 0.9f);   fillCircle(sx, sy, 6);
    canvasColor(0, 0, 0);             midpointCircle(sx, sy, 7);
    canvasColor(0.7f, 0.1f, 0.8f);    fillCircle(gx, gy, 6);
    canvasColor(0, 0, 0);             midpointCircle(gx, gy, 7);

    canvasClearClip();

    // viewport frame (always visible, even when zoomed)
    int fx0 = (int)mapX - 1, fy0 = (int)mapY - 1;
    int fx1 = (int)(mapX + S) + 1, fy1 = (int)(mapY + S) + 1;
    canvasColor(0, 0, 0);
    midpointLine(fx0, fy0, fx1, fy0);  midpointLine(fx1, fy0, fx1, fy1);
    midpointLine(fx1, fy1, fx0, fy1);  midpointLine(fx0, fy1, fx0, fy0);
}

void draw2DPanel(int w, int h)
{
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    canvasBegin(w, h);

    const float margin = 20.0f;
    S    = (w - 2 * margin < h - 2 * margin) ? (w - 2 * margin) : (h - 2 * margin);
    mapX = margin;
    mapY = h - margin - S;
    clampWindow();

    bool mapMode = !testPattern && !clipDemo;

    if (testPattern)      drawTestPattern(w, h);
    else if (clipDemo)    drawClipDemo(w, h);
    else                  drawMap();

    int total = NCELL * NCELL;
    const char *labels[3] = {
        "Accessible  (<= 1:20, 5%)",
        "Steep ramp  (<= 1:12, 8.3%)",
        "Not accessible (steeper)"
    };
    if (mapMode)
        for (int k = 0; k < 3; k++)
        {
            int y = (int)(mapY - 56 - k * 22);
            float c[3];
            slopeClassColor(k, c);
            canvasColor(c[0], c[1], c[2]);
            fillRect((int)margin, y, (int)margin + 14, y + 14);
        }

    canvasColor(0.6f, 0.6f, 0.6f);                                   // panel border
    midpointLine(1, 1, w - 2, 1);          midpointLine(w - 2, 1, w - 2, h - 2);
    midpointLine(w - 2, h - 2, 1, h - 2);  midpointLine(1, h - 2, 1, 1);

    canvasFlush();

    // text after the flush so it sits on top
    glColor3f(0, 0, 0);
    char buf[128];
    if (testPattern)
    {
        text(10, h - 18, "TEST PATTERN [K]: 24 midpoint lines, 3 circles, 2 fills");
        return;
    }
    if (clipDemo)
    {
        int a, r, c;
        clipDemoStats(&a, &r, &c);
        text(10, h - 18, "CLIP DEMO [C]: Cohen-Sutherland, 16 lines");
        sprintf(buf, "Green: inside %d   Red: clipped %d   Grey only: rejected %d", a, c, r);
        text(10, h - 34, buf);
        text(10, h - 50, "Blue dots = new endpoints on the window edge");
        return;
    }

    text(margin, mapY - 32, "Slope map (top-down, own raster algorithms)");
    for (int k = 0; k < 3; k++)
    {
        sprintf(buf, "%s : %.1f%%", labels[k], 100.0f * getClassCount(k) / total);
        text(margin + 22, mapY - 56 - k * 22 + 2, buf);
    }
    text(margin, mapY - 56 - 3 * 22 - 6,
         hillshade ? "Hillshade ON [M]   Grid [G]" : "Hillshade OFF [M]   Grid [G]");
    sprintf(buf, "Zoom x%.1f   window %.0f m wide   grid %d m",
            NCELL / ws, ws * CELL_SIZE, ((ws <= 15.0f) ? 1 : 5) * (int)CELL_SIZE);
    text(margin, mapY - 56 - 3 * 22 - 24, buf);
    text(margin, mapY - 56 - 3 * 22 - 42, "Wheel/Z zoom  drag pan  V reset  C clip demo");
}