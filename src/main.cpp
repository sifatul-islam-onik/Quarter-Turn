// "Quarter Turn" - an automated stamping line in OpenGL.
//
// STATIC BUILD - the objects only, with nothing in motion.  The machine is
// held at one fixed crank angle, and every part of it - five meshing gears, a
// press, an intermittent belt and the blanks it carries - is drawn at the pose
// that one angle gives it.  Every other file - config.h, kinematics.h, prim.h,
// materials.h, scene.h, room.h - is the animated build's, untouched; what is
// gone is the clock that advanced the angle, so the scene can be pointed at
// and explained part by part.
//
// Still live, because none of it is the machine moving: the camera (four
// preset views, orbit and the free camera), the room's four switches, the
// wireframe toggle and the edge pass.
//
// There is no lighting in this build either.  Every part is a flat colour, and
// a second pass draws the same geometry as dark edge lines so the shapes still
// read (see display()).
//
// GLEW must be included before freeglut, and glewInit() must run after
// glutCreateWindow() because it needs a live context (PRD 8).  Windows ships
// OpenGL 1.1, and the edge pass's glBlendColor is 1.4.
#include <GL/glew.h>
#include <GL/freeglut.h>

#include <cstdlib>
#include <cmath>
#include <cstdio>

#include "config.h"
#include "kinematics.h"
#include "scene.h"
#include "room.h"

using namespace cfg;

// ---------------------------------------------------------------------------
// The pose.  In the animated build these two are the whole animation state
// (PRD FR-2) and the render loop advances theta; here they are constants, so
// every mechanism is drawn at one instant and stays there.
//
// 90 degrees is the angle that build resets to, and it is the clearest one to
// stand on: the belt is stationary between indexes, the ram is half way down
// its stroke well clear of the blanks, and the Geneva driver's pin is outside
// its slot - so each mechanism is caught where it can be pointed at.
// ---------------------------------------------------------------------------
static const float POSE_THETA = RESET_THETA_DEG * RAD;  // crank angle, radians
static const long  POSE_CYCLE = 0;                      // parts finished so far

static bool  wireframe = false;

// The room's switches, independent of each other.  They still change what is
// drawn - the lever's throw, its lens, the bulbs and their halos - but nothing
// in this build moves over time, so the fan's blades hold still with its
// switch on, at the one angle below.
static bool  machine_on = true;                // the machine's own switch
static bool  bulb_on[2] = { true, true };      // left, right
static bool  fan_on  = true;
static const float FAN_DEG = 0.0f;             // the fan's fixed blade angle

// Rendering state
static bool edges   = true;                    // e: the edge-line pass
static bool details = false;                   // h: the full technical readout

// Camera
static int   preset = 1;
static float orbit  = 0.0f;                    // degrees about the look-at
static float eye_pos[3] = { EYE_X, EYE_Y, EYE_Z }; // world space; decides the
                                                // room's cutaway walls
static int   win_w = 1180, win_h = 700;

// Free camera: eye_pos plus a heading.  Yaw 0 looks down -z; positive yaw
// turns right, positive pitch looks up.  The keys are tracked as held, not
// acted on per key-repeat, so flying is speed * dt like every other motion.
static bool  free_cam = false;
static float free_yaw = 0.0f, free_pitch = 0.0f;   // degrees
enum { MV_FWD = 0, MV_BACK, MV_LEFT, MV_RIGHT, MV_UP, MV_DOWN, MV_COUNT };
static bool  held[MV_COUNT] = {};
static bool  dragging = false;
static int   drag_x = 0, drag_y = 0;

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------
static float clampf(float v, float lo, float hi) { return fminf(hi, fmaxf(lo, v)); }

