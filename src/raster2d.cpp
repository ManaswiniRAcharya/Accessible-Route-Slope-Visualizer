#include <GL/glut.h>
#include "common.h"

// One "pixel" of our software raster. Must be called between glBegin/glEnd.
static void plot(int x, int y) { glVertex2i(x, y); }

// Midpoint line algorithm (Lab 1). Works for 0 <= slope <= 1 and x0 < x1.
// Phase 4 will generalise it to all octants.
static void midpointLine(int x0, int y0, int x1, int y1)
{
    int dx = x1 - x0, dy = y1 - y0;
    int d = 2 * dy - dx;
    int incrE = 2 * dy;
    int incrNE = 2 * (dy - dx);
    int x = x0, y = y0;
    plot(x, y);
    while (x < x1) {
        if (d <= 0) { d += incrE; x++; }
        else        { d += incrNE; x++; y++; }
        plot(x, y);
    }
}

void draw2DPanel(int w, int h)
{
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);            // 1 unit = 1 pixel
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // light border so you can see the panel edges
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(1, 1);  glVertex2i(w - 2, 1);
    glVertex2i(w - 2, h - 2);  glVertex2i(1, h - 2);
    glEnd();

    // midpoint line drawn pixel by pixel
    glPointSize(3.0f);
    glColor3f(0.9f, 0.1f, 0.1f);
    glBegin(GL_POINTS);
    midpointLine(20, 50, w - 20, 50 + (w - 40) * 6 / 10);
    glEnd();
}