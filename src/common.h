#ifndef COMMON_H
#define COMMON_H

#define WIN_W 1200
#define WIN_H 700

// Member 1 implements this (view3d.cpp): draws into the left viewport
void draw3DScene(int w, int h);
void toggleDepthTest();
void update3D();               // advances animation each frame

// Member 2 implements this (raster2d.cpp): draws into the right viewport
void draw2DPanel(int w, int h);

#endif