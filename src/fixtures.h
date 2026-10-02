
#ifndef FIXTURES_H
#define FIXTURES_H

#include "common.h"

namespace scene {

inline GLuint floor_list;
inline GLuint motor_list;
inline GLuint lens_list;       // one stack-light lens, used three times

inline void build_fixtures() {
    // ---- floor (PRD 4.2 row 1) --------------------------------------------
    floor_list = new_list();
    mat::use(mat::CONCRETE);
    tiles_y(ROOM_X0, ROOM_Z0, ROOM_X1, ROOM_Z1, 0.0f,
            FLOOR_NX, FLOOR_NZ, true);
    glEndList();

    // ---- motor body and mount (row 3) -------------------------------------
    motor_list = new_list();
    mat::use(mat::BLACK_PLASTIC);
    glPushMatrix();
    glTranslatef(G1_X, G1_Y, MOTOR_Z0);
    cyl_z(MOTOR_R, MOTOR_Z1 - MOTOR_Z0);
    glPopMatrix();
    mat::use(mat::MACHINE_PAINT);                    // saddle on the panel top
    box_span(G1_X - 0.25f, PANEL_CY + 0.5f*PANEL_H, MOTOR_Z0 + 0.10f,
             G1_X + 0.25f, G1_Y,                    MOTOR_Z1);
    mat::use(mat::SILVER);
    glPushMatrix();                                  // stub into the pinion
    glTranslatef(G1_X, G1_Y, MOTOR_Z1);
    cyl_z(MOTOR_SHAFT_R, 0.14f);
    glPopMatrix();
    glEndList();

    // ---- one stack-light lens, used three times ---------------------------
    lens_list = new_list();
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

    const float base[3][3] = {{0.10f, 0.85f, 0.20f},
                              {0.95f, 0.65f, 0.05f},
                              {0.90f, 0.12f, 0.10f}};
    for (int i = 0; i < 3; ++i) {
        mat::lens(base[i][0], base[i][1], base[i][2],
                  i == 0);                  // only green is lit: it glows
        glPushMatrix();
        glTranslatef(STACK_X, STACK_Y0 + i * STACK_SEG_H, STACK_Z);
        glCallList(lens_list);
        glPopMatrix();
    }
}

} // namespace scene
#endif
