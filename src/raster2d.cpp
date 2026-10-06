#include <GL/glut.h>
#include <cstdio>
#include "common.h"

static bool hillshade = true;
void toggleHillshade() { hillshade = !hillshade; }

// ---------- Lab 1 midpoint line (still limited to 0 <= slope <= 1, x0 < x1) ----------
// Call only between glBegin(GL_POINTS) / glEnd(). Generalised in Phase 4.
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
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // panel border
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(1, 1);      glVertex2i(w - 2, 1);
    glVertex2i(w - 2, h - 2);  glVertex2i(1, h - 2);
    glEnd();

    // ---- top-down slope map (one quad per cell; replaced by our scan-fill in Phase 4) ----
    const float margin = 20.0f;
    float size = (w - 2 * margin < h - 2 * margin) ? (w - 2 * margin) : (h - 2 * margin);
    float mapX = margin;
    float mapY = h - margin - size;
    float cell = size / (GRID_N - 1);

    glBegin(GL_QUADS);
    for (int j = 0; j < GRID_N - 1; j++)
        for (int i = 0; i < GRID_N - 1; i++)
        {
            float c[3];
            slopeClassColor(getCellClass(i, j), c);
            glColor3fv(c);

            float x0 = mapX + i * cell;
            float x1 = x0 + cell;
            float y1 = mapY + (GRID_N - 1 - j) * cell;
            float y0 = y1 - cell;
            glVertex2f(x0, y0);  glVertex2f(x1, y0);
            glVertex2f(x1, y1);  glVertex2f(x0, y1);
        }
    glEnd();

    // separator drawn with OUR midpoint line
    glPointSize(2.0f);
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_POINTS);
    midpointLine((int)margin, (int)(mapY - 12), (int)(w - margin), (int)(mapY - 12));
    glEnd();

    // ---- title, legend, statistics ----
    glColor3f(0, 0, 0);
    drawText(margin, mapY - 32, "Slope map (top-down)");

    const char *labels[3] = {
        "Accessible  (<= 1:20, 5%)",
        "Steep ramp  (<= 1:12, 8.3%)",
        "Not accessible (steeper)"
    };
    int total = (GRID_N - 1) * (GRID_N - 1);
    for (int k = 0; k < 3; k++)
    {
        float y = mapY - 56 - k * 22;
        float c[3];
        slopeClassColor(k, c);
        glColor3fv(c);
        glBegin(GL_QUADS);                    // legend swatch
        glVertex2f(margin, y);       glVertex2f(margin + 14, y);
        glVertex2f(margin + 14, y + 14);  glVertex2f(margin, y + 14);
        glEnd();

        char buf[96];
        sprintf(buf, "%s : %.1f%%", labels[k], 100.0f * getClassCount(k) / total);
        glColor3f(0, 0, 0);
        drawText(margin + 22, y + 2, buf);
    }
}