// The preset's eye, orbited about its look-at point.
static void preset_view(float* eye, float* at, float* up) {
    float ex, ey, ez, ax, ay, az, ux = 0, uy = 1, uz = 0;
    switch (preset) {
    case 2:                                     // front elevation
        ex = 1.8f; ey = 3.2f; ez = 12.0f; ax = 1.8f; ay = 3.2f; az = 0.0f;
        break;
    case 3:                                     // top plan
        // 12.0 is as low as this can go: the stack light's top at 6.30 must
        // stay outside the 4.0 near plane.  It is above the ceiling, so the
        // room draws no ceiling, beams or lamps in this view.
        ex = AT_X; ey = 12.0f; ez = AT_Z; ax = AT_X; ay = AT_Y; az = AT_Z;
        ux = 0; uy = 0; uz = -1;
        break;
    case 4:                                     // the whole room
        ex = OVER_EYE_X; ey = OVER_EYE_Y; ez = OVER_EYE_Z;
        ax = OVER_AT_X;  ay = OVER_AT_Y;  az = OVER_AT_Z;
        break;
    default:                                    // three-quarter, front right
        ex = EYE_X; ey = EYE_Y; ez = EYE_Z; ax = AT_X; ay = AT_Y; az = AT_Z;
        break;
    }
    const float c = cosf(orbit * RAD), s = sinf(orbit * RAD);
    const float dx = ex - ax, dz = ez - az;
    eye[0] = ax + dx * c + dz * s;
    eye[1] = ey;
    eye[2] = az - dx * s + dz * c;
    at[0] = ax; at[1] = ay; at[2] = az;
    up[0] = ux; up[1] = uy; up[2] = uz;
}

static void free_forward(float* f) {
    const float cy = cosf(free_yaw * RAD),   sy = sinf(free_yaw * RAD);
    const float cp = cosf(free_pitch * RAD), sp = sinf(free_pitch * RAD);
    f[0] = sy * cp; f[1] = sp; f[2] = -cy * cp;
}

static void apply_camera() {
    float at[3], up[3] = { 0.0f, 1.0f, 0.0f };
    if (free_cam) {
        free_forward(at);
        for (int i = 0; i < 3; ++i) at[i] += eye_pos[i];
    } else {
        preset_view(eye_pos, at, up);
    }
    gluLookAt(eye_pos[0], eye_pos[1], eye_pos[2], at[0], at[1], at[2],
              up[0], up[1], up[2]);
}

// The free camera flies to within inches of a blank, which the preset views'
// 4.0 near plane would cut away, so each mode has its own (config.h).
static void apply_projection() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOVY, (double)win_w / win_h,
                   free_cam ? FREE_ZNEAR : ZNEAR, ZFAR);
    glMatrixMode(GL_MODELVIEW);
}

// Entering free mode takes over the current view where it stands.
static void set_free_cam(bool on) {
    if (on && !free_cam) {
        float at[3], up[3];
        preset_view(eye_pos, at, up);
        float dx = at[0] - eye_pos[0], dz = at[2] - eye_pos[2];
        const float dy = at[1] - eye_pos[1];
        const float len = sqrtf(dx * dx + dy * dy + dz * dz);
        free_pitch = clampf(asinf(dy / len) * DEG, -FREE_PITCH_MAX, FREE_PITCH_MAX);
        // The plan view looks straight down and has no heading of its own; its
        // up vector is what reads as "ahead" on screen, so keep that.
        if (fabsf(dx) + fabsf(dz) < 1e-3f * len) { dx = up[0]; dz = up[2]; }
        free_yaw = atan2f(dx, -dz) * DEG;
    }
    free_cam = on;
    dragging = false;
    apply_projection();
}

