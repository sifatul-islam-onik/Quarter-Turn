// room.h - the building around the line: four walls, a beamed ceiling, the two
// hanging bulbs that light everything, and what stands on the floor.  Not in
// the PRD, which says "no factory building"; the deviation is in the README.
//
// The machine does not know the room exists.  Nothing here feeds kinematics.
// The parts counter is a closed-form function of (theta, cycles) like the
// machine; the exhaust fan is not, because it has its own switch, so its angle
// is the one piece of animation state outside FR-2's two variables.
//
// The room is a cutaway.  A wall and everything mounted on it is drawn only
// while the eye is on the room side of that wall's plane, so whichever walls
// stand between the camera and the machine vanish as the camera orbits and
// the far ones stay.  Back-face culling alone would hide the bare wall
// surfaces, which face inward, but not the boxes fixed to them: from outside,
// a window frame or a door shutter would float in front of the machine.
//
// Each wall is built in its own frame: u runs along the wall left to right as
// seen from inside the room, y is up, and z points out of the wall into the
// room.  One routine then puts a window on any of the four.
#ifndef ROOM_H
#define ROOM_H

#include "scene.h"

namespace room {

using namespace cfg;
using namespace prim;

enum Wall { BACK = 0, LEFT, RIGHT, FRONT };
enum { R_FLOOR_ITEMS = 0, R_WALL0, R_CEILING = R_WALL0 + 4, R_FAN,
       R_PENDANTS, R_GLOBE, R_COUNT };

// The cabinet's four switches, left to right, and the colour of the status
// lamp over each.  The HUD uses the same colours, so the two read alike.
enum Switch { SW_MACHINE = 0, SW_BULB_L, SW_BULB_R, SW_FAN, SW_COUNT };
constexpr float SWITCH_RGB[SW_COUNT][3] = {
    { 0.25f, 0.90f, 0.35f }, { 1.00f, 0.72f, 0.20f },
    { 1.00f, 0.72f, 0.20f }, { 0.30f, 0.70f, 1.00f } };
inline GLuint g_list = 0;
inline GLuint L(int i) { return g_list + (GLuint)i; }

// ---------------------------------------------------------------------------
// Wall frames
// ---------------------------------------------------------------------------
inline float wall_len(int w) {
    return (w == LEFT || w == RIGHT) ? ROOM_Z1 - ROOM_Z0 : ROOM_X1 - ROOM_X0;
}

// A turn of a about y sends local +x to (cos a, 0, -sin a) and local +z to
// (sin a, 0, cos a).  Each angle below sends +z to that wall's inward normal.
inline void enter_wall(int w) {
    switch (w) {
    case BACK:  glTranslatef(ROOM_X0, 0.0f, ROOM_Z0);                            break;
    case LEFT:  glTranslatef(ROOM_X0, 0.0f, ROOM_Z1); glRotatef( 90.0f, 0, 1, 0); break;
    case RIGHT: glTranslatef(ROOM_X1, 0.0f, ROOM_Z0); glRotatef(-90.0f, 0, 1, 0); break;
    default:    glTranslatef(ROOM_X1, 0.0f, ROOM_Z1); glRotatef(180.0f, 0, 1, 0); break;
    }
}

// A world coordinate along a wall, as that wall's u.
inline float back_u (float x) { return x - ROOM_X0; }
inline float left_u (float z) { return ROOM_Z1 - z; }
inline float right_u(float z) { return z - ROOM_Z0; }
inline float front_u(float x) { return ROOM_X1 - x; }

inline bool wall_shown(int w, const float* eye) {
    switch (w) {
    case BACK:  return eye[2] > ROOM_Z0;
    case LEFT:  return eye[0] > ROOM_X0;
    case RIGHT: return eye[0] < ROOM_X1;
    default:    return eye[2] < ROOM_Z1;
    }
}

// Cylinder along +u: cyl() runs along +Y, and -90 degrees about z maps +Y to +X.
inline void cyl_u(float r, float h, int slices) {
    glPushMatrix();
    glRotatef(-90.0f, 0, 0, 1);
    cyl(r, h, slices);
    glPopMatrix();
}

// ---------------------------------------------------------------------------
// Things that go on any wall
// ---------------------------------------------------------------------------

// Concrete plinth, painted block above, a yellow line where they meet.  The
// two bands share a plane without overlapping, so nothing can z-fight.
inline void wall_surface(float len) {
    const int nu = (int)(len / WALL_CELL + 0.5f);
    const int nw = (int)((CEIL_Y - DADO_H) / WALL_CELL + 0.5f);
    mat::use(mat::CONCRETE);
    grid(0, 0, 0,  len, 0, 0,  0, DADO_H, 0,  nu, 3);
    mat::use(mat::WALL_PAINT);
    grid(0, DADO_H, 0,  len, 0, 0,  0, CEIL_Y - DADO_H, 0,  nu, nw);
    mat::use(mat::SAFETY_YELLOW);
    box_span(0, DADO_H - 0.5f * TRIM_H, 0, len, DADO_H + 0.5f * TRIM_H, 0.02f);
}

// A glowing pane in a steel frame with a cross bar.  The transom is two halves
// either side of the mullion, so no two bars share a face.
inline void window(float uc) {
    const float u0 = uc - 0.5f * WIN_W, u1 = uc + 0.5f * WIN_W;
    const float y0 = WIN_Y0, y1 = WIN_Y0 + WIN_H, f = WIN_FRAME;
    const float b = 0.5f * WIN_BAR, ym = y0 + 0.5f * WIN_H, d = WIN_DEPTH;
    mat::use(mat::WINDOW_GLASS);
    grid(u0, y0, 0.01f,  WIN_W, 0, 0,  0, WIN_H, 0,  1, 1);
    mat::use(mat::GALVANIZED);
    box_span(u0 - f, y0 - f, 0, u1 + f, y0,     d);         // sill
    box_span(u0 - f, y1,     0, u1 + f, y1 + f, d);         // head
    box_span(u0 - f, y0,     0, u0,     y1,     d);         // jambs
    box_span(u1,     y0,     0, u1 + f, y1,     d);
    box_span(uc - b, y0,     0.01f, uc + b, y1,     0.5f * d);
    box_span(u0,     ym - b, 0.01f, uc - b, ym + b, 0.5f * d);
    box_span(uc + b, ym - b, 0.01f, u1,     ym + b, 0.5f * d);
}

// A spare gear standing on a shelf, on one tooth tip.  It is built from the
// train's own body and tooth lists, so it is exactly the part it would
// replace.  Both tooth counts used (12, 20) put a tooth at 270 degrees.
inline void spare_gear(int i, float u, float y, float z) {
    mat::use(mat::BRASS);
    glPushMatrix();
    glTranslatef(u, y + scene::gear_tip_r(i), z);
    glCallList(scene::L(scene::L_GEAR0 + i));
    for (int k = 0; k < kin::LAY.g[i].N; ++k) {
        glPushMatrix();
        glRotatef(360.0f * k / kin::LAY.g[i].N, 0, 0, 1);
        glTranslatef(scene::gear_root_r(i), 0.0f, 0.0f);
        glCallList(scene::L(scene::L_TOOTH));
        glPopMatrix();
    }
    glPopMatrix();
}

// ---------------------------------------------------------------------------
// Back wall: windows, exhaust fan housing, counter housing, pipe run
// ---------------------------------------------------------------------------
inline void back_wall_fixtures() {
    const float len = wall_len(BACK);
    for (float x : BWIN_X) window(back_u(x));

    {   // exhaust fan housing; the rotor is drawn per frame
        const float uc = back_u(FAN_X), h = 0.5f * FAN_SIZE, f = 0.08f, d = 0.28f;
        mat::use(mat::BLACK_PLASTIC);
        grid(uc - h + f, FAN_Y - h + f, 0.01f,
             FAN_SIZE - 2.0f * f, 0, 0,  0, FAN_SIZE - 2.0f * f, 0,  1, 1);
        glPushMatrix();                                  // motor behind the hub
        glTranslatef(uc, FAN_Y, 0.01f);
        cyl_z(0.10f, 0.12f, 16);
        glPopMatrix();
        mat::use(mat::GALVANIZED);
        box_span(uc - h,     FAN_Y - h,     0, uc + h,     FAN_Y - h + f, d);
        box_span(uc - h,     FAN_Y + h - f, 0, uc + h,     FAN_Y + h,     d);
        box_span(uc - h,     FAN_Y - h + f, 0, uc - h + f, FAN_Y + h - f, d);
        box_span(uc + h - f, FAN_Y - h + f, 0, uc + h,     FAN_Y + h - f, d);
    }

    {   // parts counter housing; the digits are drawn per frame
        const float uc = back_u(COUNTER_X);
        const float hw = 0.5f * COUNTER_DIGITS * DIG_PITCH + 0.10f;
        mat::use(mat::BLACK_PLASTIC);
        box_span(uc - hw, COUNTER_Y, 0, uc + hw, COUNTER_Y + DIG_H + 0.24f, 0.10f);
    }

    mat::use(mat::GALVANIZED);
    for (float y : PIPE_Y) {                             // pipe run under the beams
        glPushMatrix();
        glTranslatef(0.0f, y, PIPE_OFF);
        cyl_u(PIPE_R, len, 14);
        glPopMatrix();
    }
    for (float u = 1.0f; u < len; u += 2.5f)             // hangers
        box_span(u - 0.03f, PIPE_Y[0] - PIPE_R - 0.04f, 0,
                 u + 0.03f, PIPE_Y[1] + PIPE_R + 0.04f, PIPE_OFF + 0.03f);
    {   // riser down the right-hand end, with a flange and a valve
        const float u = back_u(RISER_X);
        glPushMatrix();
        glTranslatef(u, 0.0f, PIPE_OFF);
        cyl(PIPE_R, PIPE_Y[0], 14);
        cyl(0.15f, 0.06f, 14);
        glPopMatrix();
        box_span(u - 0.13f, 1.00f, PIPE_OFF - 0.13f, u + 0.13f, 1.20f, PIPE_OFF + 0.13f);
        mat::use(mat::SIGNAL_RED);
        glPushMatrix();
        glTranslatef(u, 1.10f, PIPE_OFF + 0.13f);
        cyl_z(0.02f, 0.08f, 8);
        glTranslatef(0.0f, 0.0f, 0.08f);
        cyl_z(0.17f, 0.03f, 18, 0.01f);
        glPopMatrix();
    }
    {   // conduit from the cabinet up to the pipe run, and a breaker box
        mat::use(mat::GALVANIZED);
        glPushMatrix();
        glTranslatef(back_u(CONDUIT_X), CAB_H, 0.06f);
        cyl(0.04f, PIPE_Y[0] - CAB_H, 10);
        glPopMatrix();
        box_span(back_u(BREAKER_X0), 1.30f, 0, back_u(BREAKER_X1), 2.10f, 0.16f);
        mat::use(mat::BLACK_PLASTIC);
        const float um = 0.5f * (back_u(BREAKER_X0) + back_u(BREAKER_X1));
        box_span(um - 0.03f, 1.60f, 0.16f, um + 0.03f, 1.80f, 0.24f);   // lever
    }
}

// ---------------------------------------------------------------------------
// Left wall: windows, roll-up door, extinguisher, tool board, spare-gear shelf
// ---------------------------------------------------------------------------
inline void left_wall_fixtures() {
    for (float z : LWIN_Z) window(left_u(z));

    {   // roll-up door.  u runs toward -z on this wall, so DOOR_Z1 is its left.
        const float u0 = left_u(DOOR_Z1), u1 = left_u(DOOR_Z0);
        const float uc = 0.5f * (u0 + u1);
        mat::use(mat::BLACK_PLASTIC);                    // dark between the slats,
        grid(u0, 0, 0.025f,  u1 - u0, 0, 0,  0, DOOR_H, 0,  1, 4);  // over the trim
        mat::use(mat::GALVANIZED);
        const float sh = (DOOR_H - 0.10f) / DOOR_SLATS;
        for (int k = 0; k < DOOR_SLATS; ++k) {
            const float y = 0.10f + k * sh;
            box_span(u0, y + 0.012f, 0.03f, u1, y + sh - 0.012f, 0.07f);
        }
        box_span(u0 - 0.18f, 0.0f, 0, u0,         DOOR_H + 0.10f, 0.14f);   // guides
        box_span(u1,         0.0f, 0, u1 + 0.18f, DOOR_H + 0.10f, 0.14f);
        box_span(u0 - 0.25f, DOOR_H, 0, u1 + 0.25f, DOOR_H + 0.55f, 0.40f); // coil box
        mat::use(mat::SAFETY_YELLOW);
        box_span(u0, 0.0f, 0.03f, u1, 0.10f, 0.08f);                        // bottom bar
        mat::use(mat::BLACK_PLASTIC);
        box_span(uc - 0.12f, 0.40f, 0.07f, uc + 0.12f, 0.46f, 0.13f);       // handle
    }

    {   // fire extinguisher on a strap bracket, with its sign
        const float u = left_u(EXT_Z), zc = 0.03f + EXT_R;
        mat::use(mat::GALVANIZED);
        box_span(u - 0.10f, 0.70f, 0, u + 0.10f, 0.80f, zc);
        mat::use(mat::SIGNAL_RED);
        glPushMatrix();
        glTranslatef(u, 0.40f, zc);
        cyl(EXT_R, EXT_H, 16, 0.03f);
        glPopMatrix();
        box_span(u - 0.20f, 1.40f, 0, u + 0.20f, 1.75f, 0.015f);
        mat::use(mat::BLACK_PLASTIC);
        glPushMatrix();
        glTranslatef(u, 0.40f + EXT_H, zc);
        cyl(0.045f, 0.10f, 10);
        glPopMatrix();
    }

    const float u0 = left_u(BENCH_Z1), u1 = left_u(BENCH_Z0);   // over the bench

    {   // tool board: spanner, hammer, two screwdrivers
        mat::use(mat::WOOD);
        box_span(u0 + 0.10f, PEG_Y0, 0, u1 - 0.10f, PEG_Y1, 0.03f);

        float u = u0 + 0.45f;
        mat::use(mat::GALVANIZED);
        box_span(u - 0.06f, 1.42f, 0.03f, u + 0.06f, 1.52f, 0.06f);   // ring end
        box_span(u - 0.03f, 1.52f, 0.03f, u + 0.03f, 1.99f, 0.06f);   // shank
        box_span(u - 0.08f, 1.99f, 0.03f, u + 0.08f, 2.07f, 0.06f);   // jaw
        box_span(u - 0.08f, 2.07f, 0.03f, u - 0.035f, 2.20f, 0.06f);
        box_span(u + 0.035f, 2.07f, 0.03f, u + 0.08f, 2.20f, 0.06f);

        u = u0 + 0.90f;
        mat::use(mat::WOOD);
        box_span(u - 0.025f, 1.45f, 0.03f, u + 0.025f, 2.10f, 0.07f); // handle
        mat::use(mat::GALVANIZED);
        box_span(u - 0.14f, 2.10f, 0.03f, u + 0.14f, 2.22f, 0.10f);   // head

        for (int k = 0; k < 2; ++k) {
            u = u0 + 1.30f + 0.25f * k;
            mat::use(mat::SILVER);
            glPushMatrix();
            glTranslatef(u, 1.60f - 0.08f * k, 0.07f);
            cyl(0.012f, 0.30f, 8);
            mat::use(mat::SIGNAL_RED);
            glTranslatef(0.0f, 0.30f, 0.0f);
            cyl(0.035f, 0.20f, 10, 0.01f);
            glPopMatrix();
        }
    }

    {   // shelf of spare gears: a G3-size and a pinion
        mat::use(mat::WOOD);
        box_span(u0, SHELF_Y - 0.05f, 0, u1, SHELF_Y, SHELF_D);
        mat::use(mat::GALVANIZED);
        box_span(u0 + 0.18f, SHELF_Y - 0.30f, 0, u0 + 0.22f, SHELF_Y - 0.05f, SHELF_D - 0.06f);
        box_span(u1 - 0.22f, SHELF_Y - 0.30f, 0, u1 - 0.18f, SHELF_Y - 0.05f, SHELF_D - 0.06f);
        spare_gear(2, u0 + 0.65f, SHELF_Y, 0.5f * SHELF_D);
        spare_gear(0, u0 + 1.55f, SHELF_Y, 0.5f * SHELF_D);
    }
}

// ---------------------------------------------------------------------------
// Ceiling and the two hanging bulbs
// ---------------------------------------------------------------------------
inline void ceiling() {
    const float lx = ROOM_X1 - ROOM_X0, lz = ROOM_Z1 - ROOM_Z0;
    mat::use(mat::WALL_PAINT);
    grid(ROOM_X0, CEIL_Y, ROOM_Z0,  lx, 0, 0,  0, 0, lz,
         (int)(lx / WALL_CELL + 0.5f), (int)(lz / WALL_CELL + 0.5f));

    mat::use(mat::GALVANIZED);
    const float h = 0.5f * BEAM_W, t = BEAM_T, y0 = CEIL_Y - BEAM_D;
    for (int k = 0; k < BEAM_N; ++k) {
        const float x = BEAM_X0 + k * BEAM_PITCH;
        box_span(x - h,        y0,         ROOM_Z0, x + h,        y0 + t, ROOM_Z1);
        box_span(x - 0.5f * t, y0 + t,     ROOM_Z0, x + 0.5f * t, CEIL_Y - t, ROOM_Z1);
        box_span(x - h,        CEIL_Y - t, ROOM_Z0, x + h,        CEIL_Y, ROOM_Z1);
    }
}

// Rose, flex and lampholder for each bulb.  The glass is a separate list,
// because its material follows the switch.
inline void pendants() {
    const float holder = BULB_Y + 0.15f, holder_h = 0.16f, rose = CEIL_Y - 0.05f;
    for (int i = 0; i < 2; ++i) {
        glPushMatrix();
        glTranslatef(BULB_X[i], 0.0f, BULB_Z);
        glPushMatrix();
        glTranslatef(0.0f, rose, 0.0f);
        mat::use(mat::GALVANIZED);
        cyl(0.10f, 0.05f, 16, 0.01f);                            // ceiling rose
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.0f, holder + holder_h, 0.0f);
        mat::use(mat::BLACK_PLASTIC);
        cyl(0.012f, rose - (holder + holder_h), 6);             // flex
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.0f, holder, 0.0f);
        cyl(0.05f, holder_h, 14, 0.01f);                         // lampholder
        mat::use(mat::GALVANIZED);
        cyl(0.058f, 0.03f, 14);                                  // collar
        glPopMatrix();
        glPopMatrix();
    }
}

