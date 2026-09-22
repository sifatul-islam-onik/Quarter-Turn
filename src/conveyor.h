
#ifndef CONVEYOR_H
#define CONVEYOR_H

#include "common.h"

namespace scene {

inline void build_conveyor() {
    const float xt = lay::tail_x();

    // ---- frame and legs (row 10) ------------------------------------------
    glNewList(L(L_CONVEYOR), GL_COMPILE);
    mat::use(mat::MACHINE_PAINT);
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
    glNewList(L(L_BELT), GL_COMPILE);
    mat::use(mat::RUBBER);
    box_span(xt,     BELT_TOP_Y - BELT_T, -0.5f*BELT_W,
             HEAD_X, BELT_TOP_Y,           0.5f*BELT_W);
    box_span(xt, ROLLER_Y - BELT_R_O, -0.5f*BELT_W,
             HEAD_X, ROLLER_Y - ROLLER_R, 0.5f*BELT_W);
    for (int e = 0; e < 2; ++e) {              // the wrap at each roller
        glPushMatrix();
        glTranslatef(e ? HEAD_X : xt, ROLLER_Y, -0.5f * BELT_W);
        cyl_z(BELT_R_O, BELT_W);
        glPopMatrix();
    }
    glEndList();

    // ---- one cleat, instanced 24 times ------------------------------------
    glNewList(L(L_CLEAT), GL_COMPILE);
    glPushMatrix();
    glTranslatef(0.0f, 0.5f * CLEAT_H, 0.0f);   // base on the belt surface
    box(CLEAT_W, CLEAT_H, CLEAT_LEN);
    glPopMatrix();
    glEndList();

    // ---- one roller, used at both ends ------------------------------------
    glNewList(L(L_ROLLER), GL_COMPILE);
    glPushMatrix();
    glTranslatef(0, 0, -0.5f * ROLLER_LEN);
    cyl_z(ROLLER_R, ROLLER_LEN);
    glPopMatrix();
    glEndList();
}

inline void draw_conveyor(float B) {
    const float wd = lay::wheel_deg(B);
    const float xt = lay::tail_x();

    mat::use(mat::SILVER);
    for (int e = 0; e < 2; ++e) {
        glPushMatrix();
        glTranslatef(e ? HEAD_X : xt, ROLLER_Y, 0.0f);
        glRotatef(wd, 0, 0, 1);
        glCallList(L(L_ROLLER));
        glPopMatrix();
    }
    glPushMatrix();                                  // stub through the frame
    glTranslatef(HEAD_X, ROLLER_Y, 0.5f * ROLLER_LEN);
    glRotatef(wd, 0, 0, 1);
    cyl_z(GEN_STUB_R, GEN_WHEEL_Z0 - 0.5f * ROLLER_LEN);
    glPopMatrix();

    glPushMatrix();                                  // the wheel itself, built
    glTranslatef(HEAD_X, ROLLER_Y, 0.0f);            // in geneva.h and keyed to
    glRotatef(wd, 0, 0, 1);                          // this roller
    glCallList(L(L_GENEVA));
    glPopMatrix();

    mat::use(mat::SAFETY_YELLOW);                    // 24 cleats on the loop
    for (int k = 0; k < CLEAT_N; ++k) {
        const lay::PathPt q = lay::belt_path(lay::cleat_s(k) + B * lay::pitch());
        glPushMatrix();
        glTranslatef(q.x, q.y, 0.0f);
        glRotatef(q.rot_deg, 0, 0, 1);
        glCallList(L(L_CLEAT));
        glPopMatrix();
    }
}

} // namespace scene
#endif
