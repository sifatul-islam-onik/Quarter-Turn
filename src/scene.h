// scene.h - geometry for "Quarter Turn".
//
// Static geometry is compiled into display lists once (PRD FR-15).  Nothing is
// modelled twice: one tooth list serves all 124 teeth of the five gears, one
// cleat list is instanced 24 times, one blank list up to ten times, and the box
// and cylinder routines in prim.h generate every other part at its true size.
//
// Hierarchy (PRD FR-9), all with glPushMatrix/glPopMatrix only:
//   A: floor -> panel -> crankshaft -> disc -> pin -> rod -> ram -> punch
//   B: floor -> panel -> guide rails -> ram
//   C: floor -> panel -> Geneva shaft -> driver arm -> pin
//   D: floor -> conveyor frame -> head roller -> Geneva wheel,
//      with branches  frame -> belt path -> cleat k
//      and            frame -> station j -> blank -> squash
// A and B meet at the ram and C and D meet at the pin in the slot, so the scene
// is two closed kinematic loops rather than a tree.  Both close analytically -
// the first by s(theta), the second by beta(alpha) - so both are free.
//
// Meshing is deliberately NOT parenting: a child inherits its parent's
// rotation, but a meshing gear turns the other way at a different rate, so G3
// as a child of G2 would have to undo G2's rotation first.  Every gear is a
// child of the drive panel and the mesh law is a constraint between siblings.
#ifndef SCENE_H
#define SCENE_H

#include "prim.h"
#include "layout.h"
#include "materials.h"