// A glass globe of radius BULB_R about the origin, necking up into the holder.
inline void globe() {
    V2 p[16];
    int n = 0;
    for (int k = 0; k <= 11; ++k) {                              // -90 to 55 deg
        const float a = (-90.0f + 145.0f * k / 11.0f) * RAD;
        p[n].x = BULB_R * cosf(a);
        p[n].y = BULB_R * sinf(a);
        ++n;
    }
    p[0].x = 0.0f;                                               // exact pole
    p[n].x = 0.042f; p[n].y = 0.135f; ++n;
    p[n].x = 0.040f; p[n].y = 0.170f; ++n;
    p[n].x = 0.0f;   p[n].y = 0.170f; ++n;
    lathe(p, n, 20);
}

// ---------------------------------------------------------------------------
// Floor: hazard markings, a pallet of blanks, workbench, cabinet, drums
// ---------------------------------------------------------------------------
inline void floor_quad(float x0, float z0, float x1, float z1) {
    grid(x0, HAZ_Y, z0,  0, 0, z1 - z0,  x1 - x0, 0, 0,  1, 1);   // faces +y
}

// One dashed strip along its longer side, yellow and black alternating.
inline void dashed(float x0, float z0, float x1, float z1) {
    const bool along_x = (x1 - x0) > (z1 - z0);
    const float len = along_x ? x1 - x0 : z1 - z0;
    int n = (int)(len / HAZ_DASH + 0.5f);
    if (n < 1) n = 1;
    const float d = len / n;
    for (int pass = 0; pass < 2; ++pass) {
        mat::use(pass ? mat::RUBBER : mat::SAFETY_YELLOW);
        for (int i = pass; i < n; i += 2) {
            if (along_x) floor_quad(x0 + i * d, z0, x0 + (i + 1) * d, z1);
            else         floor_quad(x0, z0 + i * d, x1, z0 + (i + 1) * d);
        }
    }
}

