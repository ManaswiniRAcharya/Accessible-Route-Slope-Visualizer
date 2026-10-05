#include <GL/glut.h>
#include <cmath>
#include "common.h"

static bool  depthOn   = true;
static bool  wireOn    = true;
static bool  exaggerate = false;

static float camYaw   = 0.0f;     // degrees, around the vertical axis
static float camPitch = 40.0f;    // degrees above the ground
static float camDist  = 110.0f;   // metres from the centre

void update3D() { /* animation arrives in a later phase */ }

void toggleDepthTest()    { depthOn = !depthOn; }
void toggleWireframe()    { wireOn = !wireOn; }
void toggleExaggeration() { exaggerate = !exaggerate; }

void camRotate(float dYaw, float dPitch)
{
    camYaw += dYaw;
    camPitch += dPitch;
    if (camPitch < 5.0f)  camPitch = 5.0f;     // stay above the ground
    if (camPitch > 89.0f) camPitch = 89.0f;    // avoid looking exactly straight down
}

void camDrag(int dx, int dy) { camRotate(-dx * 0.4f, dy * 0.4f); }

void camZoom(float factor)
{
    camDist *= factor;
    if (camDist < 30.0f)  camDist = 30.0f;
    if (camDist > 250.0f) camDist = 250.0f;
}

// ---------- helpers ----------
static float lerp(float a, float b, float t) { return a + (b - a) * t; }

// height -> colour: green (low) -> brown (middle) -> near-white (high)
static void heightColor(float h, float c[3])
{
    float range = getMaxHeight() - getMinHeight();
    float t = (range > 0.0f) ? (h - getMinHeight()) / range : 0.0f;
    if (t < 0) t = 0;
    if (t > 1) t = 1;

    const float green[3] = {0.35f, 0.65f, 0.30f};
    const float brown[3] = {0.55f, 0.42f, 0.28f};
    const float white[3] = {0.95f, 0.95f, 0.95f};

    if (t < 0.5f)
        for (int k = 0; k < 3; k++) c[k] = lerp(green[k], brown[k], t * 2.0f);
    else
        for (int k = 0; k < 3; k++) c[k] = lerp(brown[k], white[k], (t - 0.5f) * 2.0f);
}

// emit one vertex of the mesh
static void vertexAt(int i, int j, bool useColor)
{
    float h = getHeight(i, j);
    if (useColor)
    {
        float c[3];
        heightColor(h, c);
        glColor3fv(c);
    }
    float scale = exaggerate ? 3.0f : 1.0f;    // drawing only!
    glVertex3f(gridToWorldX(i), h * scale, gridToWorldZ(j));
}

// Draw the whole terrain as triangles: two per grid cell
static void drawMesh(bool useColor)
{
    glBegin(GL_TRIANGLES);
    for (int j = 0; j < GRID_N - 1; j++)
        for (int i = 0; i < GRID_N - 1; i++)
        {
            vertexAt(i,     j,     useColor);
            vertexAt(i,     j + 1, useColor);
            vertexAt(i + 1, j,     useColor);

            vertexAt(i + 1, j,     useColor);
            vertexAt(i,     j + 1, useColor);
            vertexAt(i + 1, j + 1, useColor);
        }
    glEnd();
}

void draw3DScene(int w, int h)
{
    if (depthOn) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);

    // projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / h, 1.0, 500.0);

    // camera on a sphere around the terrain centre
    const double D2R = 3.14159265358979 / 180.0;
    float yaw = camYaw * D2R, pitch = camPitch * D2R;
    float ex = camDist * cos(pitch) * sin(yaw);
    float ey = camDist * sin(pitch);
    float ez = camDist * cos(pitch) * cos(yaw);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(ex, ey, ez,   0, 0, 0,   0, 1, 0);

    // filled, coloured mesh (pushed back slightly so the wireframe wins)
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    drawMesh(true);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // wireframe overlay
    if (wireOn)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glColor3f(0.1f, 0.1f, 0.1f);
        drawMesh(false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}