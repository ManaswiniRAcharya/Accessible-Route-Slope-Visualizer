#include <GL/glut.h>
#include <cmath>
#include <algorithm>
#include "common.h"

#define NC            (GRID_N - 1)
#define MAXPATH       12000
#define BEZIER_STEPS  6
#define RAD2DEG       57.29577951f

static float gxs[MAXPATH], gzs[MAXPATH];     // smoothed path points (grid units)
static float cum[MAXPATH];                   // cumulative length in metres
static int   np = 0;

static Wheelchair wc;                        // zero-initialised: valid = false
static float baseSpeed = 5.0f;               // demo speed on flat ground, m/s

// ---------- height ON the drawn triangles (same diagonal as view3d.cpp) ----------
static float surfaceHeight(float gx, float gz)
{
    if (gx < 0) gx = 0;   if (gx > NC) gx = NC;
    if (gz < 0) gz = 0;   if (gz > NC) gz = NC;
    int i = (int)gx, j = (int)gz;
    if (i > NC - 1) i = NC - 1;
    if (j > NC - 1) j = NC - 1;
    float fx = gx - i, fz = gz - j;

    float h00 = getHeight(i, j),     h10 = getHeight(i + 1, j);
    float h01 = getHeight(i, j + 1), h11 = getHeight(i + 1, j + 1);

    if (fx + fz <= 1.0f)                       // triangle (i,j) (i+1,j) (i,j+1)
        return h00 + fx * (h10 - h00) + fz * (h01 - h00);
    return h11 + (1 - fx) * (h01 - h11) + (1 - fz) * (h10 - h11);   // the other triangle
}

// ---------- path building ----------
static void addPoint(float gx, float gz)
{
    if (np >= MAXPATH) return;
    if (np == 0) cum[0] = 0;
    else
    {
        float dx = (gx - gxs[np - 1]) * CELL_SIZE, dz = (gz - gzs[np - 1]) * CELL_SIZE;
        float d = sqrtf(dx * dx + dz * dz);
        if (d < 1e-4f) return;                 // skip duplicate points (curve joins)
        cum[np] = cum[np - 1] + d;
    }
    gxs[np] = gx;  gzs[np] = gz;  np++;
}

// Cell centres are the Bezier control points; midpoints between centres are the curve ends.
static void buildPath()
{
    np = 0;
    wc.valid = false; wc.playing = false; wc.atEnd = false;
    wc.dist = 0; wc.total = 0; wc.speed = baseSpeed;

    const Route &r = routeGet(ROUTE_ACCESSIBLE);
    if (!r.found || r.n < 2) return;

    addPoint(r.ci[0] + 0.5f, r.cj[0] + 0.5f);
    for (int k = 1; k + 1 < r.n; k++)
    {
        float cx = r.ci[k] + 0.5f, cz = r.cj[k] + 0.5f;                       // control point C
        float ax = 0.5f * (r.ci[k - 1] + r.ci[k]) + 0.5f;                     // A: midpoint back
        float az = 0.5f * (r.cj[k - 1] + r.cj[k]) + 0.5f;
        float bx = 0.5f * (r.ci[k] + r.ci[k + 1]) + 0.5f;                     // B: midpoint forward
        float bz = 0.5f * (r.cj[k] + r.cj[k + 1]) + 0.5f;

        for (int s = 0; s <= BEZIER_STEPS; s++)
        {
            float t = (float)s / BEZIER_STEPS, u = 1.0f - t;
            addPoint(u * u * ax + 2 * u * t * cx + t * t * bx,
                     u * u * az + 2 * u * t * cz + t * t * bz);
        }
    }
    addPoint(r.ci[r.n - 1] + 0.5f, r.cj[r.n - 1] + 0.5f);

    wc.total = (np >= 2) ? cum[np - 1] : 0.0f;
    wc.valid = (np >= 2 && wc.total > 0.0f);
}