inline void hazard_markings() {
    const float w = 0.5f * HAZ_W;
    dashed(HAZ_X0 - w, HAZ_Z0 - w, HAZ_X1 + w, HAZ_Z0 + w);   // back and front
    dashed(HAZ_X0 - w, HAZ_Z1 - w, HAZ_X1 + w, HAZ_Z1 + w);   // run the full width,
    dashed(HAZ_X0 - w, HAZ_Z0 + w, HAZ_X0 + w, HAZ_Z1 - w);   // the sides stop short,
    dashed(HAZ_X1 - w, HAZ_Z0 + w, HAZ_X1 + w, HAZ_Z1 - w);   // so no two overlap
    dashed(ROOM_X0, DOOR_Z0, ROOM_X0 + 0.40f, DOOR_Z1);       // door threshold
}

// Feedstock for the magazine: two layers of the same blank list the belt
// carries, with a slip sheet between, the top layer part used.
inline void pallet_of_blanks() {
    const float h = 0.5f * PAL_S;
    mat::use(mat::WOOD);
    for (int k = -1; k <= 1; ++k) {                          // bearers
        const float z = PAL_Z + k * (h - 0.05f);
        box_span(PAL_X - h, 0.0f, z - 0.05f, PAL_X + h, 0.10f, z + 0.05f);
    }
    for (int k = 0; k < 5; ++k) {                            // deck boards
        const float x = PAL_X - h + 0.08f + k * (PAL_S - 0.16f) / 4.0f;
        box_span(x - 0.08f, 0.10f, PAL_Z - h, x + 0.08f, 0.13f, PAL_Z + h);
    }
    float y = 0.13f;
    for (int layer = 0; layer < PAL_LAYERS; ++layer) {
        const bool top = layer == PAL_LAYERS - 1;
        mat::use(mat::SILVER);
        for (int i = -1; i <= 1; ++i)
            for (int j = -1; j <= 1; ++j) {
                if (top && j == 1 && i >= 0) continue;
                glPushMatrix();
                glTranslatef(PAL_X + i * PAL_PITCH, y, PAL_Z + j * PAL_PITCH);
                glCallList(scene::L(scene::L_BLANK));
                glPopMatrix();
            }
        y += BLANK_H;
        if (!top) {
            mat::use(mat::WOOD);
            box_span(PAL_X - 0.57f, y, PAL_Z - 0.57f, PAL_X + 0.57f, y + 0.02f, PAL_Z + 0.57f);
            y += 0.02f;
        }
    }
}

