#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include "common.h"

static bool  depthOn    = true;
static bool  wireOn     = true;
static bool  exaggerate = false;
static int   colorMode  = 1;        // 0 height, 1 slope classes, 2 slope gradient

static bool  lightOn  = true;
static bool  smoothOn = true;       // Gouraud (true) or flat (false)
static bool  specOn   = true;
static bool  orthoOn  = false;
static bool  sunAnim  = false;

static float sunAz = 45.0f;         // degrees around the vertical axis
static float sunEl = 40.0f;         // degrees above the horizon

static float camYaw   = 0.0f;
static float camPitch = 40.0f;
static float camDist  = 110.0f;

static const double D2R = 3.14159265358979 / 180.0;

// ---------- toggles and controls ----------
void update3D() { if (sunAnim) { sunAz += 0.5f; if (sunAz > 360.0f) sunAz -= 360.0f; } }

void toggleDepthTest()    { depthOn = !depthOn; }
void toggleWireframe()    { wireOn = !wireOn; }
void toggleExaggeration() { exaggerate = !exaggerate; }
void cycleColorMode()     { colorMode = (colorMode + 1) % 3; }
void toggleLighting()     { lightOn = !lightOn; }
void toggleShadeModel()   { smoothOn = !smoothOn; }
void toggleSpecular()     { specOn = !specOn; }
void toggleProjection()   { orthoOn = !orthoOn; }
void toggleSunAnim()      { sunAnim = !sunAnim; }

void sunRotate(float dAz, float dEl)
{
    sunAz += dAz;
    sunEl += dEl;
    if (sunEl < 5.0f)  sunEl = 5.0f;
    if (sunEl > 90.0f) sunEl = 90.0f;
}

void getSunDir(float L[3])
{
    float az = sunAz * D2R, el = sunEl * D2R;
    L[0] = cosf(el) * sinf(az);
    L[1] = sinf(el);
    L[2] = cosf(el) * cosf(az);
}

void camRotate(float dYaw, float dPitch)
{
    camYaw += dYaw;
    camPitch += dPitch;
    if (camPitch < 5.0f)  camPitch = 5.0f;
    if (camPitch > 89.0f) camPitch = 89.0f;
}
void camDrag(int dx, int dy) { camRotate(-dx * 0.4f, dy * 0.4f); }
void camZoom(float factor)
{
    camDist *= factor;
    if (camDist < 30.0f)  camDist = 30.0f;
    if (camDist > 250.0f) camDist = 250.0f;
}
void camTopView() { camYaw = 0.0f; camPitch = 89.0f; }
void camReset()   { camYaw = 0.0f; camPitch = 40.0f; camDist = 110.0f; }

// ---------- colours ----------
static float lerp(float a, float b, float t) { return a + (b - a) * t; }

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

// ---------- mesh ----------
static float vscale() { return exaggerate ? 3.0f : 1.0f; }

// one mesh vertex: optional height colour, optional smooth normal
static void emit(int i, int j, bool heightCol, bool smoothNormal)
{
    float h = getHeight(i, j);
    if (heightCol)
    {
        float c[3];
        heightColor(h, c);
        glColor3fv(c);
    }
    if (smoothNormal)
    {
        float n[3];
        getVertexNormal(i, j, vscale(), n);
        glNormal3fv(n);
    }
    glVertex3f(gridToWorldX(i), h * vscale(), gridToWorldZ(j));
}

// true geometric normal of one triangle (cross product of two edges), sent before its vertices
static void triangleNormal(int i0, int j0, int i1, int j1, int i2, int j2)
{
    float p[3][3] = {
        {gridToWorldX(i0), getHeight(i0, j0) * vscale(), gridToWorldZ(j0)},
        {gridToWorldX(i1), getHeight(i1, j1) * vscale(), gridToWorldZ(j1)},
        {gridToWorldX(i2), getHeight(i2, j2) * vscale(), gridToWorldZ(j2)}};
    float e1[3], e2[3], n[3];
    for (int k = 0; k < 3; k++) { e1[k] = p[1][k] - p[0][k]; e2[k] = p[2][k] - p[0][k]; }
    n[0] = e1[1] * e2[2] - e1[2] * e2[1];
    n[1] = e1[2] * e2[0] - e1[0] * e2[2];
    n[2] = e1[0] * e2[1] - e1[1] * e2[0];
    float len = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (len > 0.0f) { n[0] /= len; n[1] /= len; n[2] /= len; }
    glNormal3fv(n);
}

