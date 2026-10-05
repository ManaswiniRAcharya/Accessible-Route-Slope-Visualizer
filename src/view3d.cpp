#include <GL/glut.h>
#include "common.h"

static float angle = 0.0f;
static bool depthOn = true;

GLfloat vertices[8][3] = {
    {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
    {-1,-1, 1},{1,-1, 1},{1,1, 1},{-1,1, 1}};
GLfloat colors[8][3] = {
    {0,0,0},{1,0,0},{1,1,0},{0,1,0},
    {0,0,1},{1,0,1},{1,1,1},{0,1,1}};

static void face(int a, int b, int c, int d)
{
    int idx[4] = {a, b, c, d};
    glBegin(GL_POLYGON);
    for (int i = 0; i < 4; i++) {
        glColor3fv(colors[idx[i]]);
        glVertex3fv(vertices[idx[i]]);
    }
    glEnd();
}

static void colorCube()
{
    face(0,3,2,1);  face(2,3,7,6);  face(0,4,7,3);
    face(1,2,6,5);  face(4,5,6,7);  face(0,1,5,4);
}

void toggleDepthTest() { depthOn = !depthOn; }
void update3D()        { angle += 0.8f; if (angle > 360) angle -= 360; }

void draw3DScene(int w, int h)
{
    if (depthOn) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / h, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(4, 3, 6,   0, 0, 0,   0, 1, 0);

    glRotatef(angle, 0, 1, 0);
    glRotatef(angle * 0.5f, 1, 0, 0);
    colorCube();
}