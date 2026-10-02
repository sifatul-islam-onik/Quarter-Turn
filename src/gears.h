
#ifndef GEARS_H
#define GEARS_H

#include "common.h"

namespace scene {

inline GLuint tooth_list;       // one tooth, drawn 124 times
inline GLuint gear_list[5];     // each gear's body: root disc and hub

inline float gear_root_r(int i) { return lay::gear_r(i) - TOOTH_DEDENDUM * MODULE; }
inline float gear_tip_r (int i) { return lay::gear_r(i) + TOOTH_ADDENDUM * MODULE; }

inline void build_gears() {
    tooth_list = new_list();
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
        gear_list[i] = new_list();
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

// One whole gear at the origin: its body and its ring of teeth.  The spare
// gears on the room's shelf use this too.
inline void gear_shape(int i) {
    glCallList(gear_list[i]);
    const float rr = gear_root_r(i);   // teeth sit TOOTH_SINK inside it,
                                       // so a 10-sided body still holds them
    for (int k = 0; k < TEETH[i]; ++k) {
        glPushMatrix();
        glRotatef(360.0f * k / TEETH[i], 0, 0, 1);
        glTranslatef(rr, 0.0f, 0.0f);
        glCallList(tooth_list);
        glPopMatrix();
    }
}

inline void draw_gears(float th) {
    for (int i = 0; i < 5; ++i) {
        mat::use(i == 1 || i == 3 ? mat::COPPER : mat::BRASS);
        glPushMatrix();
        glTranslatef(GEAR_X[i], GEAR_Y[i], GEAR_Z);
        glRotatef(lay::gear_deg(i, th), 0, 0, 1);
        gear_shape(i);
        glPopMatrix();
    }
}

} // namespace scene
#endif