namespace scene {

using namespace cfg;
using namespace prim;
using lay::LAY;

// Flat colours (materials.h).  Each is a compile-time constant, so baking one
// into a display list is correct; the stack light's colour is the only one in
// the machine that changes at runtime, and it is set outside its list.
inline void c_brass()   { mat::use(mat::BRASS);         }
inline void c_silver()  { mat::use(mat::SILVER);        }
inline void c_steel()   { mat::use(mat::DARK_STEEL);    }
inline void c_plastic() { mat::use(mat::BLACK_PLASTIC); }
inline void c_paint()   { mat::use(mat::MACHINE_PAINT); }
inline void c_rubber()  { mat::use(mat::RUBBER);        }
inline void c_concrete(){ mat::use(mat::CONCRETE);      }

// ---------------------------------------------------------------------------
// Display lists
// ---------------------------------------------------------------------------
enum {
    L_FLOOR = 0, L_PANEL, L_CONVEYOR, L_BELT, L_FIXTURES, L_MOTOR,
    L_TOOTH, L_CLEAT, L_BLANK, L_ROLLER, L_GENEVA, L_STACK_SEG,
    L_GEAR0, L_COUNT = L_GEAR0 + 5
};
inline GLuint g_list = 0;
inline GLuint L(int i) { return g_list + (GLuint)i; }

inline float gear_root_r(int i) { return LAY.g[i].r - TOOTH_DEDENDUM * MODULE; }
inline float gear_tip_r (int i) { return LAY.g[i].r + TOOTH_ADDENDUM * MODULE; }

// --- the Geneva wheel -------------------------------------------------------
// Built as a hub disc plus four arms.  Each arm is one extruded strip whose
// outer boundary is the rim and whose inner boundary is
//     r_in(u) = max(hub, hw/sin u, hw/cos u),
// i.e. the hub arc in the middle of the arm and, near either end, the straight
// wall of the neighbouring slot - exactly straight, because r = hw/sin u is the
// polar form of the line y = hw.  Where r_in reaches the rim the arm's end cap
// collapses, so the four arms tile the disc with four clean slots between them.
//
// Deviation from the PRD, stated rather than hidden: the PRD puts the slot
// bottom at 0.20 because the pin's centre reaches 0.2278.  A pin of radius
// 0.045 reaches 0.183, so the slot is cut to GEN_HUB_R = 0.175 and the pin
// clears the bottom by 0.008 instead of bottoming out at the deepest point.
inline void build_geneva_wheel() {
    const float R  = lay::gen_wheel_r();
    const float hw = GEN_SLOT_HW;
    const float u0 = asinf(hw / R);            // arm end, where r_in meets rim
    const float uc = asinf(hw / GEN_HUB_R);    // wall meets hub arc
    const float su = 2.0f * PI / GEN_SLOTS;    // one slot pitch

    // Sample the arm's inner boundary: the straight wall of the slot behind it,
    // the hub arc across the middle, then the straight wall of the slot ahead.
    // The corners at uc and su-uc are sampled exactly so the walls stay sharp.
    float us[48];
    int n = 0;
    const int NA = 4, NB = 16;
    for (int i = 0; i <= NA; ++i) us[n++] = u0 + (uc - u0) * i / NA;
    for (int i = 1; i <= NB; ++i) us[n++] = uc + (su - 2.0f * uc) * i / NB;
    for (int i = 1; i <= NA; ++i) us[n++] = (su - uc) + (uc - u0) * i / NA;

    V2 in[48], out[48];
    glNewList(L(L_GENEVA), GL_COMPILE);
    c_brass();
    glPushMatrix();
    glTranslatef(0, 0, GEN_WHEEL_Z0);
    cyl_z(GEN_HUB_R, GEN_WHEEL_T, 24);          // hub disc closes the slots
    glPopMatrix();
    for (int k = 0; k < GEN_SLOTS; ++k) {
        const float base = su * k;
        for (int i = 0; i < n; ++i) {
            const float u = us[i];
            float ri = GEN_HUB_R;
            const float a = hw / sinf(u), b = hw / cosf(u);
            if (a > ri) ri = a;
            if (b > ri) ri = b;
            if (ri > R) ri = R;
            const float ph = base + u;
            in [i].x = ri * cosf(ph); in [i].y = ri * sinf(ph);
            out[i].x = R  * cosf(ph); out[i].y = R  * sinf(ph);
        }
        extrude_strip(in, out, n, GEN_WHEEL_Z0, GEN_WHEEL_Z0 + GEN_WHEEL_T);
    }
    glEndList();
}

// --- the belt's two half-shells --------------------------------------------
inline void arc_shell(float cx, float cy, float a0, float a1, int segs) {
    V2 in[64], out[64];
    const int n = segs + 1;
    for (int i = 0; i <= segs; ++i) {
        const float a = a0 + (a1 - a0) * i / segs;
        in [i].x = cx + ROLLER_R * cosf(a); in [i].y = cy + ROLLER_R * sinf(a);
        out[i].x = cx + BELT_R_O * cosf(a); out[i].y = cy + BELT_R_O * sinf(a);
    }
    extrude_strip(in, out, n, -0.5f * BELT_W, 0.5f * BELT_W);
}

inline void build_lists() {
    g_list = glGenLists(L_COUNT);

    const float xt = lay::tail_x();

    // ---- floor (PRD 4.2 row 1) --------------------------------------------
    glNewList(L(L_FLOOR), GL_COMPILE);
    c_concrete();
    grid_xz(FLOOR_X0, FLOOR_X1, FLOOR_Z0, FLOOR_Z1, FLOOR_NX, FLOOR_NZ, 0.0f);
    glEndList();

    // ---- drive panel, guide rails, rail brackets (row 2) -------------------
    glNewList(L(L_PANEL), GL_COMPILE);
    c_paint();
    glPushMatrix();
    glTranslatef(PANEL_CX, PANEL_CY, PANEL_CZ);
    plate_z(PANEL_W, PANEL_H, PANEL_D, PANEL_NX, PANEL_NY);
    glPopMatrix();
    for (int s = -1; s <= 1; s += 2) {
        const float x = PRESS_X + s * RAIL_X_OFF;
        // guide rail: holds the ram over its whole travel, 3.10 -> 4.00
        box_span(x - 0.5f*RAIL_W, RAIL_Y0, -0.5f*RAIL_D,
                 x + 0.5f*RAIL_W, RAIL_Y1,  0.5f*RAIL_D);
        // bracket back to the panel, passing outside the ram in x and under
        // G2's lowest tooth tip (3.80) in y
        box_span(x - 0.5f*RAIL_W, BRACKET_Y0, PANEL_CZ + 0.5f*PANEL_D,
                 x + 0.5f*RAIL_W, BRACKET_Y1, -0.5f*RAIL_D);
    }
    glEndList();

    // ---- motor body and mount (row 3) -------------------------------------
    glNewList(L(L_MOTOR), GL_COMPILE);
    c_plastic();
    glPushMatrix();
    glTranslatef(G1_X, G1_Y, MOTOR_Z0);
    cyl_z(MOTOR_R, MOTOR_Z1 - MOTOR_Z0, 20);
    glPopMatrix();
    c_paint();
    box_span(G1_X - 0.25f, PANEL_CY + 0.5f*PANEL_H, MOTOR_Z0,
             G1_X + 0.25f, MOTOR_Z1 + 0.55f,        PANEL_CZ + 0.5f*PANEL_D);
    c_silver();
    glPushMatrix();                                  // stub into the pinion
    glTranslatef(G1_X, G1_Y, MOTOR_Z1);
    cyl_z(MOTOR_SHAFT_R, 0.14f, 12, PART_CHAMFER);
    glPopMatrix();
    glEndList();

    // ---- conveyor frame and legs (row 10) ---------------------------------
    glNewList(L(L_CONVEYOR), GL_COMPILE);
    c_paint();
    const float fx0 = 0.5f * (xt + HEAD_X) - 0.5f * CFRAME_LEN;
    const float fx1 = fx0 + CFRAME_LEN;
    for (int s = -1; s <= 1; s += 2) {
        const float z = s * CFRAME_Z;
        box_span(fx0, CFRAME_Y - 0.5f*CFRAME_H, z - 0.5f*CFRAME_W,
                 fx1, CFRAME_Y + 0.5f*CFRAME_H, z + 0.5f*CFRAME_W);
        for (int e = 0; e < 2; ++e) {
            const float lx = e ? LEG_X1 : LEG_X0;
            box_span(lx - 0.5f*LEG_W, 0.0f, z - 0.5f*CFRAME_W,
                     lx + 0.5f*LEG_W, CFRAME_Y - 0.5f*CFRAME_H, z + 0.5f*CFRAME_W);
        }
    }
    glEndList();

    // ---- belt strips and half-shells (row 11) -----------------------------
    // The strips are static on purpose.  Without textures a uniform strip that
    // moves looks identical to one that does not, so all of the belt's visible
    // motion is carried by the 24 cleats (PRD FR-6).
    glNewList(L(L_BELT), GL_COMPILE);
    c_rubber();
    slab_x(xt, HEAD_X, BELT_TOP_Y - BELT_T, BELT_TOP_Y,
           -0.5f*BELT_W, 0.5f*BELT_W, BELT_TOP_SEGS);
    box_span(xt, ROLLER_Y - BELT_R_O, -0.5f*BELT_W,
             HEAD_X, ROLLER_Y - ROLLER_R, 0.5f*BELT_W);
    arc_shell(HEAD_X, ROLLER_Y, -0.5f*PI,  0.5f*PI, SHELL_SEGS);
    arc_shell(xt,     ROLLER_Y,  0.5f*PI,  1.5f*PI, SHELL_SEGS);
    glEndList();

    // ---- magazine, exit hood, chute, bin (row 13) -------------------------
    glNewList(L(L_FIXTURES), GL_COMPILE);
    c_plastic();
    {   // feed magazine: a square tube whose lower edge sits above a blank top
        const float mx = lay::station_x(1);
        const float o = 0.5f * MAG_W, i = o - MAG_WALL;
        box_span(mx - o, MAG_Y0, -o, mx - i, MAG_Y1, o);
        box_span(mx + i, MAG_Y0, -o, mx + o, MAG_Y1, o);
        box_span(mx - i, MAG_Y0, -o, mx + i, MAG_Y1, -i);
        box_span(mx - i, MAG_Y0,  i, mx + i, MAG_Y1,  o);
    }
    {   // exit hood: a top and two sides resting on the belt edges.  The far
        // end is open, so a finished part can leave over the head roller
        box_span(HOOD_X0, HOOD_Y0, -HOOD_Z_OUT,
                 HOOD_X1, HOOD_Y0 + HOOD_TOP_T, HOOD_Z_OUT);
        for (int s = -1; s <= 1; s += 2) {
            const float z0 = s > 0 ? HOOD_Z_IN : -HOOD_Z_OUT;
            const float z1 = s > 0 ? HOOD_Z_OUT : -HOOD_Z_IN;
            box_span(HOOD_X0, BELT_TOP_Y, z0, HOOD_X1, HOOD_Y0, z1);
        }
    }
    c_paint();
    {   // chute down to the bin
        const float dx = CHUTE_X1 - CHUTE_X0, dy = CHUTE_Y1 - CHUTE_Y0;
        glPushMatrix();
        glTranslatef(0.5f*(CHUTE_X0+CHUTE_X1), 0.5f*(CHUTE_Y0+CHUTE_Y1), CHUTE_ZC);
        glRotatef(atan2f(dy, dx) * DEG, 0, 0, 1);
        box(sqrtf(dx*dx + dy*dy), CHUTE_T, CHUTE_Z);
        glPopMatrix();
    }
    {   // bin
        box_span(BIN_X0, 0.0f, -BIN_Z, BIN_X1, BIN_WALL, BIN_Z);
        box_span(BIN_X0, BIN_WALL, -BIN_Z, BIN_X0 + BIN_WALL, BIN_H, BIN_Z);
        box_span(BIN_X1 - BIN_WALL, BIN_WALL, -BIN_Z, BIN_X1, BIN_H, BIN_Z);
        box_span(BIN_X0 + BIN_WALL, BIN_WALL, -BIN_Z,
                 BIN_X1 - BIN_WALL, BIN_H, -BIN_Z + BIN_WALL);
        box_span(BIN_X0 + BIN_WALL, BIN_WALL, BIN_Z - BIN_WALL,
                 BIN_X1 - BIN_WALL, BIN_H, BIN_Z);
    }
    glEndList();

    // ---- one tooth, drawn 124 times (PRD FR-3, FR-15) ---------------------
    // Generated at the origin so it is gear-independent: the caller rotates to
    // the tooth's angle and translates out to that gear's root radius.
    glNewList(L(L_TOOTH), GL_COMPILE);
    {
        const float radial = (TOOTH_ADDENDUM + TOOTH_DEDENDUM) * MODULE;
        const float w = TOOTH_W_FRAC * PI * MODULE;
        glPushMatrix();
        glTranslatef(0.5f * radial, 0.0f, 0.0f);
        box(radial, w, GEAR_T);
        glPopMatrix();
    }
    glEndList();

    // ---- five gear bodies --------------------------------------------------
    for (int i = 0; i < 5; ++i) {
        glNewList(L(L_GEAR0 + i), GL_COMPILE);
        glPushMatrix();
        glTranslatef(0, 0, -0.5f * GEAR_T);
        cyl_z(gear_root_r(i), GEAR_T, GEAR_SLICES);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0, 0, -0.5f * HUB_T);
        cyl_z(HUB_R_FRAC * gear_root_r(i), HUB_T, 16);
        glPopMatrix();
        glEndList();
    }