// FR-1 again: the camera moves speed * dt, forward along the look direction and
// sideways level with the floor, so strafing never climbs.
static void fly(float dt) {
    if (!free_cam) return;
    float f[3];
    free_forward(f);
    const float r[3] = { cosf(free_yaw * RAD), 0.0f, sinf(free_yaw * RAD) };
    const float fwd  = (float)held[MV_FWD]   - (float)held[MV_BACK];
    const float side = (float)held[MV_RIGHT] - (float)held[MV_LEFT];
    const float rise = (float)held[MV_UP]    - (float)held[MV_DOWN];
    const float step = FREE_SPEED * dt;
    for (int i = 0; i < 3; ++i)
        eye_pos[i] += step * (fwd * f[i] + side * r[i]);
    eye_pos[1] = clampf(eye_pos[1] + step * rise, FREE_Y_MIN, FREE_Y_MAX);

    const float cx = 0.5f * (ROOM_X0 + ROOM_X1), cz = 0.5f * (ROOM_Z0 + ROOM_Z1);
    const float dx = eye_pos[0] - cx, dz = eye_pos[2] - cz;
    const float d = sqrtf(dx * dx + dz * dz);
    if (d > FREE_RADIUS) {
        eye_pos[0] = cx + dx * FREE_RADIUS / d;
        eye_pos[2] = cz + dz * FREE_RADIUS / d;
    }
}

// The four switches, in the cabinet's order (room::Switch).
static void switch_flags(bool* sw) {
    sw[room::SW_MACHINE] = machine_on;
    sw[room::SW_BULB_L]  = bulb_on[0];
    sw[room::SW_BULB_R]  = bulb_on[1];
    sw[room::SW_FAN]     = fan_on;
}

// ---------------------------------------------------------------------------
// FR-14 - on-screen indications, kept minimal: one small panel with the
// switches and the line's speed, a one-line key hint, and the full technical
// readout behind `h`.  The three numbered steps in hud() are each load-bearing:
// skipping any one produces text that is off screen or behind the machine, and
// both failures look as if it was never drawn.
// ---------------------------------------------------------------------------
static void* const SANS = GLUT_BITMAP_HELVETICA_12;
static void* const MONO = GLUT_BITMAP_8_BY_13;     // numbers that must not jitter

// glRasterPos latches the current colour, so set the colour before calling.
static void text(float x, float y, const char* s, void* font = SANS) {
    glRasterPos2f(x, y);
    while (*s) glutBitmapCharacter(font, *s++);
}

static float text_w(const char* s, void* font = SANS) {
    return (float)glutBitmapLength(font, (const unsigned char*)s);
}

static void text_right(float x_right, float y, const char* s, void* font = SANS) {
    text(x_right - text_w(s, font), y, s, font);
}

// Translucent backing, so the text reads over a bright window or a dark wall.
static void backing(float x0, float y0, float x1, float y1) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.04f, 0.05f, 0.07f, 0.74f);
    glRectf(x0, y0, x1, y1);
    glDisable(GL_BLEND);
}

static void dot(float cx, float cy, const float* rgb, bool on) {
    if (on) glColor3fv(rgb); else glColor3f(0.30f, 0.31f, 0.34f);
    glBegin(GL_TRIANGLE_FAN);                   // anticlockwise: survives culling
    glVertex2f(cx, cy);
    for (int k = 0; k <= 16; ++k) {
        const float a = 2.0f * PI * k / 16;
        glVertex2f(cx + 4.0f * cosf(a), cy + 4.0f * sinf(a));
    }
    glEnd();
}

// One label / value / key row of the status panel.
static void status_row(float x0, float x1, float y, const char* label,
                       const char* value, bool bright, const char* key) {
    const float pad = 12.0f;
    glColor3f(0.72f, 0.74f, 0.78f);
    text(x0 + pad + 14.0f, y, label);
    if (bright) glColor3f(0.96f, 0.96f, 0.94f); else glColor3f(0.48f, 0.50f, 0.54f);
    text_right(x1 - pad - 44.0f, y, value);
    glColor3f(0.56f, 0.58f, 0.62f);
    text_right(x1 - pad, y, key);
}

