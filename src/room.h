#ifndef ROOM_H
#define ROOM_H

#include "scene.h"

namespace room {

using namespace cfg;
using namespace prim;

enum Wall { BACK = 0, LEFT, RIGHT, FRONT };

enum Switch { SW_MACHINE = 0, SW_BULB_L, SW_BULB_R, SW_FAN, SW_COUNT };
constexpr float SWITCH_RGB[SW_COUNT][3] = {
    { 0.25f, 0.90f, 0.35f }, { 1.00f, 0.72f, 0.20f },
    { 1.00f, 0.72f, 0.20f }, { 0.30f, 0.70f, 1.00f } };

// Parts built once and drawn several times.
inline GLuint window_list;        // one window: nine on the walls
inline GLuint beam_list;          // one ceiling I-beam: five
inline GLuint lamp_list;          // one pendant, flex and shade: two
inline GLuint globe_list;         // one bulb globe, coloured each frame: two
inline GLuint pallet_list;        // one empty pallet: two
inline GLuint drum_list;          // one drum, coloured before each call: two
// Whole groups, made from those parts.
inline GLuint floor_items_list;   // hazard border, pallets, cabinet, drums
inline GLuint wall_list[4];       // each wall with its windows and fittings
inline GLuint ceiling_list;       // the ceiling and its beams
inline GLuint fan_list;           // the fan rotor, turned each frame

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

// Four bars round the opening u0..u1, y0..y1, each f wide and d deep.
// The windows and the fan housing share it.
inline void frame(float u0, float y0, float u1, float y1, float f, float d) {
    box_span(u0 - f, y0 - f, 0, u1 + f, y0,     d);     // bottom
    box_span(u0 - f, y1,     0, u1 + f, y1 + f, d);     // top
    box_span(u0 - f, y0,     0, u0,     y1,     d);     // left
    box_span(u1,     y0,     0, u1 + f, y1,     d);     // right
}

// One window, centred on u = 0 with its sill at y = 0: daylight glass in a
// frame, split into four panes by a cross bar.
inline void window() {
    const float u = 0.5f * WIN_W, h = WIN_H, b = 0.5f * WIN_BAR;
    mat::glow(mat::WINDOW_GLASS);                        // lit from outside
    tiles_z(-u, 0.0f, u, h, 0.01f, 1, 1);
    mat::use(mat::GALVANIZED);
    frame(-u, 0.0f, u, h, WIN_FRAME, WIN_DEPTH);
    box_span(-b, 0.0f,         0, b, h,            0.05f);   // upright
    box_span(-u, 0.5f * h - b, 0, u, 0.5f * h + b, 0.04f);   // cross bar
}

inline void window_at(float u) {
    glPushMatrix();
    glTranslatef(u, WIN_Y0, 0.0f);
    glCallList(window_list);
    glPopMatrix();
}

inline void spare_gear(int i, float u, float y, float z) {
    mat::use(mat::BRASS);
    glPushMatrix();
    glTranslatef(u, y + scene::gear_tip_r(i), z);
    scene::gear_shape(i);
    glPopMatrix();
}

inline void back_wall_fixtures() {
    for (float x : BWIN_X) window_at(back_u(x));

    {   // exhaust fan housing; the rotor is drawn per frame
        const float uc = back_u(FAN_X), h = 0.5f * FAN_SIZE - 0.08f;
        mat::use(mat::BLACK_PLASTIC);
        tiles_z(uc - h, FAN_Y - h, uc + h, FAN_Y + h, 0.01f, 1, 1);
        glPushMatrix();                                  // motor behind the hub
        glTranslatef(uc, FAN_Y, 0.01f);
        cyl_z(0.10f, 0.12f);
        glPopMatrix();
        mat::use(mat::GALVANIZED);
        frame(uc - h, FAN_Y - h, uc + h, FAN_Y + h, 0.08f, 0.28f);
    }
}


inline void left_wall_fixtures() {
    for (float z : LWIN_Z) window_at(left_u(z));

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

// One I-beam along z, centred on x = 0: flange, web, flange.
inline void ibeam() {
    const float h = 0.5f * BEAM_W, t = BEAM_T, y0 = CEIL_Y - BEAM_D;
    mat::use(mat::GALVANIZED);
    box_span(-h,        y0,         ROOM_Z0, h,        y0 + t,     ROOM_Z1);
    box_span(-0.5f * t, y0 + t,     ROOM_Z0, 0.5f * t, CEIL_Y - t, ROOM_Z1);
    box_span(-h,        CEIL_Y - t, ROOM_Z0, h,        CEIL_Y,     ROOM_Z1);
}

inline void ceiling() {
    const float lx = ROOM_X1 - ROOM_X0, lz = ROOM_Z1 - ROOM_Z0;
    mat::use(mat::WALL_PAINT);
    tiles_y(ROOM_X0, ROOM_Z0, ROOM_X1, ROOM_Z1, CEIL_Y,
            (int)(lx / WALL_CELL + 0.5f), (int)(lz / WALL_CELL + 0.5f), false);

    for (int k = 0; k < BEAM_N; ++k) {
        glPushMatrix();
        glTranslatef(BEAM_X0 + k * BEAM_PITCH, 0.0f, 0.0f);
        glCallList(beam_list);
        glPopMatrix();
    }
}

// One pendant lamp about its bulb's centre: the flex up to the ceiling and
// a metal shade over the glass.  The glass is globe_list.
inline void lamp() {
    mat::use(mat::BLACK_PLASTIC);
    cyl(0.012f, CEIL_Y - 0.05f - BULB_Y);                // flex
    mat::use(mat::GALVANIZED);
    glPushMatrix();
    glTranslatef(0.0f, 0.10f, 0.0f);
    cyl(SHADE_R, 0.06f);                                 // rim
    cyl(0.5f * SHADE_R, 0.16f);                          // crown
    glPopMatrix();
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

// One empty pallet, centred on the origin: 3 bearers under 5 deck boards.
inline void pallet() {
    const float h = 0.5f * PAL_S;
    mat::use(mat::WOOD);
    for (int k = -1; k <= 1; ++k) {
        const float z = k * (h - 0.05f);
        box_span(-h, 0.0f, z - 0.05f, h, 0.10f, z + 0.05f);
    }
    for (int k = 0; k < 5; ++k) {
        const float x = -h + 0.08f + k * (PAL_S - 0.16f) / 4.0f;
        box_span(x - 0.08f, 0.10f, -h, x + 0.08f, PAL_TOP, h);
    }
}

// A pallet with a layer of nine blanks, each h tall.  draw_blank is the
// belt's own routine, so a stamped blank here is squashed the same way.
inline void loaded_pallet(int p, float h) {
    glPushMatrix();
    glTranslatef(PAL_X[p], 0.0f, PAL_Z[p]);
    glCallList(pallet_list);
    mat::use(mat::SILVER);
    for (int j = -1; j <= 1; ++j) {
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, j * PAL_PITCH);
        for (int i = -1; i <= 1; ++i)
            scene::draw_blank(i * PAL_PITCH, PAL_TOP, h);
        glPopMatrix();
    }
    glPopMatrix();
}

// The machine's electrical cabinet, against the back wall beside the panel.
inline void cabinet() {
    const float z0 = ROOM_Z0 + 0.04f, z1 = ROOM_Z0 + CAB_D;
    const float xm = 0.5f * (CAB_X0 + CAB_X1);
    mat::use(mat::MACHINE_PAINT);
    box_span(CAB_X0, 0.0f, z0, CAB_X1, CAB_H, z1);
    mat::use(mat::BLACK_PLASTIC);                        // the door seam, then
    box_span(xm - 0.01f, 0.08f, z1, xm + 0.01f, CAB_H - 0.08f, z1 + 0.005f);
    for (float x : SWITCH_X)                             // the switch plates;
        box_span(x - 0.09f, SWITCH_Y - 0.14f, z1,        // the lamps are per
                 x + 0.09f, SWITCH_Y + 0.14f, z1 + 0.02f);   // frame
}

// One drum: the body, two rolling hoops and a rim under the lid.  It sets no
// colour, so each copy is coloured just before it is called.
inline void drum() {
    const float hoop_y[3] = { DRUM_H / 3.0f, 2.0f * DRUM_H / 3.0f, DRUM_H - 0.03f };
    cyl(DRUM_R, DRUM_H);
    for (float y : hoop_y) {
        glPushMatrix();
        glTranslatef(0.0f, y - 0.02f, 0.0f);
        cyl(DRUM_R + 0.015f, 0.04f);
        glPopMatrix();
    }
}

inline void drums() {
    for (int k = 0; k < 2; ++k) {
        mat::use(k ? mat::GALVANIZED : mat::SIGNAL_RED);
        glPushMatrix();
        glTranslatef(DRUM_X[k], 0.0f, DRUM_Z[k]);
        glCallList(drum_list);
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
    // The parts first: the groups below call them, so they need their ids.
    window_list = new_list();
    window();
    glEndList();

    beam_list = new_list();
    ibeam();
    glEndList();

    lamp_list = new_list();
    lamp();
    glEndList();

    globe_list = new_list();                             // no colour: per frame
    globe();
    glEndList();

    pallet_list = new_list();
    pallet();
    glEndList();

    drum_list = new_list();                              // no colour: per copy
    drum();
    glEndList();

    floor_items_list = new_list();
    hazard_markings();
    loaded_pallet(0, BLANK_H);                           // raw, by the tail
    loaded_pallet(1, BLANK_H_FLAT);                      // stamped, by the head
    cabinet();
    drums();
    glEndList();

    for (int w = BACK; w <= FRONT; ++w) {
        wall_list[w] = new_list();
        glPushMatrix();
        enter_wall(w);
        wall_surface(wall_len(w));
        switch (w) {
        case BACK:  back_wall_fixtures(); break;
        case LEFT:  left_wall_fixtures(); break;
        case RIGHT: for (float z : RWIN_Z) window_at(right_u(z)); break;
        default:    for (float x : FWIN_X) window_at(front_u(x)); break;
        }
        glPopMatrix();
        glEndList();
    }

    ceiling_list = new_list();
    ceiling();
    glEndList();

    fan_list = new_list();
    fan_rotor();
    glEndList();
}

inline void draw_switches(const bool* sw) {
    const float z1 = ROOM_Z0 + CAB_D + 0.02f;            // plate face
    for (int k = 0; k < SW_COUNT; ++k) {                 // in the HUD dot colour
        const float* c = SWITCH_RGB[k];
        mat::lens(c[0], c[1], c[2], sw[k]);
        glPushMatrix();
        glTranslatef(SWITCH_X[k], SWITCH_Y, z1);
        cyl_z(0.035f, 0.03f);
        glPopMatrix();
    }
}

inline void draw(const float* eye, const bool* sw, float fan_deg,
                 bool edge_pass) {
    glCallList(floor_items_list);
    draw_switches(sw);

    for (int w = BACK; w <= FRONT; ++w) {
        if (!wall_shown(w, eye)) continue;
        glCallList(wall_list[w]);
        if (w != BACK) continue;
        glPushMatrix();
        enter_wall(BACK);
        glTranslatef(back_u(FAN_X), FAN_Y, 0.0f);
        glRotatef(fan_deg, 0, 0, 1);
        glCallList(fan_list);
        glPopMatrix();
    }

    if (eye[1] < CEIL_Y) glCallList(ceiling_list);      // from above, it goes

    // The lamps hang inside the room, so they stay in every view.  A lit
    // globe glows (emission); the edge pass leaves it without an outline.
    for (int i = 0; i < 2; ++i) {
        glPushMatrix();
        glTranslatef(BULB_X[i], BULB_Y, BULB_Z);
        glCallList(lamp_list);
        if (!edge_pass) {
            if (sw[SW_BULB_L + i]) mat::glow(mat::BULB_ON);
            else                   mat::use(mat::BULB_OFF);
            glCallList(globe_list);
        }
        glPopMatrix();
    }
}

} // namespace room
#endif
