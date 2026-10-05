#include <GL/glut.h>
#include <cstring>
#include "common.h"

// ---------- Lab 1 midpoint line (still limited to 0 <= slope <= 1, x0 < x1) ----------
// Phase 4 generalises it. Call only between glBegin(GL_POINTS) / glEnd().
static void midpointLine(int x0, int y0, int x1, int y1)
{
    int dx = x1 - x0, dy = y1 - y0;
    int d = 2 * dy - dx;
    int incrE = 2 * dy;
    int incrNE = 2 * (dy - dx);
    int x = x0, y = y0;
    glVertex2i(x, y);
    while (x < x1)
    {
        if (d <= 0) { d += incrE; x++; }
        else        { d += incrNE; x++; y++; }
        glVertex2i(x, y);
    }
}

static void drawText(float x, float y, const char *s)
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

    // panel border
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(1, 1);      glVertex2i(w - 2, 1);
    glVertex2i(w - 2, h - 2);  glVertex2i(1, h - 2);
    glEnd();

    // ---- top-down height preview ----
    const float margin = 20.0f;
    float size = (w - 2 * margin < h - 2 * margin) ? (w - 2 * margin) : (h - 2 * margin);
    float mapX = margin;
    float mapY = h - margin - size;             // map sits at the top of the panel
    float cell = size / (GRID_N - 1);
    float range = getMaxHeight() - getMinHeight();

    // NOTE: on screen, +z points DOWN the map (this matches the default 3D view)
    glBegin(GL_QUADS);
    for (int j = 0; j < GRID_N - 1; j++)
        for (int i = 0; i < GRID_N - 1; i++)
        {
            float avg = (getHeight(i, j) + getHeight(i + 1, j) +
                         getHeight(i, j + 1) + getHeight(i + 1, j + 1)) * 0.25f;
            float t = (range > 0) ? (avg - getMinHeight()) / range : 0.0f;
            float g = 0.85f - 0.75f * t;        // low = light, high = dark
            glColor3f(g, g, g);

            float x0 = mapX + i * cell;
            float x1 = x0 + cell;
            float y1 = mapY + (GRID_N - 1 - j) * cell;      // top edge of the cell
            float y0 = y1 - cell;                           // bottom edge
            glVertex2f(x0, y0);  glVertex2f(x1, y0);
            glVertex2f(x1, y1);  glVertex2f(x0, y1);
        }
    glEnd();

    // separator drawn with OUR midpoint line (horizontal: slope 0)
    glPointSize(2.0f);
    glColor3f(0.9f, 0.1f, 0.1f);
    glBegin(GL_POINTS);
    midpointLine((int)margin, (int)(mapY - 12), (int)(w - margin), (int)(mapY - 12));
    glEnd();

    glColor3f(0.0f, 0.0f, 0.0f);
    drawText(margin, mapY - 32, "Height map (top-down)");
    drawText(margin, mapY - 50, "Light = low, dark = high");
}