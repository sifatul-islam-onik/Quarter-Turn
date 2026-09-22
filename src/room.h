#ifndef ROOM_H
#define ROOM_H

#include "scene.h"

namespace room {

using namespace cfg;
using namespace prim;

enum Wall { BACK = 0, LEFT, RIGHT, FRONT };
enum { R_FLOOR_ITEMS = 0, R_WALL0, R_CEILING = R_WALL0 + 4, R_FAN,
       R_PENDANTS, R_GLOBE, R_COUNT };

enum Switch { SW_MACHINE = 0, SW_BULB_L, SW_BULB_R, SW_FAN, SW_COUNT };
constexpr float SWITCH_RGB[SW_COUNT][3] = {
    { 0.25f, 0.90f, 0.35f }, { 1.00f, 0.72f, 0.20f },
    { 1.00f, 0.72f, 0.20f }, { 0.30f, 0.70f, 1.00f } };
inline GLuint g_list = 0;
inline GLuint L(int i) { return g_list + (GLuint)i; }

inline float wall_len(int w) {
    return (w == LEFT || w == RIGHT) ? ROOM_Z1 - ROOM_Z0 : ROOM_X1 - ROOM_X0;
}

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

inline void wall_surface(float len) {
    const int nu = (int)(len / WALL_CELL + 0.5f);
    const int nw = (int)((CEIL_Y - DADO_H) / WALL_CELL + 0.5f);
    mat::use(mat::CONCRETE);
    tiles_z(0.0f, 0.0f, len, DADO_H, 0.0f, nu, 3);
    mat::use(mat::WALL_PAINT);
    tiles_z(0.0f, DADO_H, len, CEIL_Y, 0.0f, nu, nw);
    mat::use(mat::SAFETY_YELLOW);
    box_span(0, DADO_H - 0.5f * TRIM_H, 0, len, DADO_H + 0.5f * TRIM_H, 0.02f);
}

inline void window(float uc) {
    const float u0 = uc - 0.5f * WIN_W, u1 = uc + 0.5f * WIN_W;
    const float y0 = WIN_Y0, y1 = WIN_Y0 + WIN_H, f = WIN_FRAME;
    const float d = WIN_DEPTH;
    mat::use(mat::WINDOW_GLASS);
    tiles_z(u0, y0, u1, y1, 0.01f, 1, 1);
    mat::use(mat::GALVANIZED);
    box_span(u0 - f, y0 - f, 0, u1 + f, y0,     d);         // sill
    box_span(u0 - f, y1,     0, u1 + f, y1 + f, d);         // head
    box_span(u0 - f, y0,     0, u0,     y1,     d);         // jambs
    box_span(u1,     y0,     0, u1 + f, y1,     d);
}

inline void spare_gear(int i, float u, float y, float z) {
    mat::use(mat::BRASS);
    glPushMatrix();
    glTranslatef(u, y + scene::gear_tip_r(i), z);
    glCallList(scene::L(scene::L_GEAR0 + i));
    for (int k = 0; k < lay::LAY.g[i].N; ++k) {
        glPushMatrix();
        glRotatef(360.0f * k / lay::LAY.g[i].N, 0, 0, 1);
        glTranslatef(scene::gear_root_r(i), 0.0f, 0.0f);
        glCallList(scene::L(scene::L_TOOTH));
        glPopMatrix();
    }
    glPopMatrix();
}

inline void back_wall_fixtures() {
    for (float x : BWIN_X) window(back_u(x));

    {   // exhaust fan housing; the rotor is drawn per frame
        const float uc = back_u(FAN_X), h = 0.5f * FAN_SIZE, f = 0.08f, d = 0.28f;
        mat::use(mat::BLACK_PLASTIC);
        tiles_z(uc - h + f, FAN_Y - h + f, uc + h - f, FAN_Y + h - f,
                0.01f, 1, 1);
        glPushMatrix();                                  // motor behind the hub
        glTranslatef(uc, FAN_Y, 0.01f);
        cyl_z(0.10f, 0.12f);
        glPopMatrix();
        mat::use(mat::GALVANIZED);
        box_span(uc - h,     FAN_Y - h,     0, uc + h,     FAN_Y - h + f, d);
        box_span(uc - h,     FAN_Y + h - f, 0, uc + h,     FAN_Y + h,     d);
        box_span(uc - h,     FAN_Y - h + f, 0, uc - h + f, FAN_Y + h - f, d);
        box_span(uc + h - f, FAN_Y - h + f, 0, uc + h,     FAN_Y + h - f, d);
    }
}


inline void left_wall_fixtures() {
    for (float z : LWIN_Z) window(left_u(z));

    {   // shelf of spare gears: a G3-size and a pinion
        const float u0 = left_u(SHELF_Z1), u1 = left_u(SHELF_Z0);
        mat::use(mat::WOOD);
        box_span(u0, SHELF_Y - 0.05f, 0, u1, SHELF_Y, SHELF_D);
        mat::use(mat::GALVANIZED);
        box_span(u0 + 0.18f, SHELF_Y - 0.30f, 0, u0 + 0.22f, SHELF_Y - 0.05f, SHELF_D - 0.06f);
        box_span(u1 - 0.22f, SHELF_Y - 0.30f, 0, u1 - 0.18f, SHELF_Y - 0.05f, SHELF_D - 0.06f);
        spare_gear(2, u0 + 0.65f, SHELF_Y, 0.5f * SHELF_D);
        spare_gear(0, u0 + 1.55f, SHELF_Y, 0.5f * SHELF_D);
    }
}


inline void ceiling() {
    const float lx = ROOM_X1 - ROOM_X0, lz = ROOM_Z1 - ROOM_Z0;
    mat::use(mat::WALL_PAINT);
    tiles_y(ROOM_X0, ROOM_Z0, ROOM_X1, ROOM_Z1, CEIL_Y,
            (int)(lx / WALL_CELL + 0.5f), (int)(lz / WALL_CELL + 0.5f), false);

    mat::use(mat::GALVANIZED);
    const float h = 0.5f * BEAM_W, t = BEAM_T, y0 = CEIL_Y - BEAM_D;
    for (int k = 0; k < BEAM_N; ++k) {
        const float x = BEAM_X0 + k * BEAM_PITCH;
        box_span(x - h,        y0,         ROOM_Z0, x + h,        y0 + t, ROOM_Z1);
        box_span(x - 0.5f * t, y0 + t,     ROOM_Z0, x + 0.5f * t, CEIL_Y - t, ROOM_Z1);
        box_span(x - h,        CEIL_Y - t, ROOM_Z0, x + h,        CEIL_Y, ROOM_Z1);
    }
}

inline void pendants() {
    const float rose = CEIL_Y - 0.05f;
    mat::use(mat::BLACK_PLASTIC);
    for (int i = 0; i < 2; ++i) {                // just the flex, ceiling to bulb
        glPushMatrix();
        glTranslatef(BULB_X[i], BULB_Y, BULB_Z);
        cyl(0.012f, rose - BULB_Y);
        glPopMatrix();
    }
}

inline void globe() {
    glPushMatrix();
    glTranslatef(0.0f, -BULB_R, 0.0f);
    cyl(BULB_R, BULB_R + 0.15f);
    glPopMatrix();
}


inline void floor_quad(float x0, float z0, float x1, float z1) {
    tiles_y(x0, z0, x1, z1, HAZ_Y, 1, 1, true);
}


inline void hazard_markings() {
    const float w = 0.5f * HAZ_W;
    mat::use(mat::SAFETY_YELLOW);                             // a plain border
    floor_quad(HAZ_X0 - w, HAZ_Z0 - w, HAZ_X1 + w, HAZ_Z0 + w);   // back and front
    floor_quad(HAZ_X0 - w, HAZ_Z1 - w, HAZ_X1 + w, HAZ_Z1 + w);   // run the full
    floor_quad(HAZ_X0 - w, HAZ_Z0 + w, HAZ_X0 + w, HAZ_Z1 - w);   // width, the sides
    floor_quad(HAZ_X1 - w, HAZ_Z0 + w, HAZ_X1 + w, HAZ_Z1 - w);   // stop short
}

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
    mat::use(mat::SILVER);                                   // one layer of nine
    for (int i = -1; i <= 1; ++i)
        for (int j = -1; j <= 1; ++j) {
            glPushMatrix();
            glTranslatef(PAL_X + i * PAL_PITCH, 0.13f, PAL_Z + j * PAL_PITCH);
            glCallList(scene::L(scene::L_BLANK));
            glPopMatrix();
        }
}

