#ifndef GENEVA_H
#define GENEVA_H

#include "common.h"

namespace scene {

inline GLuint geneva_list;     // the four-slot wheel

inline void build_geneva() {
    const float R  = lay::gen_wheel_r();
    const float z0 = GEN_WHEEL_Z0, z1 = GEN_WHEEL_Z0 + GEN_WHEEL_T;

    geneva_list = new_list();
    mat::use(mat::BRASS);
    glPushMatrix();
    glTranslatef(0, 0, z0);
    cyl_z(GEN_HUB_R, GEN_WHEEL_T);
    glPopMatrix();
    for (int k = 0; k < GEN_SLOTS; ++k) {
        glPushMatrix();
        glRotatef(45.0f + 360.0f * k / GEN_SLOTS, 0, 0, 1);
        box_span(0.0f, -GEN_ARM_HW, z0,  R, GEN_ARM_HW, z1);
        glPopMatrix();
    }
    glEndList();
}

inline void draw_geneva_driver(float th) {
    mat::use(mat::SILVER);
    glPushMatrix();
    glTranslatef(G5_X, G5_Y, GEN_SHAFT_Z0);
    cyl_z(GEN_SHAFT_R, GEN_SHAFT_Z1 - GEN_SHAFT_Z0);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(G5_X, G5_Y, 0.0f);
    glRotatef(lay::arm_deg(th), 0, 0, 1);
    box_span(-GEN_ARM_W, -0.5f*GEN_ARM_W, GEN_ARM_Z0,
             GEN_A + GEN_ARM_W, 0.5f*GEN_ARM_W, GEN_ARM_Z0 + GEN_ARM_T);
    glPushMatrix();
    glTranslatef(GEN_A, 0.0f, GEN_PIN_Z0);
    cyl_z(GEN_PIN_R, GEN_PIN_Z1 - GEN_PIN_Z0);
    glPopMatrix();
    glPopMatrix();
}

} // namespace scene
#endif
