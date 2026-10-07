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

static double mvM[16], prM[16];      // matrices captured each frame, used for picking
static GLint  vpM[4];

static const double D2R = 3.14159265358979 / 180.0;

static bool  followOn  = false;     // NEW (Phase 7)
static float followYaw = 0.0f;

// ---------- toggles and controls ----------
void update3D()
{
    if (sunAnim) { sunAz += 0.5f; if (sunAz > 360.0f) sunAz -= 360.0f; }

    const Wheelchair &wc = wcGet();
    if (followOn && wc.valid)                       // glide the camera round behind the chair
    {
        float d = (wc.heading + 180.0f) - followYaw;
        while (d >  180.0f) d -= 360.0f;
        while (d < -180.0f) d += 360.0f;
        followYaw += d * 0.08f;
    }
}

void toggleFollowCam()
{
    followOn = !followOn;
    if (followOn) followYaw = wcGet().heading + 180.0f;   // start directly behind
}


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

    sprintf(buf, "Routes: [1] black=shortest %s   [2] blue=accessible %s   click = START then GOAL   [E] clear",
        routeShown(ROUTE_SHORTEST) ? "ON" : "OFF", routeShown(ROUTE_ACCESSIBLE) ? "ON" : "OFF");
    text(10, h - 56, buf);

    sprintf(buf, "Wheelchair: [Space] play/pause  [B] restart  [U/J] speed  [O] follow camera: %s",
        followOn ? "ON" : "OFF");
    text(10, h - 74, buf);

    sprintf(buf, "Sun azimuth %.0f  elevation %.0f   [, .] [ [ ] ] rotate sun   [N] animate   [T] top view   [R] reset",
            sunAz, sunEl);
    text(10, 10, buf);
}

// ---------- picking: mouse -> ray -> terrain ----------
// terrain height under a grid-space point (bilinear), including the drawing exaggeration
static float terrainY(float gx, float gz)
{
    int i = (int)gx, j = (int)gz;
    float fx = gx - i, fz = gz - j;
    float h = (1 - fx) * (1 - fz) * getHeight(i,     j)     + fx * (1 - fz) * getHeight(i + 1, j)
            + (1 - fx) * fz       * getHeight(i,     j + 1) + fx * fz       * getHeight(i + 1, j + 1);
    return h * vscale();
}

// mx, my: window coordinates (origin top-left). Returns the terrain cell under the cursor.
bool pick3D(int mx, int my, int *ci, int *cj)
{
    double x0, y0, z0, x1, y1, z1;
    double wy = vpM[3] - my;                         // OpenGL's y points up, the mouse's points down
    if (!gluUnProject(mx, wy, 0.0, mvM, prM, vpM, &x0, &y0, &z0)) return false;   // near plane
    if (!gluUnProject(mx, wy, 1.0, mvM, prM, vpM, &x1, &y1, &z1)) return false;   // far plane

    double dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
    double len = sqrt(dx * dx + dy * dy + dz * dz);
    int steps = (int)(len / 0.25);                   // march in 0.25 m steps
    if (steps < 1) return false;

    for (int k = 0; k <= steps; k++)
    {
        double t = (double)k / steps;
        double x = x0 + t * dx, y = y0 + t * dy, z = z0 + t * dz;
        float gx = (float)(x / CELL_SIZE + (GRID_N - 1) * 0.5);
        float gz = (float)(z / CELL_SIZE + (GRID_N - 1) * 0.5);
        if (gx < 0 || gz < 0 || gx >= GRID_N - 1 || gz >= GRID_N - 1) continue;   // above the map
        if (y <= terrainY(gx, gz)) { *ci = (int)gx; *cj = (int)gz; return true; }
    }
    return false;                                    // ray missed the terrain
}

// ---------- route drawing ----------
static void centre(int ci, int cj, float lift, float *x, float *y, float *z)
{
    // a cell centre lies on the triangle diagonal, so its height is the diagonal's average
    *x = gridToWorldX(ci) + CELL_SIZE * 0.5f;
    *z = gridToWorldZ(cj) + CELL_SIZE * 0.5f;
    *y = 0.5f * (getHeight(ci + 1, cj) + getHeight(ci, cj + 1)) * vscale() + lift;
}

