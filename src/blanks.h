#ifndef BLANKS_H
#define BLANKS_H

#include "common.h"

namespace scene {

// The blank rises from y = 0, so a pressed one is the same list scaled about
// its base.
inline void build_blanks() {
    glNewList(L(L_BLANK), GL_COMPILE);
    cyl(BLANK_R, BLANK_H);
    glEndList();
}

// Stations past the press carry a pressed blank, which is simply shorter.
inline void draw_blank(float x, float y, float h) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(1.0f, h / BLANK_H, 1.0f);
    glCallList(L(L_BLANK));
    glPopMatrix();
}

// The blanks ride the belt.  Only the fraction of a station is needed: when B
// passes a whole number every blank has moved up one station, so the blank
// drawn at slot j takes over the place slot j-1 just left.  One leaves at the
// head roller and one arrives from the magazine, and nothing in between moves.
inline void draw_blanks(float th, float B) {
    mat::use(mat::SILVER);
    const float f = B - floorf(B);
    for (int j = 1; j <= LABELS; ++j) {
        const float x = lay::station_x(j) + f * lay::pitch();
        draw_blank(x, BELT_TOP_Y, lay::blank_h_at(x, th));
    }
}

}
#endif
