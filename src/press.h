
#ifndef PRESS_H
#define PRESS_H

#include "common.h"

namespace scene {

inline void build_press() {
    glNewList(L(L_PANEL), GL_COMPILE);
    mat::use(mat::MACHINE_PAINT);
    glPushMatrix();
    glTranslatef(PANEL_CX, PANEL_CY, PANEL_CZ);
    box(PANEL_W, PANEL_H, PANEL_D);
    glPopMatrix();
    for (int s = -1; s <= 1; s += 2) {
        const float x = PRESS_X + s * RAIL_X_OFF;
        box_span(x - 0.5f*RAIL_W, RAIL_Y0, -0.5f*RAIL_D,
                 x + 0.5f*RAIL_W, RAIL_Y1,  0.5f*RAIL_D);
        box_span(x - 0.5f*RAIL_W, BRACKET_Y0, PANEL_CZ + 0.5f*PANEL_D,
                 x + 0.5f*RAIL_W, BRACKET_Y1, -0.5f*RAIL_D);
    }
    glEndList();
}

inline void draw_press(float th) {
    const float px = lay::pin_x(th), py = lay::pin_y(th);
    const float s  = lay::ram_top(th);

    mat::use(mat::SILVER);
    glPushMatrix();
    glTranslatef(PRESS_X, CRANK_Y, 0.0f);
    glRotatef(lay::crank_deg(th), 0, 0, 1);
    glPushMatrix();
    glTranslatef(0, 0, PANEL_CZ + 0.5f * PANEL_D);
    cyl_z(CRANK_SHAFT_R, CRANK_DISC_Z0 - (PANEL_CZ + 0.5f*PANEL_D));
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, 0, CRANK_DISC_Z0);
    cyl_z(CRANK_DISC_R, CRANK_DISC_T);
    glPopMatrix();
    glPushMatrix();                                  // crank pin
    glTranslatef(CRANK_R, 0.0f, CRANK_DISC_Z0 + CRANK_DISC_T);
    cyl_z(CRANK_PIN_R, 0.11f);
    glPopMatrix();
    glPopMatrix();
    mat::use(mat::DARK_STEEL);
    glPushMatrix();
    glTranslatef(px, py, 0.0f);
    glRotatef(lay::rod_deg(th), 0, 0, 1);
    glTranslatef(0.5f * ROD_L, 0.0f, 0.0f);
    box(ROD_L, ROD_W, ROD_D);
    glPopMatrix();

    mat::use(mat::SILVER);
    glPushMatrix();                                  // wrist pin
    glTranslatef(PRESS_X, s, -0.06f);
    cyl_z(0.06f, 0.12f);
    glPopMatrix();

    mat::use(mat::DARK_STEEL);
    glPushMatrix();                                  // ram and punch
    glTranslatef(PRESS_X, s - 0.5f * RAM_H, 0.0f);
    box(RAM_W, RAM_H, RAM_D);
    glPopMatrix();
}

} // namespace scene
#endif
