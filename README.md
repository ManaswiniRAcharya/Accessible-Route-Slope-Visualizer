# Accessible Route & Slope Visualizer for Wheelchair Users

A Computer Graphics and Visualization mini project (CS3002-1).
It builds a small 3D campus terrain, analyses the slope of every cell against wheelchair-accessibility limits, and compares the **shortest** route with the **accessible** route. A wheelchair then rolls along the accessible route.

Everything is written in C++ with legacy (fixed-function) OpenGL, GLU and freeglut, the same stack as the lab manual. There are no external data files, no GIS data and no database.

---

## Team

| Member | Role | Main modules |
|---|---|---|
| Member 1: *(name, USN)* | 3D side | `view3d.cpp`, `fill.cpp`, `clip.cpp`, `main.cpp`, `selftest.cpp`, `selftest_gfx.cpp` |
| Member 2: *(name, USN)* | 2D raster and analysis side | `terrain.cpp`, `slope.cpp`, `normals.cpp`, `canvas.cpp`, `raster2d.cpp`, `route.cpp`, `wheelchair.cpp`, `selftest_world.cpp` |

Guide: *(name)*  |  Department of Computer Science & Engineering, *(college)*

---

## What the project does

- Generates a procedural **campus terrain** (80 m x 80 m, 41 x 41 vertices, 2 m spacing) containing:
  - a steep hill
  - a gentle mound
  - a raised platform
  - a ramp
  - a flight of stairs
- Computes the **slope of every cell** and classifies it:

  | Colour | Slope | Meaning |
  |---|---|---|
  | Green | <= 1:20 (5 %) | Accessible |
  | Yellow | <= 1:12 (8.33 %) | Steep ramp, allowed |
  | Red | steeper | Not accessible |

  These limits follow commonly used accessibility guidelines. They are project settings in `common.h`, not legal advice.
- Lets you **click a start and a goal** in either view, then computes two routes:
  - **Shortest** (black): ignores slope.
  - **Accessible** (blue): avoids red cells and penalises yellow ones.
- Shows the length, steepest slope and detour of each route.
- Rolls an **animated wheelchair** along the accessible route. It tilts with the terrain and slows down uphill, and a follow camera can track it.
- Shows a **2D minimap** whose pixels are produced entirely by our own raster algorithms. It supports zoom and pan.

The window has two viewports: the 3D scene on the left (2/3) and the 2D raster panel on the right (1/3).

---

## Computer Graphics concepts used

| Syllabus concept | Where it is used | Lab program |
|---|---|---|
| Midpoint line (all octants) | Minimap lines, grid, routes (`canvas.cpp`) | Lab 1 |
| Midpoint circle, filled disc | Markers, wheelchair dot (`fill.cpp`) | Lab 2 |
| Scan-line polygon fill (convex) | Minimap cells, legend (`fill.cpp`) | Lab 4 |
| Cohen-Sutherland line clipping | Grid, border and route lines (`clip.cpp`) | Lab 3 |
| Sutherland-Hodgman polygon clipping | Clip demo, key **C** (`clip.cpp`) | none |
| Window-to-viewport transformation | Minimap zoom, pan and picking (`raster2d.cpp`) | Lab 8 |
| Polygon mesh | Terrain as a triangle mesh (`view3d.cpp`) | Lab 6 |
| 3D transformations, composition, homogeneous matrices | Wheelchair pose matrix, camera, markers | Lab 7 |
| Perspective and parallel projection | Keys **P** and **T** | Lab 9 |
| Illumination (ambient, diffuse, specular) | OpenGL lights, software hillshade | Lab 9 |
| Flat and Gouraud shading | Key **F** | Lab 9 |
| Z-buffer | Key **D** toggle | Lab 9 |
| Colour models (RGB, HSV) | Slope colours, HSV gradient (`slope.cpp`) | Lab 9 |
| Bezier curves | Smoothing the wheelchair path (`wheelchair.cpp`) | none |
| Fractals (diamond-square midpoint displacement) | Fractal ground, keys **Y** and **Q** (`terrain.cpp`) | idea of Lab 5 only |
| Inverse viewing pipeline (picking) | `gluUnProject` + ray march in 3D, inverse window-to-viewport in 2D | Lab 8 |