inline void workbench() {
    const float x0 = ROOM_X0 + 0.04f, x1 = ROOM_X0 + BENCH_D;
    const float z0 = BENCH_Z0, z1 = BENCH_Z1;
    const float yt = BENCH_H - BENCH_TOP_T, l = BENCH_LEG, i = 0.05f, e = 0.01f;
    mat::use(mat::WOOD);
    box_span(x0, yt, z0, x1, BENCH_H, z1);                              // top
    box_span(x0 + i + e, 0.25f, z0 + i + e, x1 - i - e, 0.29f, z1 - i - e); // shelf,
    mat::use(mat::GALVANIZED);                                          // inside the legs
    for (int a = 0; a < 2; ++a)
        for (int b = 0; b < 2; ++b) {
            const float lx = a ? x1 - i - l : x0 + i;
            const float lz = b ? z1 - i - l : z0 + i;
            box_span(lx, 0.0f, lz, lx + l, yt, lz + l);
        }

    {   // vice on the front edge, screw toward the room
        const float vz = z1 - 0.35f, y = BENCH_H;
        mat::use(mat::MACHINE_PAINT);
        box_span(x1 - 0.30f, y,         vz - 0.10f, x1 - 0.02f, y + 0.07f, vz + 0.10f);
        box_span(x1 - 0.30f, y + 0.07f, vz - 0.12f, x1 - 0.22f, y + 0.24f, vz + 0.12f);
        box_span(x1 - 0.12f, y + 0.07f, vz - 0.12f, x1 - 0.04f, y + 0.24f, vz + 0.12f);
        mat::use(mat::SILVER);
        glPushMatrix();
        glTranslatef(x1 - 0.04f, y + 0.14f, vz);
        glRotatef(-90.0f, 0, 0, 1);                     // +Y to +X
        cyl(0.02f, 0.16f, 8);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(x1 + 0.12f, y + 0.14f, vz - 0.12f);
        cyl_z(0.012f, 0.24f, 8);                        // tommy bar
        glPopMatrix();
    }

    mat::use(mat::SIGNAL_RED);                          // toolbox
    box_span(x0 + 0.12f, BENCH_H, z0 + 0.20f, x0 + 0.42f, BENCH_H + 0.22f, z0 + 0.72f);
    mat::use(mat::BLACK_PLASTIC);
    box_span(x0 + 0.25f, BENCH_H + 0.22f, z0 + 0.35f, x0 + 0.29f, BENCH_H + 0.27f, z0 + 0.57f);

    // A blank and a stamped part side by side.  The stamped one is built flat
    // at the squashed size (radius r/sqrt q) rather than scaled, so the belt's
    // squash stays the only glScalef in the scene.
    mat::use(mat::SILVER);
    glPushMatrix();
    glTranslatef(x0 + 0.40f, BENCH_H, z0 + 1.05f);
    glCallList(scene::L(scene::L_BLANK));
    glTranslatef(0.0f, 0.0f, 0.55f);
    cyl(BLANK_R * sqrtf(BLANK_H / BLANK_H_FLAT), BLANK_H_FLAT, BLANK_SLICES, BLANK_CHAMFER);
    glPopMatrix();
}

