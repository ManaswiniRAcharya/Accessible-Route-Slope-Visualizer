#include <GL/glut.h>
#include <cstdlib>
#include "common.h"

static int winW = WIN_W, winH = WIN_H;

void display()
{
    int leftW = winW * 2 / 3;          // 3D area = 2/3 of the window
    int rightW = winW - leftW;         // 2D panel = 1/3

    glEnable(GL_SCISSOR_TEST);

    // ---- Left: 3D viewport ----
    glViewport(0, 0, leftW, winH);
    glScissor(0, 0, leftW, winH);
    glClearColor(0.75f, 0.85f, 0.95f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw3DScene(leftW, winH);

    // ---- Right: 2D raster panel ----
    glViewport(leftW, 0, rightW, winH);
    glScissor(leftW, 0, rightW, winH);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw2DPanel(rightW, winH);

    glDisable(GL_SCISSOR_TEST);
    glutSwapBuffers();
}

void reshape(int w, int h)
{
    winW = w;
    winH = (h == 0) ? 1 : h;
}

void keyboard(unsigned char key, int x, int y)
{
    if (key == 27) exit(0);            // ESC
    if (key == 'd' || key == 'D') toggleDepthTest();
    glutPostRedisplay();
}

void timer(int v)
{
    update3D();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);       // ~60 frames per second
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Accessible Route & Slope Visualizer");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}