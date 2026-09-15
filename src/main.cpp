// "Quarter Turn" - an automated stamping line in OpenGL.
//
// Milestone 2: geometry and motion.  Every object in PRD 4.2 is modelled and
// every mechanism in FR-3 to FR-7 runs.  Lighting, materials and the three
// shading modes (FR-10, FR-11, FR-12) are the next milestone; the placeholder
// light set up in init() is scaffolding and is marked as such.
//
// GLEW must be included before freeglut, and glewInit() must run after
// glutCreateWindow() because it needs a live context (PRD 8).
#include <GL/glew.h>
#include <GL/freeglut.h>

#include <cstdlib>
#include <cmath>
#include <cstdio>

#include "config.h"
#include "kinematics.h"
#include "scene.h"

using namespace cfg;

// ---------------------------------------------------------------------------
// Animation state.  PRD FR-2: exactly two variables.  Everything else visible
// in the scene is a pure function of these two plus the constants in config.h.
// ---------------------------------------------------------------------------
static float theta  = RESET_THETA_DEG * RAD;   // crankshaft angle, [0, 2pi)
static long  cycles = 0;                       // monotonic, one per part

static float ppm     = PPM_DEFAULT;            // one revolution is one part
static bool  running = true;
static bool  wireframe = false;
static float frame_ms = 0.0f;

// Camera
static int   preset = 1;
static float orbit  = 0.0f;                    // degrees about the look-at
static int   win_w = 1180, win_h = 700;

// Live check of the PRD 7 acceptance criterion: belt travel must be strictly
// constant in every frame in which the punch face is below the top of an
// unstamped blank.  Reported on the HUD rather than hidden in an assert, so it
// is checkable during the demo itself.
static float last_B = 0.0f;
static long  interlock_faults = 0;

// ---------------------------------------------------------------------------
// FR-1 - frame-rate-independent timing.  No motion uses a per-frame constant.
// ---------------------------------------------------------------------------
static void update(float dt) {
    if (!running) return;
    const float omega = ppm * 2.0f * PI / 60.0f;      // rad/s
    theta += omega * dt;
    // A while, not an if: `cycles` must increment exactly once per revolution
    // because belt travel depends on it, and that stays true if PPM_MAX is
    // ever raised (PRD FR-2).
    while (theta >= 2.0f * PI) { theta -= 2.0f * PI; ++cycles; }
}

static void check_interlock() {
    const float B = kin::belt_travel(theta, cycles);
    if (kin::punch_face(theta) < BELT_TOP_Y + BLANK_H && B != last_B)
        ++interlock_faults;
    last_B = B;
}

static void step_theta(float deg) {
    theta += deg * RAD;
    while (theta >= 2.0f * PI) { theta -= 2.0f * PI; ++cycles; }
}

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------
static void apply_camera() {
    float ex, ey, ez, ax, ay, az, ux = 0, uy = 1, uz = 0;
    switch (preset) {
    case 2:                                     // front elevation
        ex = 1.8f; ey = 3.2f; ez = 12.0f; ax = 1.8f; ay = 3.2f; az = 0.0f;
        break;
    case 3:                                     // top plan
        // 12.0 is as low as this can go: the stack light's top at 6.30 must
        // stay outside the 4.0 near plane, and the floor is 12.0 wide.
        ex = AT_X; ey = 12.0f; ez = AT_Z; ax = AT_X; ay = AT_Y; az = AT_Z;
        ux = 0; uy = 0; uz = -1;
        break;
    default:                                    // three-quarter, front right
        ex = EYE_X; ey = EYE_Y; ez = EYE_Z; ax = AT_X; ay = AT_Y; az = AT_Z;
        break;
    }
    const float c = cosf(orbit * RAD), s = sinf(orbit * RAD);
    const float dx = ex - ax, dz = ez - az;
    gluLookAt(ax + dx * c + dz * s, ey, az - dx * s + dz * c,
              ax, ay, az, ux, uy, uz);
}

// ---------------------------------------------------------------------------
// FR-14 - on-screen indications.  The five steps below are each load-bearing:
// skipping any one produces text that is invisible, black, or behind the
// machine, and all three failures look as if it was never drawn.
// ---------------------------------------------------------------------------
static void text(float x, float y, const char* s) {
    glRasterPos2f(x, y);
    while (*s) glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
}

