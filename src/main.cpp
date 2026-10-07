#include <GL/glut.h>
#include <cstdlib>
#include "common.h"

static int winW = WIN_W, winH = WIN_H;
static bool dragging = false;               // NEW
static int lastX = 0, lastY = 0;            // NEW

void display()
{
    int leftW = winW * 2 / 3;
    int rightW = winW - leftW;

    glEnable(GL_SCISSOR_TEST);

    glViewport(0, 0, leftW, winH);
    glScissor(0, 0, leftW, winH);
    glClearColor(0.75f, 0.85f, 0.95f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw3DScene(leftW, winH);

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
    switch (key)
    {
        case 27:  exit(0);
        case 'd': case 'D': toggleDepthTest();    break;
        case 'w': case 'W': toggleWireframe();    break;
        case 'h': case 'H': toggleExaggeration(); break;
        case 's': case 'S': cycleColorMode();     break;
        case 'l': case 'L': toggleLighting();     break;   // NEW (Phase 3)
        case 'f': case 'F': toggleShadeModel();   break;
        case 'x': case 'X': toggleSpecular();     break;
        case 'p': case 'P': toggleProjection();   break;
        case 't': case 'T': camTopView();         break;
        case 'r': case 'R': camReset();           break;
        case 'n': case 'N': toggleSunAnim();      break;
        case 'm': case 'M': toggleHillshade();    break;   // Member 2's minimap toggle
        case ',': sunRotate(-10, 0);              break;
        case '.': sunRotate( 10, 0);              break;
        case '[': sunRotate(0, -5);               break;
        case ']': sunRotate(0,  5);               break;
        case '+': case '=': camZoom(0.9f);        break;
        case '-': case '_': camZoom(1.1f);        break;
        case 'g': case 'G': toggleGrid();         break;   // NEW (Phase 4)
        case 'k': case 'K': toggleTestPattern();  break;   // NEW (Phase 4)
    }
    glutPostRedisplay();
}

void special(int key, int x, int y)                      // NEW: arrow keys orbit
{
    if (key == GLUT_KEY_LEFT)  camRotate(-5, 0);
    if (key == GLUT_KEY_RIGHT) camRotate( 5, 0);
    if (key == GLUT_KEY_UP)    camRotate(0,  5);
    if (key == GLUT_KEY_DOWN)  camRotate(0, -5);
    glutPostRedisplay();
}

void mouse(int btn, int state, int x, int y)             // NEW
{
    // RIGHT drag orbits (left click is reserved for picking in Phase 6)
    if (btn == GLUT_RIGHT_BUTTON)
    {
        dragging = (state == GLUT_DOWN) && (x < winW * 2 / 3);
        lastX = x; lastY = y;
    }
    if (state == GLUT_DOWN && btn == 3) camZoom(0.9f);   // wheel up
    if (state == GLUT_DOWN && btn == 4) camZoom(1.1f);   // wheel down
}

void motion(int x, int y)                                // NEW
{
    if (dragging)
    {
        camDrag(x - lastX, y - lastY);
        lastX = x; lastY = y;
    }
}

void timer(int v)
{
    update3D();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Accessible Route & Slope Visualizer");

    generateTerrain();                                   // NEW
    computeSlopes();                                     // NEW (Phase 2)

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);                            // NEW
    glutMouseFunc(mouse);                                // NEW
    glutMotionFunc(motion);                              // NEW
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}