// The machine's electrical cabinet, against the back wall beside the panel.
inline void cabinet() {
    const float z0 = ROOM_Z0 + 0.04f, z1 = ROOM_Z0 + CAB_D;
    const float xm = 0.5f * (CAB_X0 + CAB_X1);
    mat::use(mat::BLACK_PLASTIC);
    box_span(CAB_X0 + 0.05f, 0.0f, z0 + 0.05f, CAB_X1 - 0.05f, 0.10f, z1 - 0.05f);
    mat::use(mat::MACHINE_PAINT);
    box_span(CAB_X0, 0.10f, z0, CAB_X1, CAB_H, z1);
    mat::use(mat::BLACK_PLASTIC);
    box_span(xm - 0.006f, 0.20f, z1, xm + 0.006f, CAB_H - 0.10f, z1 + 0.008f); // seam
    for (int k = 0; k < 3; ++k)                                                // louvres
        box_span(CAB_X0 + 0.15f, 0.35f + 0.10f * k, z1,
                 CAB_X0 + 0.55f, 0.39f + 0.10f * k, z1 + 0.015f);
    mat::use(mat::GALVANIZED);
    box_span(xm - 0.12f, 1.10f, z1, xm - 0.08f, 1.45f, z1 + 0.05f);            // handles
    box_span(xm + 0.08f, 1.10f, z1, xm + 0.12f, 1.45f, z1 + 0.05f);
    mat::use(mat::BLACK_PLASTIC);                        // switch plates; the
    for (float x : SWITCH_X)                             // rockers and lamps
        box_span(x - 0.09f, SWITCH_Y - 0.14f, z1,        // are drawn per frame
                 x + 0.09f, SWITCH_Y + 0.14f, z1 + 0.02f);
}