static void status_panel(float& bottom) {
    static const char* LABEL[room::SW_COUNT] = { "Machine", "Left bulb",
                                                 "Right bulb", "Fan" };
    static const char* KEY[room::SW_COUNT]   = { "space", "[", "]", "f" };
    bool sw[room::SW_COUNT];
    switch_flags(sw);

    const float x0 = 12.0f, x1 = x0 + 236.0f, pad = 12.0f, row = 20.0f;
    const float top = (float)win_h - 12.0f;
    const int extra = (wireframe ? 1 : 0) + (edges ? 0 : 1);
    const float y_title   = top - pad - 10.0f;
    const float y_sub     = y_title - 15.0f;
    const float y_sw0     = y_sub - 8.0f - row;
    const float y_sw_last = y_sw0 - (room::SW_COUNT - 1) * row;
    const float y_extra0  = y_sw_last - 8.0f - row;
    const float y_last    = extra ? y_extra0 - (extra - 1) * row : y_sw_last;
    bottom = y_last - pad + 2.0f;
    backing(x0, bottom, x1, top);

    glColor3f(0.96f, 0.96f, 0.94f);
    text(x0 + pad, y_title, "QUARTER TURN");
    glColor3f(0.56f, 0.58f, 0.62f);
    text(x0 + pad, y_sub, "objects only - nothing moves");

    for (int k = 0; k < room::SW_COUNT; ++k) {
        const float y = y_sw0 - k * row;
        dot(x0 + pad + 4.0f, y + 4.0f, room::SWITCH_RGB[k], sw[k]);
        status_row(x0, x1, y, LABEL[k], sw[k] ? "ON" : "OFF", sw[k], KEY[k]);
    }

    // Non-default render states get a row only while they are on.
    float y = y_extra0;
    if (wireframe) { status_row(x0, x1, y, "Wireframe", "ON", true, "w"); y -= row; }
    if (!edges)    { status_row(x0, x1, y, "Edges", "OFF", true, "e");    y -= row; }
}

// The technical readout the README's walk-through refers to, behind `h`: the
// numbers behind the pose on screen, all of them constant in this build.
static void details_panel(float top) {
    struct Line { float r, g, b; char s[160]; };
    Line L[10];
    int n = 0;
    auto add = [&](float r, float g, float bl) -> char* {
        L[n].r = r; L[n].g = g; L[n].b = bl; L[n].s[0] = 0;
        return L[n++].s;
    };

    const kin::Phase ph = kin::phase_of(POSE_THETA);
    const float B  = kin::belt_travel(POSE_THETA, POSE_CYCLE);
    const float pf = kin::punch_face(POSE_THETA);
    const float h7 = kin::press_blank_h(POSE_THETA);

    snprintf(add(0.95f, 0.95f, 0.90f), 160, "theta %6.1f deg (held)  %-8s  %s",
             POSE_THETA * DEG, kin::phase_name(ph),
             kin::belt_locked(ph) ? "belt locked" : "belt free");
    snprintf(add(0.80f, 0.86f, 0.95f), 160,
             "punch face %.3f  blank top %.3f  clearance %+.3f",
             pf, BELT_TOP_Y + h7, pf - (BELT_TOP_Y + h7));
    snprintf(add(0.80f, 0.86f, 0.95f), 160,
             "Geneva %7.2f deg  belt %.4f stations  index %.3f",
             kin::wheel_angle_deg(B), B, kin::index_progress(POSE_THETA));
    snprintf(add(0.80f, 0.86f, 0.95f), 160,
             "teeth %d:%d:%d:%d:%d   one turn of the crank is one part",
             TEETH[0], TEETH[1], TEETH[2], TEETH[3], TEETH[4]);

    snprintf(add(0.50f, 0.52f, 0.56f), 160,
             "static build - the crank does not turn   w wireframe   e edges");

    const float x0 = 12.0f, pad = 12.0f, row = 17.0f;
    float w = 0.0f;
    for (int i = 0; i < n; ++i) w = fmaxf(w, text_w(L[i].s, MONO));
    backing(x0, top - 2.0f * pad - n * row + 4.0f, x0 + w + 2.0f * pad, top);
    float y = top - pad - 10.0f;
    for (int i = 0; i < n; ++i, y -= row) {
        glColor3f(L[i].r, L[i].g, L[i].b);
        text(x0 + pad, y, L[i].s, MONO);
    }
}