A\* route finding is a **supporting algorithm** that gives the visuals a purpose. It is not a syllabus topic.

---

## Requirements

- A C++ compiler (g++ recommended)
- OpenGL, GLU and **freeglut**
- Windows or Linux. macOS has not been tested.

### Windows (MSYS2 MinGW64 terminal)
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-freeglut make
```

### Linux (Ubuntu / Debian)
```bash
sudo apt install build-essential freeglut3-dev
```

---

## Build and run

```bash
# Windows (MSYS2 MinGW64)
g++ -O2 src/*.cpp -o slopeviz.exe -lfreeglut -lopengl32 -lglu32
./slopeviz.exe

# Linux
g++ -O2 src/*.cpp -o slopeviz -lglut -lGLU -lGL
./slopeviz
```

Or with the Makefile:
```bash
make        # build
make run    # build and run
make test   # build and run the self-tests
```

With Code::Blocks, add every `.cpp` file in `src/` to one project and link `freeglut`, `opengl32`, `glu32` in that order.

---

## Self-tests (no window needed)

```bash
./slopeviz --test       # Linux
./slopeviz.exe --test   # Windows
```

This runs about 50 automatic checks and prints `N checks, 0 failed` when everything passes. They cover:

- exact pixel results for the midpoint line, circle and scan fill
- Cohen-Sutherland and Sutherland-Hodgman results
- terrain, slope classes, normals and Lambert's law
- route logic (shortest vs. accessible, blocked goals)
- fractal terrain (the campus core is unchanged, and seeds are repeatable)

---

## Controls

### Mouse

| Action | Effect |
|---|---|
| Right-drag in the 3D view | Orbit the camera |
| Mouse wheel over the 3D view | Zoom the camera |
| Mouse wheel over the minimap | Zoom the minimap toward the cursor |
| Left-drag in the minimap | Pan the minimap |
| Left-click in either view | Place the START (first click) and GOAL (second click). A third click starts a new pair. |

A press that moves more than 4 px counts as a drag, not a click.

### Keyboard

| Key | Function |
|---|---|
| **Esc** | Quit |
| **Arrow keys** | Orbit the camera |
| **+** / **-** | Zoom the 3D camera |
| **T** | Top-down view |
| **R** | Reset the camera |
| **P** | Perspective / orthographic projection |
| **D** | Depth test (Z-buffer) on / off |
| **W** | Wireframe on / off |
| **H** | 3x height exaggeration (drawing only) |
| **S** | Colour mode: height, slope classes, slope gradient (HSV) |
| **L** | Lighting on / off |
| **F** | Flat / smooth (Gouraud) shading |
| **X** | Specular highlight on / off |
| **,** **.** | Rotate the sun (azimuth) |
| **[** **]** | Sun elevation |
| **N** | Animate the sun |
| **M** | Minimap hillshade on / off |
| **G** | Minimap grid on / off |
| **z** / **Z** | Minimap zoom in / out (about its centre) |
| **V** | Reset the minimap view |
| **K** | Raster test pattern (lines in all octants, circles, fills) |
| **C** | Clipping demo (Cohen-Sutherland and Sutherland-Hodgman) |
| **1** | Show / hide the shortest route (black) |
| **2** | Show / hide the accessible route (blue) |
| **E** | Clear start and goal |
| **Space** | Wheelchair play / pause (restarts if finished) |
| **B** | Restart the wheelchair |
| **U** / **J** | Wheelchair faster / slower |
| **O** | Follow camera on / off |
| **Y** | Fractal ground on / off |
| **Q** | New random fractal seed (turns fractal ground on) |

---

## Suggested demo (about 8 minutes)

1. **Startup:** the green/yellow/red terrain, the minimap and both routes.
2. **Camera:** orbit, zoom, **P** for orthographic, **T** for top view.
3. **Lighting:** **L**, **F**, **X**, rotate the sun, and watch the minimap hillshade change.
4. **Z-buffer:** orbit low, press **D** off, then on.
5. **Colour:** **S** cycles the colour modes.
6. **Own algorithms:** **K** (test pattern), then **C** (clipping demo).
7. **Window-to-viewport:** wheel-zoom and drag-pan the minimap.
8. **Routes:** click a start and a goal across the platform wall, then compare black and blue.
9. **Wheelchair:** **Space**, then **O** for the follow camera.
10. **Fractals:** **Y**, then **Q** a few times, then **Y** off.

---

## Project structure

```
SlopeViz/
├── README.md
├── Makefile
└── src/
    ├── common.h           shared declarations (the contract between modules)
    ├── main.cpp           window, layout, input, timer, --test entry
    ├── terrain.cpp        heightmap, campus features, diamond-square fractal
    ├── slope.cpp          per-cell slope, classification, RGB/HSV colours
    ├── normals.cpp        surface normals, Lambert's cosine law
    ├── view3d.cpp         3D scene: mesh, lighting, camera, picking, routes, wheelchair model
    ├── canvas.cpp         software pixel buffer, midpoint line
    ├── fill.cpp           midpoint circle, filled disc, scan-line polygon fill, test pattern
    ├── clip.cpp           Cohen-Sutherland, Sutherland-Hodgman, clipping demo
    ├── raster2d.cpp       minimap: window-to-viewport, zoom, pan, picking, legend
    ├── route.cpp          A* shortest and accessible routes, shared route state
    ├── wheelchair.cpp     Bezier path, motion, terrain pose, speed model
    ├── selftest.cpp       test runner
    ├── selftest_gfx.cpp   raster and clipping tests
    └── selftest_world.cpp terrain, slope, route and fractal tests
```

### Data flow

```
terrain.cpp -> slope.cpp -> normals.cpp
                  |
                  +--> route.cpp -> wheelchair.cpp
                  |         |             |
                  v         v             v
              view3d.cpp (3D)     raster2d.cpp (2D, via canvas/fill/clip)
```

Every view reads the same data through the functions in `common.h`, so the 3D scene and the minimap can never disagree.

---

## Design decisions worth knowing

- **Procedural terrain** avoids GIS and file-format problems and keeps the focus on graphics.
- **Slope uses real heights.** Key **H** (3x exaggeration) only affects drawing.
- **Triangles, not quads**, because a triangle is always planar. The wheelchair and picking use the same diagonal split as the mesh, so they sit exactly on the surface.
- **The minimap never calls OpenGL drawing primitives.** Every pixel goes through `canvasPixel()`, and OpenGL only displays the finished buffer with `glDrawPixels`.
- **The campus core is protected from fractal noise** by a smooth mask, so the ramp, platform and stairs keep their designed slopes.
- **The model is scaled 3x** and the chair moves at a 5 m/s demo speed so the scene is visible. A real wheelchair moves at about 1.4 m/s.

---

## Limitations (what is NOT implemented)

- Antialiasing
- Phong (per-pixel) shading. Fixed-function OpenGL gives flat and Gouraud only.
- B-Spline curves or surfaces, and Bezier surfaces
- Concave polygon fill. Scan fill is correct for convex polygons only.
- Polygon clipping against non-rectangular windows
- CMY and YIQ colour models
- List-priority, scan-line and area-subdivision visible-surface algorithms. Only the Z-buffer is used.
- Real map or GIS data
- Routes are computed per grid cell on an 8-neighbour grid, so they follow cell centres. The Bezier smoothing only rounds the visual path.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `GL/glut.h: No such file` | Install freeglut (see Requirements). |
| `undefined reference to glutInit` | Check the link order and libraries (see Build and run). |
| `undefined reference to <our function>` | Compile every file: use `src/*.cpp`. |
| Window flickers | `GLUT_DOUBLE` or `glutSwapBuffers()` is missing. |
| `--test` opens a window | The `--test` check must be the first thing in `main()`. |
| Clicks in the 3D view do nothing | The matrices for picking are captured right after `gluLookAt`. Check that those three `glGet*` lines are present. |
| `make: missing separator` | Makefile recipe lines must start with a tab. |
| Low frame rate | Compile with `-O2`. |

---

## Credits

- Course: Computer Graphics and Visualization (CS3002-1)
- Textbooks: Foley et al., *Computer Graphics*; Angel, *Computer Graphics: A Top-Down Approach with OpenGL*
- Lab programs 1 to 9 from the course lab manual were used as the foundation for the raster, clipping, mesh, transformation, mouse and 3D viewing code.