inline void drums() {
    for (int k = 0; k < 2; ++k) {
        mat::use(k ? mat::GALVANIZED : mat::SIGNAL_RED);
        glPushMatrix();
        glTranslatef(DRUM_X[k], 0.0f, DRUM_Z[k]);
        cyl(DRUM_R, DRUM_H, 20, 0.02f);
        glPushMatrix();                                  // two rolling hoops
        glTranslatef(0.0f, 0.30f, 0.0f);
        cyl(DRUM_R + 0.012f, 0.03f, 20);
        glTranslatef(0.0f, 0.28f, 0.0f);
        cyl(DRUM_R + 0.012f, 0.03f, 20);
        glPopMatrix();
        mat::use(mat::BLACK_PLASTIC);                    // bung
        glTranslatef(0.14f, DRUM_H, 0.0f);
        cyl(0.04f, 0.02f, 10);
        glPopMatrix();
    }
}

// ---------------------------------------------------------------------------
// Display lists
// ---------------------------------------------------------------------------
inline void fan_rotor() {
    mat::use(mat::SILVER);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.13f);
    cyl_z(0.07f, 0.08f, 14, 0.015f);
    glPopMatrix();
    mat::use(mat::GALVANIZED);
    for (int k = 0; k < FAN_BLADES; ++k) {
        glPushMatrix();
        glRotatef(360.0f * k / FAN_BLADES, 0, 0, 1);
        glTranslatef(0.24f, 0.0f, 0.17f);
        glRotatef(25.0f, 1, 0, 0);                       // blade pitch
        box(0.28f, 0.12f, 0.015f);
        glPopMatrix();
    }
}

