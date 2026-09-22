
#ifndef GEARS_H
#define GEARS_H

#include "common.h"

namespace scene {


inline float gear_root_r(int i) { return LAY.g[i].r - TOOTH_DEDENDUM * MODULE; }
inline float gear_tip_r (int i) { return LAY.g[i].r + TOOTH_ADDENDUM * MODULE; }

inline void build_gears() {

    glNewList(L(L_TOOTH), GL_COMPILE);
    {

        const float radial = (TOOTH_ADDENDUM + TOOTH_DEDENDUM) * MODULE;
        const float w = TOOTH_W_FRAC * PI * MODULE;
        glPushMatrix();
        glTranslatef(0.5f * (radial - TOOTH_SINK), 0.0f, 0.0f);
        box(radial + TOOTH_SINK, w, GEAR_T);
        glPopMatrix();
    }
    glEndList();

    for (int i = 0; i < 5; ++i) {
        glNewList(L(L_GEAR0 + i), GL_COMPILE);
        glPushMatrix();
        glTranslatef(0, 0, -0.5f * GEAR_T);
        cyl_z(gear_root_r(i), GEAR_T);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0, 0, -0.5f * HUB_T);
        cyl_z(HUB_R_FRAC * gear_root_r(i), HUB_T);
        glPopMatrix();
        glEndList();
    }
}

inline void draw_gears(float th) {
    for (int i = 0; i < 5; ++i) {
        mat::use(i == 1 || i == 3 ? mat::COPPER : mat::BRASS);
        glPushMatrix();
        glTranslatef(LAY.g[i].cx, LAY.g[i].cy, GEAR_Z);
        glRotatef(lay::gear_deg(i, th), 0, 0, 1);
        glCallList(L(L_GEAR0 + i));
        const float rr = gear_root_r(i);   // teeth sit TOOTH_SINK inside it,
                                           // so a 10-sided body still holds them
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

} // namespace scene
#endif