// Keys in a single line along the bottom: key bright, what it does dim.  The
// free camera repurposes the arrows, so it gets its own line.
static void key_hint() {
    static const char* VIEW[][2] = { { "h", "details" }, { "1-4", "view" },
                                     { "c", "free camera" },
                                     { "left right", "orbit" },
                                     { "r", "reset view" }, { "esc", "quit" } };
    static const char* FREE[][2] = { { "arrows", "fly" }, { "pgup pgdn", "rise, sink" },
                                     { "drag", "look" },
                                     { "c", "exit free camera" }, { "esc", "quit" } };
    const char* (*K)[2] = free_cam ? FREE : VIEW;
    const int n = free_cam ? (int)(sizeof FREE / sizeof FREE[0])
                           : (int)(sizeof VIEW / sizeof VIEW[0]);
    float w = 0.0f;
    for (int i = 0; i < n; ++i)
        w += text_w(K[i][0]) + 5.0f + text_w(K[i][1]) + (i + 1 < n ? 18.0f : 0.0f);
    backing(12.0f, 10.0f, 12.0f + w + 24.0f, 32.0f);
    float x = 24.0f;
    for (int i = 0; i < n; ++i) {
        glColor3f(0.90f, 0.91f, 0.93f);
        text(x, 17.0f, K[i][0]);
        x += text_w(K[i][0]) + 5.0f;
        glColor3f(0.52f, 0.54f, 0.58f);
        text(x, 17.0f, K[i][1]);
        x += text_w(K[i][1]) + 18.0f;
    }
}

static void hud() {
    glDisable(GL_DEPTH_TEST);                   // 1. never behind the machine
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, win_w, 0, win_h);             // 2. raster pos in screen space
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();

    float bottom = 0.0f;
    status_panel(bottom);
    if (details) details_panel(bottom - 8.0f);
    key_hint();

    glPopMatrix();                              // 3. restore both matrices
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

// ---------------------------------------------------------------------------
// GLUT callbacks
// ---------------------------------------------------------------------------
static void draw_world(const bool* sw, bool edge_pass = false) {
    scene::draw(POSE_THETA, POSE_CYCLE);
    room::draw(eye_pos, POSE_THETA, POSE_CYCLE, FAN_DEG, sw, edge_pass);
}

// An edge pixel keeps this fraction of the face colour under it.
static const GLfloat EDGE_K = 0.40f;

// The edge pass cannot just set a dark colour, because every display list
// sets its own colours as it draws.  So the colour is thrown away at the blend
// instead: source factor GL_ZERO, destination factor GL_CONSTANT_COLOR, which
// gives  result = face colour already in the framebuffer * EDGE_K.  Without
// OpenGL 1.4 the fallback is the 1.1 logic op GL_CLEAR, which writes black.
static void edge_blend(bool on) {
    if (GLEW_VERSION_1_4) {
        if (on) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_ZERO, GL_CONSTANT_COLOR);
            glBlendColor(EDGE_K, EDGE_K, EDGE_K, 1.0f);
        } else {
            glDisable(GL_BLEND);
        }
    } else {
        if (on) { glEnable(GL_COLOR_LOGIC_OP); glLogicOp(GL_CLEAR); }
        else      glDisable(GL_COLOR_LOGIC_OP);
    }
}

