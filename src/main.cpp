
#include <GL/glew.h>        // first: it must come before any other GL header
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
#include "shading.h"
#include "shadows.h"

using namespace cfg;

static float theta   = lay::ENGAGE_DEG * RAD;  // radians; just after an index
static long  turns   = 0;                      // whole revolutions so far
static float fan_deg = 0.0f;                   // the one part off the clock

// The four switches, in the cabinet's order: machine, left bulb, right bulb, fan.
static bool sw[room::SW_COUNT] = { true, true, true, true };

static bool edges = true;                      // e: the edge-line pass
static shade::Mode mode = shade::RAYS;         // s: flat, Gouraud, Phong, rays


static void draw_world(bool edge_pass = false) {
    scene::draw(theta, turns);
    room::draw(sw, fan_deg, edge_pass);
}


static const GLfloat BULB_DIFFUSE[4]  = { 0.85f, 0.80f, 0.70f, 1.0f };
static const GLfloat BULB_SPECULAR[4] = { 0.90f, 0.88f, 0.82f, 1.0f };
static const GLfloat LIGHT_OFF[4]     = { 0.00f, 0.00f, 0.00f, 1.0f };
static const GLfloat ROOM_AMBIENT[4]  = { 0.33f, 0.32f, 0.30f, 1.0f };   // warm: bounced light
static const GLfloat SPOT_DOWN[3]     = { 0.0f, -1.0f, 0.0f };

// Daylight through the windows: a third, directional light (w = 0), cool
// against the warm bulbs, from the front left and above.
static const GLfloat DAYLIGHT[4]     = { 0.30f, 0.33f, 0.40f, 1.0f };
static const GLfloat DAYLIGHT_DIR[4] = { -0.45f, 0.75f, 0.50f, 0.0f };

// After the camera: the position and the spot direction are both moved by
// the modelview matrix when they are set.
static void place_lights() {
    for (int i = 0; i < 2; ++i) {
        const GLenum l = (GLenum)(GL_LIGHT0 + i);
        const bool on = sw[room::SW_BULB_L + i];
        const GLfloat pos[4] = { BULB_X[i], BULB_Y, BULB_Z, 1.0f };
        glLightfv(l, GL_POSITION, pos);
        glLightfv(l, GL_SPOT_DIRECTION, SPOT_DOWN);
        glLightfv(l, GL_DIFFUSE,  on ? BULB_DIFFUSE  : LIGHT_OFF);
        glLightfv(l, GL_SPECULAR, on ? BULB_SPECULAR : LIGHT_OFF);
    }
    glLightfv(GL_LIGHT2, GL_POSITION, DAYLIGHT_DIR);
}

static void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    cam::apply();

    place_lights();
    shade::apply(mode);
    if (mode == shade::RAYS) shadows::upload(shade::program, theta, turns);
    if (!edges) {                               // flat colour only
        draw_world();
    } else {

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        draw_world();
        glDisable(GL_POLYGON_OFFSET_FILL);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        draw_world(true);
        glDisable(GL_BLEND);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    shade::off();
    hud::draw(sw, edges, shade::NAME[mode]);
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

    if (sw[room::SW_MACHINE]) {
        theta += CRANK_DPS * dt * RAD;
        while (theta >= 2.0f * PI) {    // one more revolution of the crank,
            theta -= 2.0f * PI;         // which is one more part stamped
            ++turns;
        }
    }
    if (sw[room::SW_FAN]) fan_deg += FAN_DPS * dt;

    cam::fly(dt);
    glutPostRedisplay();
}

static void keyboard(unsigned char k, int, int) {
    switch (k) {
    case 27: glutLeaveMainLoop(); break;
    case ' ': sw[room::SW_MACHINE] = !sw[room::SW_MACHINE]; break;  // start, stop
    case '[': case '{': sw[room::SW_BULB_L] = !sw[room::SW_BULB_L]; break;
    case ']': case '}': sw[room::SW_BULB_R] = !sw[room::SW_BULB_R]; break;
    case 'f': case 'F': sw[room::SW_FAN]    = !sw[room::SW_FAN];    break;
    case 'e': case 'E': edges = !edges; break;
    case 's': case 'S':                            // flat -> Gouraud -> Phong
        mode = (shade::Mode)((mode + 1) % shade::MODE_COUNT);
        if (mode >= shade::PHONG && !shade::program) mode = shade::FLAT;
        break;
    case '1': case '2':                            // also leaves free cam
        cam::preset = k - '0'; cam::orbit = 0.0f;
        cam::set_free(false);
        break;
    case 'c': case 'C': cam::set_free(!cam::free_cam); break;
    case 'r': case 'R':                            // back to the start
        theta = lay::ENGAGE_DEG * RAD; turns = 0; fan_deg = 0.0f;
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
    case GLUT_KEY_LEFT:  cam::orbit = fmaxf(cam::orbit - 3.0f, -ORBIT_MAX); break;
    case GLUT_KEY_RIGHT: cam::orbit = fminf(cam::orbit + 3.0f,  ORBIT_MAX); break;
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
    glEnable(GL_NORMALIZE);       // the blank squash is a non-uniform scale
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ROOM_AMBIENT);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);   // true V for specular
    glEnable(GL_COLOR_MATERIAL);  // glColor3fv in the lists becomes ka and kd
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    for (int i = 0; i < 2; ++i) {
        const GLenum l = (GLenum)(GL_LIGHT0 + i);
        glLightf(l, GL_CONSTANT_ATTENUATION,  BULB_A0);
        glLightf(l, GL_LINEAR_ATTENUATION,    BULB_A1);
        glLightf(l, GL_QUADRATIC_ATTENUATION, BULB_A2);
        glLightf(l, GL_SPOT_CUTOFF,   BULB_CUTOFF);
        glLightf(l, GL_SPOT_EXPONENT, BULB_SPOT_EXP);
        glEnable(l);
    }
    glLightfv(GL_LIGHT2, GL_DIFFUSE, DAYLIGHT);    // no specular: soft sky light
    glEnable(GL_LIGHT2);

    shade::build();
    if (!shade::program) mode = shade::GOURAUD;    // no OpenGL 2.0: no shaders

    scene::build_lists();
    room::build_lists();        // after: it calls the machine's gear and blank lists
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(cam::win_w, cam::win_h);
    glutCreateWindow("Quarter Turn");

    // Windows' opengl32.dll stops at OpenGL 1.1; GLEW finds the 2.0 shader
    // functions in the driver.  It needs the window's context, so it comes here.
    glewInit();
    printf("GL %s, GLSL %s\n", glGetString(GL_VERSION),
           glGetString(GL_SHADING_LANGUAGE_VERSION));

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
