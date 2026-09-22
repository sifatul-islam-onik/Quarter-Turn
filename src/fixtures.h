
#ifndef FIXTURES_H
#define FIXTURES_H

#include "common.h"

namespace scene {

inline void build_fixtures() {
    // ---- floor (PRD 4.2 row 1) --------------------------------------------
    glNewList(L(L_FLOOR), GL_COMPILE);
    mat::use(mat::CONCRETE);
    tiles_y(FLOOR_X0, FLOOR_Z0, FLOOR_X1, FLOOR_Z1, 0.0f,
            FLOOR_NX, FLOOR_NZ, true);
    glEndList();

    // ---- motor body and mount (row 3) -------------------------------------
    glNewList(L(L_MOTOR), GL_COMPILE);
    mat::use(mat::BLACK_PLASTIC);
    glPushMatrix();
    glTranslatef(G1_X, G1_Y, MOTOR_Z0);
    cyl_z(MOTOR_R, MOTOR_Z1 - MOTOR_Z0);
    glPopMatrix();
    mat::use(mat::MACHINE_PAINT);
    box_span(G1_X - 0.25f, PANEL_CY + 0.5f*PANEL_H, MOTOR_Z0,
             G1_X + 0.25f, MOTOR_Z1 + 0.55f,        PANEL_CZ + 0.5f*PANEL_D);
    mat::use(mat::SILVER);
    glPushMatrix();                                  // stub into the pinion
    glTranslatef(G1_X, G1_Y, MOTOR_Z1);
    cyl_z(MOTOR_SHAFT_R, 0.14f);
    glPopMatrix();
    glEndList();

    // ---- magazine and exit hood (row 13) ----------------------------------
    glNewList(L(L_FIXTURES), GL_COMPILE);
    mat::use(mat::BLACK_PLASTIC);
    {   // feed magazine: a plate either side of the belt, the blanks drop
        // out between them
        const float mx = lay::station_x(1);
        const float o = 0.5f * MAG_W, i = o - MAG_WALL;
        box_span(mx - o, MAG_Y0, -o, mx + o, MAG_Y1, -i);
        box_span(mx - o, MAG_Y0,  i, mx + o, MAG_Y1,  o);
    }
    // exit hood: one plate over the belt, open all round underneath
    box_span(HOOD_X0, HOOD_Y0, -HOOD_Z_OUT,
             HOOD_X1, HOOD_Y0 + HOOD_TOP_T, HOOD_Z_OUT);
    glEndList();

    // ---- one stack-light lens, used three times ---------------------------
    glNewList(L(L_STACK_SEG), GL_COMPILE);
    cyl(STACK_R, STACK_SEG_H);
    glEndList();
}

inline void draw_stack_light() {
    mat::use(mat::MACHINE_PAINT);
    glPushMatrix();
    glTranslatef(STACK_X, 0.0f, STACK_Z);
    cyl(STACK_POST_R, STACK_POST_H);
    glPopMatrix();
    mat::use(mat::BLACK_PLASTIC);
    glPushMatrix();
    glTranslatef(STACK_X, STACK_POST_H, STACK_Z);
    cyl(STACK_R, STACK_Y0 - STACK_POST_H);
    glPopMatrix();

    glDisable(GL_LIGHTING);                  // the lenses are lit from inside
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
    glEnable(GL_LIGHTING);
}

} // namespace scene
#endif