static void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    apply_camera();

    bool sw[room::SW_COUNT];
    switch_flags(sw);
    if (wireframe) {                            // lines only, in each part's colour
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        draw_world(sw);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else if (!edges) {                        // flat colour only
        draw_world(sw);
    } else {
        // Pass 1: the faces, each pushed slightly back in depth.  Without the
        // offset an edge line and its own face land at the same depth and
        // fight, so the line comes and goes along its length.
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        draw_world(sw);
        glDisable(GL_POLYGON_OFFSET_FILL);

        // Pass 2: the same geometry as lines, darkening the faces under them.
        // Back-face culling and the depth test still apply to lines, so hidden
        // edges stay hidden.  Depth is written, so where two quads share an
        // edge the second line fails GL_LESS and it is not darkened twice.
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        edge_blend(true);
        draw_world(sw, true);
        edge_blend(false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    room::draw_glow(sw);                        // halos last, over everything

    hud();
    glutSwapBuffers();
}

static void reshape(int w, int h) {
    win_w = w; win_h = h < 1 ? 1 : h;
    glViewport(0, 0, w, win_h);
    apply_projection();
}

// The scene is fixed, so the only thing left with a speed is the free camera,
// and it still flies speed * dt (FR-1) rather than a step per frame.
static void idle() {
    static int prev = 0;
    const int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = prev ? (now - prev) * 0.001f : 0.0f;   // first frame: dt = 0
    prev = now;
    if (dt > DT_CLAMP) dt = DT_CLAMP;   // a window drag must not jump the camera
    fly(dt);
    glutPostRedisplay();
}

static void keyboard(unsigned char k, int, int) {
    switch (k) {
    case 27: glutLeaveMainLoop(); break;
    // The machine switch throws its lever and lights its lens like the other
    // three; with the clock gone it starts nothing.
    case ' ': machine_on = !machine_on; break;
    case 'w': case 'W': wireframe = !wireframe; break;
    case 'e': case 'E': edges = !edges; break;
    case '[': case '{': case ']': case '}': {             // bulb switches
        const int i = (k == ']' || k == '}') ? 1 : 0;
        bulb_on[i] = !bulb_on[i];
        break;
    }
    case 'f': case 'F': fan_on = !fan_on; break;          // fan switch
    case 'h': case 'H': details = !details; break;
    case '1': case '2': case '3': case '4':               // also leaves free cam
        preset = k - '0'; orbit = 0.0f;
        set_free_cam(false);
        break;
    case 'c': case 'C': set_free_cam(!free_cam); break;
    // Nothing about the scene to reset, so r puts the camera back instead.
    case 'r': case 'R':
        preset = 1; orbit = 0.0f;
        set_free_cam(false);
        break;
    default: break;
    }
}

// Held state is tracked in every mode, so a key already down when c is pressed
// flies straight away.  Key-repeat only re-sets it.
static void hold(int k, bool down) {
    switch (k) {
    case GLUT_KEY_UP:        held[MV_FWD]   = down; break;
    case GLUT_KEY_DOWN:      held[MV_BACK]  = down; break;
    case GLUT_KEY_LEFT:      held[MV_LEFT]  = down; break;
    case GLUT_KEY_RIGHT:     held[MV_RIGHT] = down; break;
    case GLUT_KEY_PAGE_UP:   held[MV_UP]    = down; break;
    case GLUT_KEY_PAGE_DOWN: held[MV_DOWN]  = down; break;
    default: break;
    }
}

static void special(int k, int, int) {
    hold(k, true);
    if (free_cam) return;                       // the arrows fly instead
    switch (k) {                                // up and down had the speed
    case GLUT_KEY_LEFT:  orbit -= 3.0f; break;
    case GLUT_KEY_RIGHT: orbit += 3.0f; break;
    default: break;
    }
}

static void special_up(int k, int, int) { hold(k, false); }

// Left-drag looks around in free mode: the view follows the mouse.
static void mouse(int button, int state, int x, int y) {
    if (button != GLUT_LEFT_BUTTON) return;
    dragging = (state == GLUT_DOWN);
    drag_x = x; drag_y = y;
}

static void motion(int x, int y) {
    if (!dragging) return;
    if (free_cam) {
        free_yaw   = fmodf(free_yaw + (x - drag_x) * FREE_LOOK, 360.0f);
        free_pitch = clampf(free_pitch - (y - drag_y) * FREE_LOOK,
                            -FREE_PITCH_MAX, FREE_PITCH_MAX);
    }
    drag_x = x; drag_y = y;
}

// ---------------------------------------------------------------------------
// Startup checks.  These are the numbers PRD 10 records; asserting them here
// means a change to the press stack or the Geneva geometry cannot quietly
// invalidate the interlock.
// ---------------------------------------------------------------------------
// Checked in release too, not with assert: a layout that quietly stops holding
// under -DNDEBUG is exactly the failure the PRD calls invisible.  The full set
// lives in tests/mathcheck.cpp; these are the ones worth re-proving every run.
static int g_layout_fails = 0;
static void must(bool ok, const char* what) {
    if (!ok) { fprintf(stderr, "LAYOUT CHECK FAILED: %s\n", what); ++g_layout_fails; }
}

static void verify_layout() {
    must(ROD_L >= MIN_L_OVER_R * CRANK_R, "rod obliquity: L/r >= 2.5");
    must(fabsf(kin::ram_top(0.0f) - 4.00f) < 1e-4f, "s_max = 4.00");
    must(fabsf(kin::ram_top(PI)  - 3.50f) < 1e-4f, "s_min = 3.50");
    must(fabsf(kin::tail_x() + 2.283185f) < 1e-4f, "tail roller at -2.2832");
    must(fabsf(kin::gen_centre_dist() - 0.77782f) < 1e-4f, "Geneva c = 0.7778");
    must(fabsf(kin::gen_wheel_r() - 0.55f) < 1e-4f, "Geneva wheel r = 0.55");
    for (int i = 0; i < 5; ++i) {                            // pitch-radius sums
        const int j = kin::LAY.drivenBy[i];
        if (j < 0) continue;
        const float dx = kin::LAY.g[i].cx - kin::LAY.g[j].cx;
        const float dy = kin::LAY.g[i].cy - kin::LAY.g[j].cy;
        must(fabsf(sqrtf(dx*dx + dy*dy)
                   - (kin::LAY.g[i].r + kin::LAY.g[j].r)) < 1e-3f,
             "a mesh is not at its pitch-radius sum");
    }
    printf("layout %s:  G3 (%.3f, %.3f)  G4 (%.3f, %.3f)\n",
           g_layout_fails ? "FAILED" : "ok",
           kin::LAY.g[2].cx, kin::LAY.g[2].cy,
           kin::LAY.g[3].cx, kin::LAY.g[3].cy);
    printf("blank zone: %.2f to %.2f deg   index: %.0f to %.0f deg\n",
           kin::blank_zone_lo_deg(), kin::blank_zone_hi_deg(),
           360.0f - kin::engage_half() * DEG, kin::engage_half() * DEG);
    printf("pitch %.6f   tail roller x %.6f   loop %.4f = %.1f pitches\n",
           kin::pitch(), kin::tail_x(), kin::loop_len(),
           kin::loop_len() / kin::pitch());
}

static void init() {
    glClearColor(0.09f, 0.10f, 0.12f, 1.0f);
    glEnable(GL_DEPTH_TEST);      // hidden-surface elimination, PRD FR-15
    glEnable(GL_CULL_FACE);       // and the other half of it
    glCullFace(GL_BACK);

    scene::build_lists();
    room::build_lists();        // after: it calls the machine's gear and blank lists
}

int main(int argc, char** argv) {
    verify_layout();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(win_w, win_h);
    glutCreateWindow("Quarter Turn - static scene, objects only");

    const GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "glewInit failed: %s\n", glewGetErrorString(err));
        return 1;
    }
    printf("GL %s\n", glGetString(GL_VERSION));

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutSpecialUpFunc(special_up);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(idle);
    glutMainLoop();
    return 0;
}
