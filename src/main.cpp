
#include <GL/freeglut.h>

#include <cstdlib>
#include <cmath>
#include <cstdio>

#include "config.h"
#include "layout.h"
#include "scene.h"
#include "room.h"
#include "camera.h"
#include "hud.h"

using namespace cfg;

static float theta   = lay::START_DEG * RAD;   // radians
static long  turns   = 0;                      // whole revolutions so far
static float fan_deg = 0.0f;                   // the one part off the clock

static bool  machine_on = true;                // the machine's own switch
static bool  bulb_on[2] = { true, true };      // left, right
static bool  fan_on     = true;

static bool edges = true;                      // e: the edge-line pass

// The four switches, in the cabinet's order (room::Switch).
static void switch_flags(bool* sw) {
    sw[room::SW_MACHINE] = machine_on;
    sw[room::SW_BULB_L]  = bulb_on[0];
    sw[room::SW_BULB_R]  = bulb_on[1];
    sw[room::SW_FAN]     = fan_on;
}


static void draw_world(const bool* sw, bool edge_pass = false) {
    scene::draw(theta, turns);
    room::draw(cam::eye_pos, sw, fan_deg, edge_pass);
}


static const GLfloat BULB_DIFFUSE[4] = { 0.60f, 0.57f, 0.50f, 1.0f };
static const GLfloat LIGHT_OFF[4]    = { 0.00f, 0.00f, 0.00f, 1.0f };
static const GLfloat ROOM_AMBIENT[4] = { 0.44f, 0.44f, 0.47f, 1.0f };

static void place_lights(const bool* sw) {
    for (int i = 0; i < 2; ++i) {
        const GLenum l = (GLenum)(GL_LIGHT0 + i);
        const GLfloat pos[4] = { BULB_X[i], BULB_Y, BULB_Z, 1.0f };
        glLightfv(l, GL_POSITION, pos);
        glLightfv(l, GL_DIFFUSE, sw[room::SW_BULB_L + i] ? BULB_DIFFUSE
                                                         : LIGHT_OFF);
    }
}

static void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    cam::apply();

    bool sw[room::SW_COUNT];
    switch_flags(sw);
    place_lights(sw);
    if (!edges) {                               // flat colour only
        draw_world(sw);
    } else {

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        draw_world(sw);
        glDisable(GL_POLYGON_OFFSET_FILL);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        draw_world(sw, true);
        glDisable(GL_BLEND);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    const hud::State st = { sw, edges };
    hud::draw(st);
    glutSwapBuffers();
}

static void reshape(int w, int h) {
    cam::win_w = w; cam::win_h = h < 1 ? 1 : h;
    glViewport(0, 0, w, cam::win_h);
    cam::apply_projection();
}

// Everything advances by speed * dt (FR-1), never by a step per frame.
static void idle() {
    static int prev = 0;
    const int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = prev ? (now - prev) * 0.001f : 0.0f;   // first frame: dt = 0
    prev = now;
    if (dt > DT_CLAMP) dt = DT_CLAMP;   // a window drag must not jump anything

    if (machine_on) {
        theta += CRANK_DPS * dt * RAD;
        while (theta >= 2.0f * PI) {    // one more revolution of the crank,
            theta -= 2.0f * PI;         // which is one more part stamped
            ++turns;
        }
    }
    if (fan_on) fan_deg += FAN_DPS * dt;

    cam::fly(dt);
    glutPostRedisplay();
}

static void keyboard(unsigned char k, int, int) {
    switch (k) {
    case 27: glutLeaveMainLoop(); break;
    case ' ': machine_on = !machine_on; break;     // start and stop the line
    case 'e': case 'E': edges = !edges; break;
    case '[': case '{': case ']': case '}': {             // bulb switches
        const int i = (k == ']' || k == '}') ? 1 : 0;
        bulb_on[i] = !bulb_on[i];
        break;
    }
    case 'f': case 'F': fan_on = !fan_on; break;          // fan switch
    case '1': case '2':                            // also leaves free cam
        cam::preset = k - '0'; cam::orbit = 0.0f;
        cam::set_free(false);
        break;
    case 'c': case 'C': cam::set_free(!cam::free_cam); break;
    case 'r': case 'R':                            // back to the start
        theta = lay::START_DEG * RAD; turns = 0; fan_deg = 0.0f;
        cam::preset = 1; cam::orbit = 0.0f;
        cam::set_free(false);
        break;
    default: break;
    }
}

static void hold(int k, bool down) {
    switch (k) {
    case GLUT_KEY_UP:        cam::held[cam::MV_FWD]   = down; break;
    case GLUT_KEY_DOWN:      cam::held[cam::MV_BACK]  = down; break;
    case GLUT_KEY_LEFT:      cam::held[cam::MV_TURN_L] = down; break;
    case GLUT_KEY_RIGHT:     cam::held[cam::MV_TURN_R] = down; break;
    case GLUT_KEY_PAGE_UP:   cam::held[cam::MV_UP]    = down; break;
    case GLUT_KEY_PAGE_DOWN: cam::held[cam::MV_DOWN]  = down; break;
    default: break;
    }
}

static void special(int k, int, int) {
    hold(k, true);
    if (cam::free_cam) return;                  // the arrows fly instead
    switch (k) {                                // up and down had the speed
    case GLUT_KEY_LEFT:  cam::orbit -= 3.0f; break;
    case GLUT_KEY_RIGHT: cam::orbit += 3.0f; break;
    default: break;
    }
}

static void special_up(int k, int, int) { hold(k, false); }

static void init() {
    glClearColor(0.09f, 0.10f, 0.12f, 1.0f);
    glEnable(GL_DEPTH_TEST);      // hidden-surface elimination, PRD FR-15
    glEnable(GL_CULL_FACE);       // and the other half of it
    glCullFace(GL_BACK);

    glEnable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);      // interpolate across a face, not flat-fill it
    glEnable(GL_NORMALIZE);       // the blank squash is a non-uniform scale
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ROOM_AMBIENT);
    glEnable(GL_COLOR_MATERIAL);  // glColor3fv in the lists becomes the material
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    for (int i = 0; i < 2; ++i) {
        glLightfv((GLenum)(GL_LIGHT0 + i), GL_SPECULAR, LIGHT_OFF);
        glEnable((GLenum)(GL_LIGHT0 + i));
    }

    scene::build_lists();
    room::build_lists();        // after: it calls the machine's gear and blank lists
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(cam::win_w, cam::win_h);
    glutCreateWindow("Quarter Turn");

    printf("GL %s\n", glGetString(GL_VERSION));

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutSpecialUpFunc(special_up);
    glutIdleFunc(idle);
    glutMainLoop();
    return 0;
}