// mode: 0 height, 1 slope classes, 2 slope gradient, -1 no colour (wireframe)
// withNormals: send normals (needed only when lighting is on)
static void drawMesh(int mode, bool withNormals)
{
    bool heightCol = (mode == 0);
    bool smoothN   = withNormals && smoothOn;
    bool flatN     = withNormals && !smoothOn;

    glBegin(GL_TRIANGLES);
    for (int j = 0; j < GRID_N - 1; j++)
        for (int i = 0; i < GRID_N - 1; i++)
        {
            float c[3];
            if (mode == 1)      { slopeClassColor(getCellClass(i, j), c); glColor3fv(c); }
            else if (mode == 2) { slopeGradientColor(getCellSlope(i, j), c); glColor3fv(c); }

            if (flatN) triangleNormal(i, j, i, j + 1, i + 1, j);
            emit(i,     j,     heightCol, smoothN);
            emit(i,     j + 1, heightCol, smoothN);
            emit(i + 1, j,     heightCol, smoothN);

            if (flatN) triangleNormal(i + 1, j, i, j + 1, i + 1, j + 1);
            emit(i + 1, j,     heightCol, smoothN);
            emit(i,     j + 1, heightCol, smoothN);
            emit(i + 1, j + 1, heightCol, smoothN);
        }
    glEnd();
}

// ---------- lighting ----------
static void setupLighting()
{
    GLfloat modelAmbient[] = {0.25f, 0.25f, 0.25f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, modelAmbient);

    float L[3];
    getSunDir(L);
    GLfloat pos[]  = {L[0], L[1], L[2], 0.0f};          // w = 0 -> directional
    GLfloat amb[]  = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat dif[]  = {0.90f, 0.90f, 0.85f, 1.0f};
    GLfloat spc[]  = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, pos);              // AFTER gluLookAt: sun stays in the world
    glLightfv(GL_LIGHT0, GL_AMBIENT,  amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spc);

    GLfloat matSpec[] = {specOn ? 0.35f : 0.0f, specOn ? 0.35f : 0.0f, specOn ? 0.35f : 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 30.0f);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);   // glColor now sets the material
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
}

static void disableLighting()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glShadeModel(GL_SMOOTH);
}

// ---------- HUD ----------
static void text(float x, float y, const char *s)
{
    glRasterPos2f(x, y);
    for (; *s; s++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s);
}

static void drawHUD(int w, int h)
{
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const char *modes[3] = {"HEIGHT", "SLOPE CLASSES (green <=5%, yellow <=8.3%, red steeper)", "SLOPE GRADIENT (HSV)"};
    char buf[200];
    glColor3f(0, 0, 0);
    sprintf(buf, "Colour: %s   [S]", modes[colorMode]);
    text(10, h - 20, buf);

    sprintf(buf, "Light:%s [L]  Shading:%s [F]  Specular:%s [X]  Projection:%s [P]  Depth:%s [D]",
            lightOn ? "ON" : "OFF", smoothOn ? "SMOOTH(Gouraud)" : "FLAT",
            specOn ? "ON" : "OFF", orthoOn ? "ORTHOGRAPHIC" : "PERSPECTIVE", depthOn ? "ON" : "OFF");
    text(10, h - 38, buf);

    sprintf(buf, "Sun azimuth %.0f  elevation %.0f   [, .] [ [ ] ] rotate sun   [N] animate   [T] top view   [R] reset",
            sunAz, sunEl);
    text(10, 10, buf);
}

// ---------- main 3D draw ----------
void draw3DScene(int w, int h)
{
    if (depthOn) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);

    // projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = (double)w / h;
    if (orthoOn)
    {
        double hh = camDist * 0.414;             // same apparent size at the terrain centre
        glOrtho(-hh * aspect, hh * aspect, -hh, hh, 1.0, 500.0);
    }
    else
        gluPerspective(45.0, aspect, 1.0, 500.0);

    // camera
    float yaw = camYaw * D2R, pitch = camPitch * D2R;
    float ex = camDist * cos(pitch) * sin(yaw);
    float ey = camDist * sin(pitch);
    float ez = camDist * cos(pitch) * cos(yaw);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(ex, ey, ez,   0, 0, 0,   0, 1, 0);

    // lighting must be configured after gluLookAt
    if (lightOn) setupLighting();
    glShadeModel(smoothOn ? GL_SMOOTH : GL_FLAT);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    drawMesh(colorMode, lightOn);
    glDisable(GL_POLYGON_OFFSET_FILL);

    disableLighting();                           // the wireframe, HUD and 2D panel are unlit

    if (wireOn)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glColor3f(0.1f, 0.1f, 0.1f);
        drawMesh(-1, false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    drawHUD(w, h);
}