    // ---- cleat, blank, roller, stack-light segment ------------------------
    glNewList(L(L_CLEAT), GL_COMPILE);
    glPushMatrix();
    glTranslatef(0.0f, 0.5f * CLEAT_H, 0.0f);   // base on the belt surface
    box(CLEAT_W, CLEAT_H, CLEAT_LEN);
    glPopMatrix();
    glEndList();

    glNewList(L(L_BLANK), GL_COMPILE);          // rises from y = 0 so FR-7 can
    cyl(BLANK_R, BLANK_H, BLANK_SLICES, BLANK_CHAMFER);   // about its base
    glEndList();

    glNewList(L(L_ROLLER), GL_COMPILE);
    glPushMatrix();
    glTranslatef(0, 0, -0.5f * ROLLER_LEN);
    cyl_z(ROLLER_R, ROLLER_LEN, 24, PART_CHAMFER);
    glPopMatrix();
    // a key across the front end cap, so the roller's quarter turn is legible
    // on the half of it the belt does not wrap
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.5f * ROLLER_LEN + 0.005f);
    box(2.0f * ROLLER_R * 0.8f, 0.06f, 0.01f);
    glPopMatrix();
    glEndList();

    glNewList(L(L_STACK_SEG), GL_COMPILE);
    cyl(STACK_R, STACK_SEG_H, 14);
    glEndList();

    build_geneva_wheel();
}