// point at distance d metres along the path
static void posAt(float d, float *gx, float *gz)
{
    if (d <= 0)            { *gx = gxs[0];      *gz = gzs[0];      return; }
    if (d >= cum[np - 1])  { *gx = gxs[np - 1]; *gz = gzs[np - 1]; return; }
    int k = (int)(std::upper_bound(cum, cum + np, d) - cum);      // first point beyond d
    float seg = cum[k] - cum[k - 1];
    float t = (seg > 0) ? (d - cum[k - 1]) / seg : 0.0f;
    *gx = gxs[k - 1] + t * (gxs[k] - gxs[k - 1]);
    *gz = gzs[k - 1] + t * (gzs[k] - gzs[k - 1]);
}

// compute the chair pose for the current distance
static void place()
{
    if (!wc.valid) return;

    float a = std::max(0.0f, wc.dist - 1.0f), b = std::min(wc.total, wc.dist + 1.0f);
    float ax, az, bx, bz;
    posAt(a, &ax, &az);
    posAt(b, &bx, &bz);
    posAt(wc.dist, &wc.gx, &wc.gz);

    float dx = bx - ax, dz = bz - az;              // direction of travel (smooth: +-1 m)
    if (dx * dx + dz * dz > 1e-8f) wc.heading = atan2f(dx, dz) * RAD2DEG;

    wc.x = (wc.gx - (GRID_N - 1) * 0.5f) * CELL_SIZE;
    wc.z = (wc.gz - (GRID_N - 1) * 0.5f) * CELL_SIZE;
    wc.y = surfaceHeight(wc.gx, wc.gz);

    const float e = 0.5f;                          // central difference, half a cell each side
    wc.dhdx = (surfaceHeight(wc.gx + e, wc.gz) - surfaceHeight(wc.gx - e, wc.gz)) / (2 * e * CELL_SIZE);
    wc.dhdz = (surfaceHeight(wc.gx, wc.gz + e) - surfaceHeight(wc.gx, wc.gz - e)) / (2 * e * CELL_SIZE);

    float hr = wc.heading / RAD2DEG;
    wc.slopeAlong = wc.dhdx * sinf(hr) + wc.dhdz * cosf(hr);
    wc.slopeAbs   = sqrtf(wc.dhdx * wc.dhdx + wc.dhdz * wc.dhdz);

    float f = 1.0f - 5.0f * wc.slopeAlong;         // uphill slows, downhill speeds up
    if (f < 0.4f) f = 0.4f;
    if (f > 1.3f) f = 1.3f;
    wc.speed = baseSpeed * f;
}

// ---------- public interface ----------
void wcUpdate()
{
    static int lastVer = -1, lastT = 0;

    int ver = routeVersion();
    if (ver != lastVer)                 // the route changed: rebuild and stop at the start
    {
        lastVer = ver;
        buildPath();
        place();
    }

    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastT) / 1000.0f;
    lastT = now;
    if (dt < 0) dt = 0;
    if (dt > 0.05f) dt = 0.05f;          // never jump after a pause

    if (wc.valid && wc.playing)
    {
        wc.dist += wc.speed * dt;
        if (wc.dist >= wc.total) { wc.dist = wc.total; wc.playing = false; wc.atEnd = true; }
        place();
    }
}

void wcRestart()
{
    if (!wc.valid) return;
    wc.dist = 0; wc.atEnd = false; wc.playing = true;
    place();
}

void wcTogglePlay()
{
    if (!wc.valid) return;
    if (wc.atEnd) { wcRestart(); return; }
    wc.playing = !wc.playing;
}

void wcSpeedScale(float factor)
{
    baseSpeed *= factor;
    if (baseSpeed < 1.0f)  baseSpeed = 1.0f;
    if (baseSpeed > 30.0f) baseSpeed = 30.0f;
}

const Wheelchair &wcGet() { return wc; }