// The machine's electrical cabinet, against the back wall beside the panel.
inline void cabinet() {
    const float z0 = ROOM_Z0 + 0.04f, z1 = ROOM_Z0 + CAB_D;
    mat::use(mat::MACHINE_PAINT);
    box_span(CAB_X0, 0.0f, z0, CAB_X1, CAB_H, z1);
    mat::use(mat::BLACK_PLASTIC);                        // switch plates; the
    for (float x : SWITCH_X)                             // lamps are per frame
        box_span(x - 0.09f, SWITCH_Y - 0.14f, z1,
                 x + 0.09f, SWITCH_Y + 0.14f, z1 + 0.02f);
}

inline void drums() {
    for (int k = 0; k < 2; ++k) {
        mat::use(k ? mat::GALVANIZED : mat::SIGNAL_RED);
        glPushMatrix();
        glTranslatef(DRUM_X[k], 0.0f, DRUM_Z[k]);
        cyl(DRUM_R, DRUM_H);
        glPopMatrix();
    }
}

inline void fan_rotor() {
    mat::use(mat::SILVER);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.13f);
    cyl_z(0.07f, 0.08f);
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

inline void build_lists() {
    g_list = glGenLists(R_COUNT);

    glNewList(L(R_FLOOR_ITEMS), GL_COMPILE);
    hazard_markings();
    pallet_of_blanks();
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

inline void draw_switches(const bool* sw) {
    const float z1 = ROOM_Z0 + CAB_D + 0.02f;            // plate face
    glDisable(GL_LIGHTING);                              // a lamp makes light,
    for (int k = 0; k < SW_COUNT; ++k) {                 // it does not catch it
        const float* c = SWITCH_RGB[k];                  // in the HUD dot colour
        mat::use(mat::lens(c[0], c[1], c[2], sw[k]));
        glPushMatrix();
        glTranslatef(SWITCH_X[k], SWITCH_Y, z1);
        cyl_z(0.035f, 0.03f);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

inline void draw(const float* eye, const bool* sw, float fan_deg,
                 bool edge_pass) {
    glCallList(L(R_FLOOR_ITEMS));
    draw_switches(sw);

    for (int w = BACK; w <= FRONT; ++w) {
        if (!wall_shown(w, eye)) continue;
        glCallList(L(R_WALL0 + w));
        if (w != BACK) continue;
        glPushMatrix();
        enter_wall(BACK);
        glTranslatef(back_u(FAN_X), FAN_Y, 0.0f);
        glRotatef(fan_deg, 0, 0, 1);
        glCallList(L(R_FAN));
        glPopMatrix();
    }

    if (eye[1] < CEIL_Y) glCallList(L(R_CEILING));      // from above, it goes

    // The bulbs hang inside the room, so they stay in every view.
    glCallList(L(R_PENDANTS));
    if (edge_pass) return;
    glDisable(GL_LIGHTING);             // the glass is the source of the light
    for (int i = 0; i < 2; ++i) {
        mat::use(sw[SW_BULB_L + i] ? mat::BULB_ON : mat::BULB_OFF);
        glPushMatrix();
        glTranslatef(BULB_X[i], BULB_Y, BULB_Z);
        glCallList(L(R_GLOBE));
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

} // namespace room
#endif
