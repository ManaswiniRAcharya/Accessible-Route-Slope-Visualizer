#include <GL/glut.h>
#include <cstdio>
#include "common.h"

static bool hillshade   = true;
static bool showGrid    = true;
static bool testPattern = false;

void toggleHillshade()   { hillshade = !hillshade; }
void toggleGrid()        { showGrid = !showGrid; }
void toggleTestPattern() { testPattern = !testPattern; }

// ---------- map layout (shared by every helper below) ----------
static float mapX, mapY, cell;       // bottom-left corner and cell size in pixels

// grid vertex (i, j) -> pixel. Rounded the same way everywhere, so neighbouring
// cells share identical corners (no gaps). +z points DOWN the screen, as in the 3D view.
static int gridPx(int i) { return (int)(mapX + i * cell + 0.5f); }
static int gridPy(int j) { return (int)(mapY + (GRID_N - 1 - j) * cell + 0.5f); }

static void fillRect(int x0, int y0, int x1, int y1)
{
    int xs[4] = {x0, x1, x1, x0};
    int ys[4] = {y0, y0, y1, y1};
    scanFillPolygon(xs, ys, 4);
}

static void text(float x, float y, const char *s)
{
    glRasterPos2f(x, y);
    for (; *s; s++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s);
}

void draw2DPanel(int w, int h)
{
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);               // 1 unit = 1 pixel
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    canvasBegin(w, h);

    const float margin = 20.0f;
    float size = (w - 2 * margin < h - 2 * margin) ? (w - 2 * margin) : (h - 2 * margin);
    mapX = margin;
    mapY = h - margin - size;
    cell = size / (GRID_N - 1);

    if (testPattern)
    {
        drawTestPattern(w, h);
    }
    else
    {
        // ---- slope map: every cell is a scan-filled quad ----
        float L[3];
        getSunDir(L);
        for (int j = 0; j < GRID_N - 1; j++)
            for (int i = 0; i < GRID_N - 1; i++)
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
                fillRect(gridPx(i), gridPy(j + 1), gridPx(i + 1), gridPy(j));
            }

        // ---- grid every 5 cells (10 m), midpoint lines ----
        if (showGrid)
        {
            canvasColor(0.25f, 0.25f, 0.25f);
            for (int k = 0; k < GRID_N; k += 5)
            {
                midpointLine(gridPx(k), gridPy(GRID_N - 1), gridPx(k), gridPy(0));   // vertical
                midpointLine(gridPx(0), gridPy(k), gridPx(GRID_N - 1), gridPy(k));   // horizontal
            }
        }

        // ---- map border ----
        canvasColor(0, 0, 0);
        int bx0 = gridPx(0), bx1 = gridPx(GRID_N - 1);
        int by0 = gridPy(GRID_N - 1), by1 = gridPy(0);
        midpointLine(bx0, by0, bx1, by0);  midpointLine(bx1, by0, bx1, by1);
        midpointLine(bx1, by1, bx0, by1);  midpointLine(bx0, by1, bx0, by0);

        // ---- placeholder start / goal (Phase 6 replaces these with mouse picks) ----
        int sI = 4, sJ = 8, gI = 32, gJ = 9;
        int sx = gridPx(sI), sy = gridPy(sJ);
        int gx = gridPx(gI), gy = gridPy(gJ);

        canvasColor(0.1f, 0.1f, 0.1f);
        canvasBrush(2);
        midpointLine(sx, sy, gx, gy);                 // straight line start -> goal
        canvasBrush(1);

        canvasColor(0.1f, 0.35f, 0.9f);               // start: blue disc + black ring
        fillCircle(sx, sy, 6);
        canvasColor(0, 0, 0);
        midpointCircle(sx, sy, 7);

        canvasColor(0.7f, 0.1f, 0.8f);                // goal: purple disc + black ring
        fillCircle(gx, gy, 6);
        canvasColor(0, 0, 0);
        midpointCircle(gx, gy, 7);
    }

    // ---- legend swatches (scan-filled) ----
    int total = (GRID_N - 1) * (GRID_N - 1);
    const char *labels[3] = {
        "Accessible  (<= 1:20, 5%)",
        "Steep ramp  (<= 1:12, 8.3%)",
        "Not accessible (steeper)"
    };
    if (!testPattern)
        for (int k = 0; k < 3; k++)
        {
            int y = (int)(mapY - 56 - k * 22);
            float c[3];
            slopeClassColor(k, c);
            canvasColor(c[0], c[1], c[2]);
            fillRect((int)margin, y, (int)margin + 14, y + 14);
        }

    // ---- panel border and separator ----
    canvasColor(0.6f, 0.6f, 0.6f);
    midpointLine(1, 1, w - 2, 1);        midpointLine(w - 2, 1, w - 2, h - 2);
    midpointLine(w - 2, h - 2, 1, h - 2);  midpointLine(1, h - 2, 1, 1);

    canvasFlush();                        // the whole panel appears here

    // ---- text is drawn AFTER the flush so the bitmap fonts sit on top ----
    glColor3f(0, 0, 0);
    if (testPattern)
    {
        text(10, h - 18, "TEST PATTERN [K]: 24 midpoint lines, 3 circles, 2 fills");
        text(10, h - 34, "All lines/circles/fills = our own algorithms");
        return;
    }

    text(margin, mapY - 32, "Slope map (top-down, own raster algorithms)");
    for (int k = 0; k < 3; k++)
    {
        char buf[96];
        sprintf(buf, "%s : %.1f%%", labels[k], 100.0f * getClassCount(k) / total);
        text(margin + 22, mapY - 56 - k * 22 + 2, buf);
    }
    text(margin, mapY - 56 - 3 * 22 - 6,
         hillshade ? "Hillshade ON [M]   Grid 10 m [G]" : "Hillshade OFF [M]   Grid 10 m [G]");
    text(margin, mapY - 56 - 3 * 22 - 24, "Blue/purple = placeholder start/goal [K = test]");
}