static void hud() {
    if (GLEW_VERSION_2_0) glUseProgram(0);      // 1. no shader on the glyphs
    glDisable(GL_LIGHTING);                     // 2. glColor3f, not a material
    glDisable(GL_DEPTH_TEST);                   // 3. never behind the machine
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, win_w, 0, win_h);             // 4. raster pos in screen space
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();

    const float d  = theta * DEG;
    const kin::Phase ph = kin::phase_of(theta);
    const float B  = kin::belt_travel(theta, cycles);
    const float pf = kin::punch_face(theta);
    const float h7 = kin::press_blank_h(theta);
    const long  parts = cycles + (d >= 180.0f ? 1 : 0);

    char b[160];
    float y = (float)win_h - 22.0f;
    glColor3f(0.95f, 0.95f, 0.90f);
    snprintf(b, sizeof b, "QUARTER TURN   theta %6.1f deg   %-8s  %s",
             d, kin::phase_name(ph),
             kin::belt_locked(ph) ? "BELT LOCKED (punch is down)" : "BELT MOVING");
    text(14, y, b); y -= 18;

    glColor3f(0.80f, 0.86f, 0.95f);
    snprintf(b, sizeof b, "punch face %.3f   blank top %.3f   clearance %+.3f",
             pf, BELT_TOP_Y + h7, pf - (BELT_TOP_Y + h7));
    text(14, y, b); y -= 18;
    snprintf(b, sizeof b, "Geneva wheel %7.2f deg   belt travel %.4f stations"
                          "   index %.3f",
             kin::wheel_angle_deg(B), B, kin::index_progress(theta));
    text(14, y, b); y -= 18;
    snprintf(b, sizeof b, "parts made %ld   speed %.0f ppm   motor %.0f rpm",
             parts, ppm, ppm * 3.0f);
    text(14, y, b); y -= 18;
    snprintf(b, sizeof b, "frame %5.2f ms   %5.1f fps   gears freeze near"
                          " %.0f ppm",
             frame_ms, frame_ms > 0.0f ? 1000.0f / frame_ms : 0.0f,
             frame_ms > 0.0f ? (1000.0f / frame_ms) / 0.6f : 0.0f);
    text(14, y, b); y -= 18;

    if (interlock_faults) {
        glColor3f(1.0f, 0.3f, 0.2f);
        snprintf(b, sizeof b, "INTERLOCK FAULT x%ld - belt moved under the"
                              " punch", interlock_faults);
        text(14, y, b);
    } else {
        glColor3f(0.45f, 0.75f, 0.45f);
        text(14, y, "interlock ok - belt travel constant whenever the punch"
                    " is in the blank zone");
    }

    glColor3f(0.60f, 0.60f, 0.62f);
    text(14, 14, "SPACE run/stop   . step 5deg   UP/DOWN speed   LEFT/RIGHT"
                 " orbit   1 2 3 views   w wireframe   r reset   ESC quit");

    glPopMatrix();                              // 5. restore both matrices
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

// ---------------------------------------------------------------------------
// GLUT callbacks
// ---------------------------------------------------------------------------
static void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    apply_camera();

    // FR-10 will set the two lights here, after the camera transform and
    // before any model transform, in one function called from this one place.

    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    scene::draw(theta, cycles);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    hud();
    glutSwapBuffers();
}

static void reshape(int w, int h) {
    win_w = w; win_h = h < 1 ? 1 : h;
    glViewport(0, 0, w, win_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOVY, (double)w / win_h, ZNEAR, ZFAR);
    glMatrixMode(GL_MODELVIEW);
}

static void idle() {
    static int prev = 0;
    const int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = prev ? (now - prev) * 0.001f : 0.0f;   // first frame: dt = 0
    prev = now;
    frame_ms = dt * 1000.0f;
    if (dt > DT_CLAMP) dt = DT_CLAMP;   // a window drag must not skip a phase
    update(dt);
    check_interlock();
    glutPostRedisplay();
}

static void keyboard(unsigned char k, int, int) {
    switch (k) {
    case 27: glutLeaveMainLoop(); break;
    case ' ': running = !running; break;
    case '.': if (!running) step_theta(STEP_DEG); break;   // forward only
    case 'w': case 'W': wireframe = !wireframe; break;
    case '1': preset = 1; orbit = 0.0f; break;
    case '2': preset = 2; orbit = 0.0f; break;
    case '3': preset = 3; orbit = 0.0f; break;
    case 'r': case 'R':
        theta = RESET_THETA_DEG * RAD; cycles = 0;
        interlock_faults = 0; last_B = kin::belt_travel(theta, cycles);
        break;
    default: break;
    }
    // 's' shading mode, 'l' fill light and 'n' GL_NORMALIZE arrive with FR-10.
}

static void special(int k, int, int) {
    switch (k) {
    case GLUT_KEY_UP:    ppm = fminf(PPM_MAX, ppm + PPM_STEP); break;
    case GLUT_KEY_DOWN:  ppm = fmaxf(PPM_MIN, ppm - PPM_STEP); break;
    case GLUT_KEY_LEFT:  orbit -= 3.0f; break;
    case GLUT_KEY_RIGHT: orbit += 3.0f; break;
    default: break;
    }
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
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);       // FR-7: the blank squash is a non-uniform scale

    // ---- SCAFFOLD -----------------------------------------------------------
    // A single default headlight so the models are readable while the geometry
    // is being built.  PRD FR-10's two-light rig and FR-11's six materials
    // replace this entire block in the next milestone; nothing else in the
    // project depends on it.
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    const GLfloat pos[4] = { 0.35f, 0.75f, 0.55f, 0.0f };
    const GLfloat dif[4] = { 0.95f, 0.95f, 0.95f, 1.0f };
    const GLfloat amb[4] = { 0.30f, 0.30f, 0.34f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
    // ---- end SCAFFOLD -------------------------------------------------------

    scene::build_lists();
    last_B = kin::belt_travel(theta, cycles);
}

int main(int argc, char** argv) {
    verify_layout();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(win_w, win_h);
    glutCreateWindow("Quarter Turn - an automated stamping line");

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
    glutIdleFunc(idle);
    glutMainLoop();
    return 0;
}
