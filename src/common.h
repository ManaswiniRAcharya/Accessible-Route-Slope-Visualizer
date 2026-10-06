#ifndef COMMON_H
#define COMMON_H

#define WIN_W 1200
#define WIN_H 700

// ---------- Terrain data contract (terrain.cpp, Member 2) ----------
#define GRID_N    41
#define CELL_SIZE 2.0f

void  generateTerrain();
float getHeight(int i, int j);
float getMinHeight();
float getMaxHeight();

inline float gridToWorldX(int i) { return (i - (GRID_N - 1) * 0.5f) * CELL_SIZE; }
inline float gridToWorldZ(int j) { return (j - (GRID_N - 1) * 0.5f) * CELL_SIZE; }

// ---------- Slope analysis contract (slope.cpp, Member 2) ----------
#define SLOPE_EASY_LIMIT (1.0f / 20.0f)
#define SLOPE_MAX_LIMIT  (1.0f / 12.0f)

enum SlopeClass { SLOPE_GREEN = 0, SLOPE_YELLOW = 1, SLOPE_RED = 2 };

void  computeSlopes();
float getCellSlope(int ci, int cj);
int   getCellClass(int ci, int cj);
int   getClassCount(int cls);
void  slopeClassColor(int cls, float c[3]);
void  slopeGradientColor(float slope, float c[3]);

// ---------- Normals contract (normals.cpp, Member 2) ----------
// vscale = vertical exaggeration used when drawing (1 or 3) so lighting matches the picture
void  getVertexNormal(int i, int j, float vscale, float n[3]);   // smooth, unit length
void  getCellNormal(int ci, int cj, float n[3]);                 // real heights, unit length
float lambert(const float n[3], const float L[3]);               // max(0, N.L)

// ---------- 3D side (view3d.cpp, Member 1) ----------
void draw3DScene(int w, int h);
void update3D();
void toggleDepthTest();
void toggleWireframe();
void toggleExaggeration();
void cycleColorMode();
void toggleLighting();
void toggleShadeModel();      // flat <-> smooth
void toggleSpecular();
void toggleProjection();      // perspective <-> orthographic
void toggleSunAnim();
void sunRotate(float dAz, float dEl);
void getSunDir(float L[3]);   // unit vector pointing TOWARDS the sun (world space)
void camDrag(int dx, int dy);
void camRotate(float dYaw, float dPitch);
void camZoom(float factor);
void camTopView();
void camReset();

// ---------- 2D side (raster2d.cpp, Member 2) ----------
void draw2DPanel(int w, int h);
void toggleHillshade();

#endif