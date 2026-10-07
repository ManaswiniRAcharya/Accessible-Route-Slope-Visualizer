#include <cmath>
#include <cstdio>
#include "common.h"

#define NC (GRID_N - 1)
#define NN (NC * NC)
#define YELLOW_COST 3.0f          // a steep-but-legal cell costs 3x a flat one

// cost multiplier of entering a cell, or -1 if the cell is blocked
static float cellCost(int mode, int ci, int cj)
{
    if (ci < 0 || ci >= NC || cj < 0 || cj >= NC) return -1.0f;
    if (mode == ROUTE_SHORTEST) return 1.0f;               // slope is ignored
    int c = getCellClass(ci, cj);
    if (c == SLOPE_RED) return -1.0f;
    return (c == SLOPE_YELLOW) ? YELLOW_COST : 1.0f;
}

static float heuristic(int ci, int cj, int gi, int gj)     // straight-line distance in cells
{
    float dx = (float)(ci - gi), dy = (float)(cj - gj);
    return sqrtf(dx * dx + dy * dy);
}

static void measure(Route &r)
{
    r.length = 0; r.maxSlope = 0; r.yellowCells = 0; r.redCells = 0;
    for (int k = 0; k < r.n; k++)
    {
        int c = getCellClass(r.ci[k], r.cj[k]);
        if (c == SLOPE_YELLOW) r.yellowCells++;
        if (c == SLOPE_RED)    r.redCells++;
        float s = getCellSlope(r.ci[k], r.cj[k]);
        if (s > r.maxSlope) r.maxSlope = s;
        if (k > 0)
        {
            float dx = (float)(r.ci[k] - r.ci[k - 1]), dy = (float)(r.cj[k] - r.cj[k - 1]);
            r.length += sqrtf(dx * dx + dy * dy) * CELL_SIZE;
        }
    }
}

bool findRoute(int mode, int si, int sj, int gi, int gj, Route &out)
{
    out.found = false; out.fail = 0; out.n = 0;
    out.length = 0; out.maxSlope = 0; out.yellowCells = 0; out.redCells = 0;

    if (cellCost(mode, si, sj) < 0 || cellCost(mode, gi, gj) < 0) { out.fail = 1; return false; }

    static float g[NN], f[NN];
    static int   parent[NN];
    static unsigned char state[NN];            // 0 = unseen, 1 = open, 2 = closed
    for (int k = 0; k < NN; k++) { g[k] = 1e9f; f[k] = 1e9f; parent[k] = -1; state[k] = 0; }

    int s = sj * NC + si, t = gj * NC + gi;
    g[s] = 0; f[s] = heuristic(si, sj, gi, gj); state[s] = 1;

    for (;;)
    {
        int cur = -1; float best = 1e30f;                      // cheapest open node
        for (int k = 0; k < NN; k++)
            if (state[k] == 1 && f[k] < best) { best = f[k]; cur = k; }
        if (cur < 0) { out.fail = 2; return false; }           // nothing left to try: no path
        if (cur == t) break;

        state[cur] = 2;
        int ci = cur % NC, cj = cur / NC;

        for (int dj = -1; dj <= 1; dj++)
            for (int di = -1; di <= 1; di++)
            {
                if (di == 0 && dj == 0) continue;
                int ni = ci + di, nj = cj + dj;
                float cost = cellCost(mode, ni, nj);
                if (cost < 0) continue;

                bool diag = (di != 0 && dj != 0);
                if (diag && mode == ROUTE_ACCESSIBLE)           // no cutting through wall corners
                    if (cellCost(mode, ci + di, cj) < 0 || cellCost(mode, ci, cj + dj) < 0) continue;

                int nk = nj * NC + ni;
                if (state[nk] == 2) continue;
                float ng = g[cur] + (diag ? 1.41421356f : 1.0f) * cost;
                if (ng < g[nk])
                {
                    g[nk] = ng; parent[nk] = cur;
                    f[nk] = ng + heuristic(ni, nj, gi, gj);
                    state[nk] = 1;
                }
            }
    }

    // walk back from the goal to the start, then fill the arrays start -> goal
    int count = 0;
    for (int k = t; k >= 0; k = parent[k]) count++;
    out.n = count;
    int idx = count - 1;
    for (int k = t; k >= 0; k = parent[k]) { out.ci[idx] = k % NC; out.cj[idx] = k / NC; idx--; }

    out.found = true;
    measure(out);
    return true;
}

// ---------- shared route state (both views read this) ----------
static int   sI = -1, sJ = -1, gI = -1, gJ = -1;
static Route routes[2];
static bool  shown[2] = {true, true};

static void clearRoutes()
{
    for (int m = 0; m < 2; m++)
    { routes[m].found = false; routes[m].fail = 0; routes[m].n = 0; routes[m].length = 0;
      routes[m].maxSlope = 0; routes[m].yellowCells = 0; routes[m].redCells = 0; }
}

static void recompute()
{
    clearRoutes();
    if (sI < 0 || gI < 0) return;
    findRoute(ROUTE_SHORTEST,   sI, sJ, gI, gJ, routes[ROUTE_SHORTEST]);
    findRoute(ROUTE_ACCESSIBLE, sI, sJ, gI, gJ, routes[ROUTE_ACCESSIBLE]);
    printf("Routes: shortest %.1f m (%d steep cells) | accessible %s",
           routes[0].length, routes[0].redCells, routes[1].found ? "" : "NONE\n");
    if (routes[1].found) printf("%.1f m, steepest %.1f%%\n", routes[1].length, 100 * routes[1].maxSlope);
}

void routeInit()
{
    sI = 22; sJ = 20;          // open ground south of the stairs
    gI = 32; gJ = 9;           // on the platform: the straight line crosses its wall
    recompute();
}

void routePick(int ci, int cj)
{
    printf("Picked cell (%d,%d): slope %.1f%%, class %d\n",
           ci, cj, 100 * getCellSlope(ci, cj), getCellClass(ci, cj));
    if (sI < 0 || gI >= 0) { sI = ci; sJ = cj; gI = gJ = -1; }   // begin a new pair
    else                   { gI = ci; gJ = cj; }
    recompute();
}

void routeClear() { sI = sJ = gI = gJ = -1; recompute(); }

bool routeHasStart() { return sI >= 0; }
bool routeHasGoal()  { return gI >= 0; }
void routeStart(int *ci, int *cj) { *ci = sI; *cj = sJ; }
void routeGoal(int *ci, int *cj)  { *ci = gI; *cj = gJ; }
const Route &routeGet(int mode)   { return routes[mode]; }
bool routeShown(int mode)         { return shown[mode]; }
void toggleRouteShown(int mode)   { shown[mode] = !shown[mode]; }