static void drawPolyline3D(const Route &r, float R, float G, float B, float width)
{
    if (!r.found || r.n < 2) return;
    glLineWidth(width);
    glColor3f(R, G, B);
    glBegin(GL_LINE_STRIP);
    for (int k = 0; k < r.n; k++)
    {
        float x, y, z;
        centre(r.ci[k], r.cj[k], 0.45f, &x, &y, &z);     // lifted so it is not buried
        glVertex3f(x, y, z);
    }
    glEnd();
    glLineWidth(1.0f);
}

static void drawMarker3D(int ci, int cj, float R, float G, float B)
{
    float x, y, z;
    centre(ci, cj, 0.0f, &x, &y, &z);
    glLineWidth(3.0f);
    glColor3f(0, 0, 0);
    glBegin(GL_LINES);                                   // pole
    glVertex3f(x, y, z);  glVertex3f(x, y + 7.0f, z);
    glEnd();
    glLineWidth(1.0f);
    glColor3f(R, G, B);
    glPushMatrix();                                      // translation, as in Lab 7
    glTranslatef(x, y + 7.8f, z);
    glutSolidSphere(1.3, 16, 12);
    glPopMatrix();
}

static void drawRoute3D()
{
    if (routeShown(ROUTE_SHORTEST))   drawPolyline3D(routeGet(ROUTE_SHORTEST),   0.05f, 0.05f, 0.05f, 3.0f);
    if (routeShown(ROUTE_ACCESSIBLE)) drawPolyline3D(routeGet(ROUTE_ACCESSIBLE), 0.10f, 0.25f, 1.00f, 5.0f);

    int ci, cj;
    if (routeHasStart()) { routeStart(&ci, &cj); drawMarker3D(ci, cj, 1.0f, 1.0f, 1.0f); }
    if (routeHasGoal())  { routeGoal(&ci, &cj);  drawMarker3D(ci, cj, 0.7f, 0.1f, 0.8f); }
}

// ---------- wheelchair ----------
#define CHAIR_SCALE 3.0f                  // model is in metres; x3 so it is visible from far away

static void box(float x, float y, float z, float sx, float sy, float sz)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    glutSolidCube(1.0);
    glPopMatrix();
}

// local frame: origin on the ground between the wheels, +y up, +z forward, +x right
static void drawChairModel(float spinDeg)
{
    glColor3f(0.95f, 0.55f, 0.10f);                         // seat and backrest
    box(0, 0.50f, 0.00f,   0.46f, 0.07f, 0.46f);
    box(0, 0.82f, -0.22f,  0.46f, 0.55f, 0.05f);

    glColor3f(0.25f, 0.25f, 0.28f);                         // frame
    box(-0.24f, 0.62f, 0.0f,  0.04f, 0.04f, 0.40f);         // armrests
    box( 0.24f, 0.62f, 0.0f,  0.04f, 0.04f, 0.40f);
    box(0, 0.305f, -0.10f,    0.68f, 0.03f, 0.03f);         // rear axle
    box(-0.20f, 1.10f, -0.26f, 0.04f, 0.04f, 0.14f);        // push handles
    box( 0.20f, 1.10f, -0.26f, 0.04f, 0.04f, 0.14f);
    box(-0.20f, 0.34f, 0.30f,  0.03f, 0.32f, 0.03f);        // footrest posts
    box( 0.20f, 0.34f, 0.30f,  0.03f, 0.32f, 0.03f);
    box(0, 0.18f, 0.40f,      0.34f, 0.03f, 0.14f);         // footrest

    glPushMatrix();                                         // front casters
    glTranslatef(-0.22f, 0.07f, 0.38f);  glutSolidSphere(0.07, 10, 8);
    glPopMatrix();
    glPushMatrix();
    glTranslatef( 0.22f, 0.07f, 0.38f);  glutSolidSphere(0.07, 10, 8);
    glPopMatrix();

    for (int s = -1; s <= 1; s += 2)                        // big rear wheels
    {
        glPushMatrix();
        glTranslatef(s * 0.34f, 0.305f, -0.10f);
        glRotatef(spinDeg, 1, 0, 0);                        // rolling: spin about the axle
        glColor3f(0.75f, 0.75f, 0.78f);
        box(0, 0, 0,  0.02f, 0.56f, 0.02f);                 // two spokes (visible spin)
        box(0, 0, 0,  0.02f, 0.02f, 0.56f);
        glRotatef(90, 0, 1, 0);                             // torus axis z -> x
        glColor3f(0.05f, 0.05f, 0.05f);
        glutSolidTorus(0.025, 0.28, 8, 24);                 // tyre
        glPopMatrix();
    }
}

