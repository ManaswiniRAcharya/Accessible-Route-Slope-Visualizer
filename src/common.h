#ifndef COMMON_H
#define COMMON_H

#define WIN_W 1200
#define WIN_H 700

// ---------- Terrain data contract (terrain.cpp, Member 2) ----------
#define GRID_N    41        // vertices per side  (40 x 40 cells)
#define CELL_SIZE 2.0f      // metres between neighbouring vertices

void  generateTerrain();                // fills the height grid
float getHeight(int i, int j);          // real height in metres (0 if out of range)
float getMinHeight();
float getMaxHeight();

// grid index -> world coordinates (terrain is centred on the origin)
inline float gridToWorldX(int i) { return (i - (GRID_N - 1) * 0.5f) * CELL_SIZE; }
inline float gridToWorldZ(int j) { return (j - (GRID_N - 1) * 0.5f) * CELL_SIZE; }

// ---------- 3D side (view3d.cpp, Member 1) ----------
void draw3DScene(int w, int h);
void update3D();
void toggleDepthTest();
void toggleWireframe();
void toggleExaggeration();
void camDrag(int dx, int dy);           // mouse orbit
void camRotate(float dYaw, float dPitch);  // key orbit
void camZoom(float factor);

// ---------- 2D side (raster2d.cpp, Member 2) ----------
void draw2DPanel(int w, int h);

#endif