// Called after scene::build_lists(): the shelf gears and the pallet's blanks
// call the machine's own lists.
inline void build_lists() {
    g_list = glGenLists(R_COUNT);

    glNewList(L(R_FLOOR_ITEMS), GL_COMPILE);
    hazard_markings();
    pallet_of_blanks();
    workbench();
    cabinet();
    drums();
    glEndList();

    for (int w = BACK; w <= FRONT; ++w) {
        glNewList(L(R_WALL0 + w), GL_COMPILE);
        glPushMatrix();
        enter_wall(w);
        wall_surface(wall_len(w));
        switch (w) {
        case BACK:  back_wall_fixtures(); break;
        case LEFT:  left_wall_fixtures(); break;
        case RIGHT: for (float z : RWIN_Z) window(right_u(z)); break;
        default:    for (float x : FWIN_X) window(front_u(x)); break;
        }
        glPopMatrix();
        glEndList();
    }

    glNewList(L(R_CEILING), GL_COMPILE);
    ceiling();
    glEndList();

    glNewList(L(R_FAN), GL_COMPILE);
    fan_rotor();
    glEndList();

    glNewList(L(R_PENDANTS), GL_COMPILE);
    pendants();
    glEndList();

    glNewList(L(R_GLOBE), GL_COMPILE);                   // no material: per frame
    globe();
    glEndList();
}

// ---------------------------------------------------------------------------
// Per frame
// ---------------------------------------------------------------------------

// Seven-segment digits.  Like the stack light, the emission changes at run
// time, so it is set here and never baked into a list.  Lit segments are
// drawn in one pass and unlit in another, two material changes per frame.
inline void draw_counter(long parts) {
    static const unsigned char SEG[10] = {           // bits a..g
        0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };
    struct Seg { float x, y; bool horiz; };
    const float w = 0.5f * DIG_W, h = 0.5f * DIG_H, q = 0.25f * DIG_H;
    const Seg S[7] = { { 0,  h, true }, { w,  q, false }, { w, -q, false },
                       { 0, -h, true }, {-w, -q, false }, {-w,  q, false },
                       { 0,  0, true } };

    long mod = 1;
    for (int k = 0; k < COUNTER_DIGITS; ++k) mod *= 10;
    long v = parts % mod;
    int digit[COUNTER_DIGITS];
    for (int k = COUNTER_DIGITS - 1; k >= 0; --k) { digit[k] = (int)(v % 10); v /= 10; }

    const float uc = back_u(COUNTER_X), yc = COUNTER_Y + 0.12f + h;
    for (int pass = 0; pass < 2; ++pass) {
        const int lit = pass == 0 ? 1 : 0;
        mat::use(mat::lens(1.0f, 0.30f, 0.08f, lit != 0));
        for (int k = 0; k < COUNTER_DIGITS; ++k) {
            const float xc = uc + (k - 0.5f * (COUNTER_DIGITS - 1)) * DIG_PITCH;
            for (int s = 0; s < 7; ++s) {
                if (((SEG[digit[k]] >> s) & 1) != lit) continue;
                const float sx = S[s].horiz ? DIG_W - SEG_T : SEG_T;
                const float sy = S[s].horiz ? SEG_T : 0.5f * DIG_H - SEG_T;
                box_span(xc + S[s].x - 0.5f * sx, yc + S[s].y - 0.5f * sy, 0.10f,
                         xc + S[s].x + 0.5f * sx, yc + S[s].y + 0.5f * sy, 0.12f);
            }
        }
    }
    glMaterialfv(GL_FRONT, GL_EMISSION, mat::NO_EMISSION);
}