// ---------------------------------------------------------------------------
// Drawing.  Nothing below takes an angle or a time: every transformation is
// built from layout.h, which is fixed at startup, so the same picture is
// drawn every frame.
// ---------------------------------------------------------------------------
// Brass and copper alternate along the train (G1-G2, G2-G3, G3-G4, G4-G5), so
// the two gears at every mesh are different colours.
inline void draw_gears() {
    for (int i = 0; i < 5; ++i) {
        mat::use(i == 1 || i == 3 ? mat::COPPER : mat::BRASS);
        glPushMatrix();
        glTranslatef(LAY.g[i].cx, LAY.g[i].cy, GEAR_Z);
        glRotatef(LAY.phi[i] * DEG, 0, 0, 1);
        glCallList(L(L_GEAR0 + i));
        const float rr = gear_root_r(i);
        for (int k = 0; k < LAY.g[i].N; ++k) {      // 124 instances in total
            glPushMatrix();
            glRotatef(360.0f * k / LAY.g[i].N, 0, 0, 1);
            glTranslatef(rr, 0.0f, 0.0f);
            glCallList(L(L_TOOTH));
            glPopMatrix();
        }
        glPopMatrix();
    }
}

// Chain A: crankshaft -> disc -> pin -> rod -> ram.  Four levels of
// glPushMatrix, each one a child of the level above; the disc's own frame is
// unrotated in this pose, which is what puts the pin at 3 o'clock.
inline void draw_press() {
    const float px = lay::PIN_X, py = lay::PIN_Y;
    const float s  = lay::ram_top();

    c_silver();
    glPushMatrix();
    glTranslatef(PRESS_X, CRANK_Y, 0.0f);
    glRotatef(lay::CRANK_DEG, 0, 0, 1);
    glPushMatrix();
    glTranslatef(0, 0, PANEL_CZ + 0.5f * PANEL_D);
    cyl_z(CRANK_SHAFT_R, CRANK_DISC_Z0 - (PANEL_CZ + 0.5f*PANEL_D), 14,
          PART_CHAMFER);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, 0, CRANK_DISC_Z0);
    cyl_z(CRANK_DISC_R, CRANK_DISC_T, 20, PART_CHAMFER);
    glPopMatrix();
    glPushMatrix();                                  // crank pin
    glTranslatef(CRANK_R, 0.0f, CRANK_DISC_Z0 + CRANK_DISC_T);
    cyl_z(CRANK_PIN_R, 0.11f, 12, 0.015f);
    glPopMatrix();
    glPopMatrix();

    // connecting rod - placed from both of its endpoints, which is a composite
    // transformation in its own right.  Its length is exactly ROD_L because
    // that is what ram_top() solves for.
    c_steel();
    glPushMatrix();
    glTranslatef(px, py, 0.0f);
    glRotatef(lay::rod_deg(), 0, 0, 1);
    glTranslatef(0.5f * ROD_L, 0.0f, 0.0f);
    box(ROD_L, ROD_W, ROD_D);
    glPopMatrix();

    c_silver();
    glPushMatrix();                                  // wrist pin
    glTranslatef(PRESS_X, s, -0.06f);
    cyl_z(0.06f, 0.12f, 12, 0.015f);
    glPopMatrix();

    c_steel();
    glPushMatrix();                                  // ram and punch
    glTranslatef(PRESS_X, s - 0.5f * RAM_H, 0.0f);
    box(RAM_W, RAM_H, RAM_D);
    glPopMatrix();
}

