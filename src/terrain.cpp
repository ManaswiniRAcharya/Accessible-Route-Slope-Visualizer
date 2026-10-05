#include <cmath>
#include <cstdio>
#include "common.h"

static float H[GRID_N][GRID_N];      // H[j][i] = height in metres
static float hMin = 0, hMax = 0;

// A smooth bump centred at (ci, cj) with the given peak height and width
static float bump(int i, int j, float ci, float cj, float sigma, float peak)
{
    float di = i - ci, dj = j - cj;
    return peak * expf(-(di * di + dj * dj) / (2.0f * sigma * sigma));
}

void generateTerrain()
{
    for (int j = 0; j < GRID_N; j++)
    {
        for (int i = 0; i < GRID_N; i++)
        {
            float h = 0.0f;

            // 1. Steep hill (not wheelchair-friendly): peak 6 m
            h += bump(i, j, 10, 28, 4.5f, 6.0f);

            // 2. Gentle mound (mostly accessible): peak 1 m, very wide
            h += bump(i, j, 31, 31, 6.0f, 1.0f);

            // 3. Raised platform, 1.5 m high (overrides whatever is underneath)
            if (i >= 28 && i <= 36 && j >= 5 && j <= 13)
                h = 1.5f;

            // 4. Ramp up to the platform: rises 1.5 m over 11 cells (22 m)
            //    = about 6.8 % grade, close to the 1:12 limit
            else if (i >= 17 && i <= 27 && j >= 8 && j <= 10)
                h = 1.5f * (i - 17) / 11.0f;

            // 5. Stairs from the platform's north edge: 0.25 m per step
            else if (i >= 30 && i <= 34 && j >= 14 && j <= 19)
                h = 1.5f - 0.25f * (j - 13);

            H[j][i] = h;
        }
    }

    hMin = hMax = H[0][0];
    for (int j = 0; j < GRID_N; j++)
        for (int i = 0; i < GRID_N; i++)
        {
            if (H[j][i] < hMin) hMin = H[j][i];
            if (H[j][i] > hMax) hMax = H[j][i];
        }

    printf("Terrain generated: %d x %d vertices, height %.2f m to %.2f m\n",
           GRID_N, GRID_N, hMin, hMax);
}

float getHeight(int i, int j)
{
    if (i < 0 || i >= GRID_N || j < 0 || j >= GRID_N) return 0.0f;
    return H[j][i];
}

float getMinHeight() { return hMin; }
float getMaxHeight() { return hMax; }