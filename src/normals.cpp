#include <cmath>
#include "common.h"

static void normalize3(float n[3])
{
    float len = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (len > 0.0f) { n[0] /= len; n[1] /= len; n[2] /= len; }
}

// Smooth normal at a grid vertex using central differences
// (one-sided at the border, so the index stays inside the grid).
void getVertexNormal(int i, int j, float vscale, float n[3])
{
    int i0 = (i > 0) ? i - 1 : i;
    int i1 = (i < GRID_N - 1) ? i + 1 : i;
    int j0 = (j > 0) ? j - 1 : j;
    int j1 = (j < GRID_N - 1) ? j + 1 : j;

    float dhdx = (getHeight(i1, j) - getHeight(i0, j)) / ((i1 - i0) * CELL_SIZE);
    float dhdz = (getHeight(i, j1) - getHeight(i, j0)) / ((j1 - j0) * CELL_SIZE);

    n[0] = -dhdx * vscale;
    n[1] = 1.0f;
    n[2] = -dhdz * vscale;
    normalize3(n);
}

// Normal of a whole cell, from the same gradient used for slope in Phase 2
void getCellNormal(int ci, int cj, float n[3])
{
    float h00 = getHeight(ci,     cj);
    float h10 = getHeight(ci + 1, cj);
    float h01 = getHeight(ci,     cj + 1);
    float h11 = getHeight(ci + 1, cj + 1);

    float dzdx = ((h10 - h00) + (h11 - h01)) / (2.0f * CELL_SIZE);
    float dzdz = ((h01 - h00) + (h11 - h10)) / (2.0f * CELL_SIZE);

    n[0] = -dzdx;
    n[1] = 1.0f;
    n[2] = -dzdz;
    normalize3(n);
}

// Lambert's cosine law: both vectors must be unit length
float lambert(const float n[3], const float L[3])
{
    float d = n[0] * L[0] + n[1] * L[1] + n[2] * L[2];
    return (d > 0.0f) ? d : 0.0f;
}