// The cabinet's rockers and status lamps, drawn from the same flags the keys
// flip.  A rocker leans its top in when on, and the lamp over it lights.
inline void draw_switches(const bool* sw) {
    const float z1 = ROOM_Z0 + CAB_D + 0.02f;            // plate face
    mat::use(mat::GALVANIZED);
    for (int k = 0; k < SW_COUNT; ++k) {
        glPushMatrix();
        glTranslatef(SWITCH_X[k], SWITCH_Y, z1);
        glRotatef(sw[k] ? -16.0f : 16.0f, 1, 0, 0);
        box(0.08f, 0.16f, 0.05f);
        glPopMatrix();
    }
    for (int k = 0; k < SW_COUNT; ++k) {
        const float* c = SWITCH_RGB[k];
        mat::use(mat::lens(c[0], c[1], c[2], sw[k]));
        glPushMatrix();
        glTranslatef(SWITCH_X[k], SWITCH_Y + 0.28f, z1 - 0.02f);
        cyl_z(0.035f, 0.03f, 12);
        glPopMatrix();
    }
    glMaterialfv(GL_FRONT, GL_EMISSION, mat::NO_EMISSION);
}

// `eye` is the camera position in world space, which decides the cutaway.
// `sw` is indexed by Switch; `fan_deg` is the fan's own accumulated angle.
inline void draw(const float* eye, float th, long cycles, float fan_deg,
                 const bool* sw) {
    glCallList(L(R_FLOOR_ITEMS));
    draw_switches(sw);

    for (int w = BACK; w <= FRONT; ++w) {
        if (!wall_shown(w, eye)) continue;
        glCallList(L(R_WALL0 + w));
        if (w != BACK) continue;
        glPushMatrix();
        enter_wall(BACK);
        glPushMatrix();
        glTranslatef(back_u(FAN_X), FAN_Y, 0.0f);
        glRotatef(-fan_deg, 0, 0, 1);
        glCallList(L(R_FAN));
        glPopMatrix();
        draw_counter(kin::parts_made(th, cycles));
        glPopMatrix();
    }

    if (eye[1] < CEIL_Y) glCallList(L(R_CEILING));      // from above, it goes

    // The bulbs hang inside the room, so they stay in every view.
    glCallList(L(R_PENDANTS));
    for (int i = 0; i < 2; ++i) {
        mat::use(sw[SW_BULB_L + i] ? mat::BULB_ON : mat::BULB_OFF);
        glPushMatrix();
        glTranslatef(BULB_X[i], BULB_Y, BULB_Z);
        glCallList(L(R_GLOBE));
        glPopMatrix();
    }
    glMaterialfv(GL_FRONT, GL_EMISSION, mat::NO_EMISSION);
}

// A soft halo round each lit bulb, so it reads as the source of the light: a
// camera-facing disc whose alpha falls from the centre to the rim, blended
// additively.  It is drawn last and outside the lighting - no program, no
// lighting, and smooth shading even in FLAT mode, where the rim's zero alpha
// would otherwise flood the whole disc.  Depth is tested, so the machine still
// hides a bulb behind it, but not written, so the halo cuts no holes.
inline void draw_glow(const bool* sw) {
    GLint prog = 0, shade = GL_SMOOTH;
    if (GLEW_VERSION_2_0) {
        glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
        glUseProgram(0);
    }
    glGetIntegerv(GL_SHADE_MODEL, &shade);

    // With only the camera on the modelview stack, the matrix's first two rows
    // are the camera's right and up vectors in world space.
    GLfloat m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    const float rx = m[0], ry = m[4], rz = m[8];
    const float ux = m[1], uy = m[5], uz = m[9];

    glDisable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    for (int i = 0; i < 2; ++i) {
        if (!sw[SW_BULB_L + i]) continue;
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.00f, 0.88f, 0.62f, 0.60f);
        glVertex3f(BULB_X[i], BULB_Y, BULB_Z);
        glColor4f(1.00f, 0.80f, 0.45f, 0.0f);
        for (int k = 0; k <= 32; ++k) {
            const float a = 2.0f * PI * k / 32;
            const float c = GLOW_R * cosf(a), sn = GLOW_R * sinf(a);
            glVertex3f(BULB_X[i] + c * rx + sn * ux,
                       BULB_Y    + c * ry + sn * uy,
                       BULB_Z    + c * rz + sn * uz);
        }
        glEnd();
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glShadeModel((GLenum)shade);
    glEnable(GL_LIGHTING);
    if (GLEW_VERSION_2_0) glUseProgram((GLuint)prog);
}

} // namespace room
#endif