// Chain C: Geneva shaft -> driver arm -> pin.  The arm is keyed to G5's shaft;
// where that leaves it in this pose is lay::ARM_DEG.
inline void draw_geneva_driver() {
    c_silver();
    glPushMatrix();
    glTranslatef(G5_X, G5_Y, GEN_SHAFT_Z0);
    cyl_z(GEN_SHAFT_R, GEN_SHAFT_Z1 - GEN_SHAFT_Z0, 12, 0.012f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(G5_X, G5_Y, 0.0f);
    glRotatef(lay::ARM_DEG, 0, 0, 1);
    box_span(-GEN_ARM_W, -0.5f*GEN_ARM_W, GEN_ARM_Z0,
             GEN_A + GEN_ARM_W, 0.5f*GEN_ARM_W, GEN_ARM_Z0 + GEN_ARM_T);
    glPushMatrix();
    glTranslatef(GEN_A, 0.0f, GEN_PIN_Z0);
    cyl_z(GEN_PIN_R, GEN_PIN_Z1 - GEN_PIN_Z0, 12, 0.012f);
    glPopMatrix();
    glPopMatrix();
}

// Chain D: conveyor frame -> rollers -> Geneva wheel, and the belt branches.
inline void draw_conveyor() {
    const float wa = lay::WHEEL_DEG;
    const float xt = lay::tail_x();

    c_silver();
    for (int e = 0; e < 2; ++e) {
        glPushMatrix();
        glTranslatef(e ? HEAD_X : xt, ROLLER_Y, 0.0f);
        glRotatef(wa, 0, 0, 1);
        glCallList(L(L_ROLLER));
        glPopMatrix();
    }
    glPushMatrix();                                  // stub through the frame
    glTranslatef(HEAD_X, ROLLER_Y, 0.5f * ROLLER_LEN);
    glRotatef(wa, 0, 0, 1);
    cyl_z(GEN_STUB_R, GEN_WHEEL_Z0 - 0.5f * ROLLER_LEN, 12, PART_CHAMFER);
    glPopMatrix();

    glPushMatrix();                                  // the wheel itself
    glTranslatef(HEAD_X, ROLLER_Y, 0.0f);
    glRotatef(wa, 0, 0, 1);
    glCallList(L(L_GENEVA));
    glPopMatrix();

    mat::use(mat::SAFETY_YELLOW);                    // 24 cleats on the loop
    for (int k = 0; k < CLEAT_N; ++k) {
        const lay::PathPt q = lay::belt_path(lay::cleat_s(k));
        glPushMatrix();
        glTranslatef(q.x, q.y, 0.0f);
        glRotatef(q.rot_deg, 0, 0, 1);
        glCallList(L(L_CLEAT));
        glPopMatrix();
    }
}

// The scene's only glScalef (PRD 4.3).  Applied about the blank's base, so a
// flattened part stays on the belt instead of being lifted off it; scaling
// about the centre would do the opposite.  (x, y, z) is the base centre.
inline void draw_blank(float x, float y, float h, float z = 0.0f) {
    const float q = lay::squash_q(h);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(1.0f / sqrtf(q), q, 1.0f / sqrtf(q));
    glCallList(L(L_BLANK));
    glPopMatrix();
}

// The nine blanks, one on each station: full height up to the press at
// station 7, flat past it.  The belt is parked, so each one sits square on its
// own station.
inline void draw_blanks() {
    c_silver();
    for (int j = 1; j <= LABELS; ++j)
        draw_blank(lay::station_x(j), BELT_TOP_Y, lay::blank_h(j));
}

inline void draw_stack_light() {
    c_paint();
    glPushMatrix();
    glTranslatef(STACK_X, 0.0f, STACK_Z);
    cyl(STACK_POST_R, STACK_POST_H, 10);
    glPopMatrix();
    c_plastic();
    glPushMatrix();
    glTranslatef(STACK_X, STACK_POST_H, STACK_Z);
    cyl(STACK_R, STACK_Y0 - STACK_POST_H, 14);
    glPopMatrix();

    // green, amber, red upward, the lit one in its full colour and the other
    // two dimmed.  The three lenses share one display list and differ only in
    // the colour set immediately before each call: a list captures the values
    // passed to glColor when it was compiled, not a reference to them, so the
    // colour cannot be baked into the list (PRD FR-15).
    const float base[3][3] = {{0.10f, 0.85f, 0.20f},
                              {0.95f, 0.65f, 0.05f},
                              {0.90f, 0.12f, 0.10f}};
    for (int i = 0; i < 3; ++i) {
        mat::use(mat::lens(base[i][0], base[i][1], base[i][2],
                           i == lay::STACK_LIT));
        glPushMatrix();
        glTranslatef(STACK_X, STACK_Y0 + i * STACK_SEG_H, STACK_Z);
        glCallList(L(L_STACK_SEG));
        glPopMatrix();
    }
}

inline void draw() {
    // Each list carries its own colour, which is safe because none of them
    // change; the stack light's lenses are the exception, and their colour is
    // set outside the list below.
    glCallList(L(L_FLOOR));
    glCallList(L(L_PANEL));
    glCallList(L(L_CONVEYOR));
    glCallList(L(L_FIXTURES));
    glCallList(L(L_MOTOR));
    glCallList(L(L_BELT));

    // The four transformation chains, each built fresh from layout.h.  They
    // are not folded into the lists above because they are what the hierarchy
    // demonstration points at (PRD FR-15).
    draw_gears();
    draw_press();
    draw_geneva_driver();
    draw_conveyor();
    draw_blanks();
    draw_stack_light();
}

} // namespace scene
#endif