static void drawWheelchair3D()
{
    const Wheelchair &c = wcGet();
    if (!c.valid) return;
    float vs = vscale();

    // frame axes: up = terrain normal, forward = heading projected onto the slope, right = up x forward
    float up[3] = { -c.dhdx * vs, 1.0f, -c.dhdz * vs };
    float ul = sqrtf(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);
    for (int k = 0; k < 3; k++) up[k] /= ul;

    float hr = c.heading * (float)D2R;
    float f0[3] = { sinf(hr), 0.0f, cosf(hr) };
    float dp = f0[0] * up[0] + f0[1] * up[1] + f0[2] * up[2];
    float fw[3] = { f0[0] - dp * up[0], f0[1] - dp * up[1], f0[2] - dp * up[2] };
    float fl = sqrtf(fw[0] * fw[0] + fw[1] * fw[1] + fw[2] * fw[2]);
    for (int k = 0; k < 3; k++) fw[k] /= fl;

    float rt[3] = { up[1] * fw[2] - up[2] * fw[1],
                    up[2] * fw[0] - up[0] * fw[2],
                    up[0] * fw[1] - up[1] * fw[0] };

    GLfloat m[16] = {                    // column-major, like Lab 7's m[16]
        rt[0], rt[1], rt[2], 0,          // X axis (right)
        up[0], up[1], up[2], 0,          // Y axis (up)
        fw[0], fw[1], fw[2], 0,          // Z axis (forward)
        c.x,   c.y * vs, c.z, 1 };       // position on the (possibly exaggerated) surface

    float spin = c.dist / (CHAIR_SCALE * 0.305f) * 57.29578f;    // angle = distance / radius

    if (lightOn) setupLighting();        // modelview is the camera only, so the sun stays put
    glPushMatrix();
    glMultMatrixf(m);
    glScalef(CHAIR_SCALE, CHAIR_SCALE, CHAIR_SCALE);
    drawChairModel(spin);
    glPopMatrix();
    disableLighting();
}

// ---------- main 3D draw ----------
void draw3DScene(int w, int h)
{
    if (depthOn) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);

    // camera parameters: free orbit around the centre, or follow the wheelchair
    const Wheelchair &wc = wcGet();
    bool following = followOn && wc.valid;
    double dist = camDist, tx = 0, ty = 0, tz = 0;
    double yawDeg = camYaw, pitchDeg = camPitch;
    if (following)
    {
        dist = camDist * 0.25;  if (dist < 8.0) dist = 8.0;     // wheel zoom still works
        tx = wc.x;  ty = wc.y * vscale() + 2.0;  tz = wc.z;
        yawDeg = followYaw;  pitchDeg = 25.0;
    }

    // projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = (double)w / h;
    if (orthoOn)
    {
        double hh = dist * 0.414;
        glOrtho(-hh * aspect, hh * aspect, -hh, hh, 1.0, 500.0);
    }
    else
        gluPerspective(45.0, aspect, 1.0, 500.0);

    // camera
    double yaw = yawDeg * D2R, pitch = pitchDeg * D2R;
    double ex = tx + dist * cos(pitch) * sin(yaw);
    double ey = ty + dist * sin(pitch);
    double ez = tz + dist * cos(pitch) * cos(yaw);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(ex, ey, ez,   tx, ty, tz,   0, 1, 0);

    glGetDoublev(GL_MODELVIEW_MATRIX,  mvM);      // remembered for picking
    glGetDoublev(GL_PROJECTION_MATRIX, prM);
    glGetIntegerv(GL_VIEWPORT,         vpM);

    if (lightOn) setupLighting();
    glShadeModel(smoothOn ? GL_SMOOTH : GL_FLAT);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    drawMesh(colorMode, lightOn);
    glDisable(GL_POLYGON_OFFSET_FILL);

    disableLighting();

    if (wireOn)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glColor3f(0.1f, 0.1f, 0.1f);
        drawMesh(-1, false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    drawRoute3D();
    drawWheelchair3D();                           // NEW (Phase 7)
    drawHUD(w, h);
}