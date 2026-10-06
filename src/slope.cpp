#include <cmath>
#include <cstdio>
#include "common.h"

#define NC (GRID_N - 1)                 // cells per side

static float slopeOf[NC][NC];           // rise/run per cell
static int   classOf[NC][NC];
static int   counts[3] = {0, 0, 0};

void computeSlopes()
{
    counts[0] = counts[1] = counts[2] = 0;

    for (int cj = 0; cj < NC; cj++)
        for (int ci = 0; ci < NC; ci++)
        {
            float h00 = getHeight(ci,     cj);
            float h10 = getHeight(ci + 1, cj);
            float h01 = getHeight(ci,     cj + 1);
            float h11 = getHeight(ci + 1, cj + 1);

            // average the two edge differences in each direction
            float dzdx = ((h10 - h00) + (h11 - h01)) / (2.0f * CELL_SIZE);
            float dzdz = ((h01 - h00) + (h11 - h10)) / (2.0f * CELL_SIZE);

            float s = sqrtf(dzdx * dzdx + dzdz * dzdz);
            slopeOf[cj][ci] = s;

            int c;
            if      (s <= SLOPE_EASY_LIMIT) c = SLOPE_GREEN;
            else if (s <= SLOPE_MAX_LIMIT)  c = SLOPE_YELLOW;
            else                            c = SLOPE_RED;
            classOf[cj][ci] = c;
            counts[c]++;
        }

    printf("Slope analysis: green %d, yellow %d, red %d cells (of %d)\n",
           counts[0], counts[1], counts[2], NC * NC);
}

float getCellSlope(int ci, int cj)
{
    if (ci < 0 || ci >= NC || cj < 0 || cj >= NC) return 0.0f;
    return slopeOf[cj][ci];
}

int getCellClass(int ci, int cj)
{
    if (ci < 0 || ci >= NC || cj < 0 || cj >= NC) return SLOPE_GREEN;
    return classOf[cj][ci];
}

int getClassCount(int cls) { return (cls >= 0 && cls <= 2) ? counts[cls] : 0; }

void slopeClassColor(int cls, float c[3])
{
    static const float table[3][3] = {
        {0.20f, 0.75f, 0.25f},      // green
        {0.95f, 0.85f, 0.15f},      // yellow
        {0.85f, 0.15f, 0.15f}       // red
    };
    for (int k = 0; k < 3; k++) c[k] = table[cls][k];
}

// ---- HSV -> RGB (colour model from the syllabus) ----
// h in degrees [0,360), s and v in [0,1]
static void hsvToRgb(float h, float s, float v, float c[3])
{
    h = fmodf(h, 360.0f);
    if (h < 0) h += 360.0f;
    float hh = h / 60.0f;
    int   i  = (int)hh;
    float f  = hh - i;
    float p = v * (1 - s);
    float q = v * (1 - s * f);
    float t = v * (1 - s * (1 - f));
    switch (i % 6)
    {
        case 0: c[0] = v; c[1] = t; c[2] = p; break;
        case 1: c[0] = q; c[1] = v; c[2] = p; break;
        case 2: c[0] = p; c[1] = v; c[2] = t; break;
        case 3: c[0] = p; c[1] = q; c[2] = v; break;
        case 4: c[0] = t; c[1] = p; c[2] = v; break;
        default: c[0] = v; c[1] = p; c[2] = q; break;
    }
}

// slope 0 -> hue 120 (green), slope >= 15 % -> hue 0 (red)
void slopeGradientColor(float slope, float c[3])
{
    float t = slope / 0.15f;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    hsvToRgb(120.0f * (1.0f - t), 0.85f, 0.90f, c);
}