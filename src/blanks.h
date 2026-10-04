#ifndef BLANKS_H
#define BLANKS_H

#include "common.h"

namespace scene {

inline GLuint blank_list;      // one blank, drawn at every station

// The blank rises from y = 0, so a pressed one is the same list scaled about
// its base.
inline void build_blanks() {
    blank_list = new_list();
    cyl(BLANK_R, BLANK_H);
    glEndList();
}

// Stations past the press carry a pressed blank, which is simply shorter.
inline void draw_blank(float x, float y, float h) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(1.0f, h / BLANK_H, 1.0f);
    glCallList(blank_list);
    glPopMatrix();
}

// The blanks ride the belt.  Only the fraction of a station is needed: when B
// passes a whole number every blank has moved up one station, so the blank
// drawn at slot j takes over the place slot j-1 just left.  One reaches the
// head roller, where draw_bin takes it over, and one arrives at station 1.
inline void draw_blanks(float th, float B) {
    mat::use(mat::SILVER);
    const float f = B - floorf(B);
    for (int j = 1; j <= LABELS; ++j) {
        const float x = lay::station_x(j) + f * lay::pitch();
        draw_blank(x, BELT_TOP_Y, lay::blank_h_at(x, th));
    }
}

inline GLuint bin_list;        // the bin past the head roller

// An open box: a floor and four walls.
inline void build_bin() {
    const float x0 = BIN_X - 0.5f * BIN_S, x1 = BIN_X + 0.5f * BIN_S;
    const float z0 = BIN_Z - 0.5f * BIN_S, z1 = BIN_Z + 0.5f * BIN_S;
    const float t = BIN_T;
    bin_list = new_list();
    mat::use(mat::WOOD);
    box_span(x0 + t, 0.0f, z0 + t, x1 - t, t,     z1 - t);     // floor
    box_span(x0,     0.0f, z0,     x1,     BIN_H, z0 + t);     // back
    box_span(x0,     0.0f, z1 - t, x1,     BIN_H, z1);         // front
    box_span(x0,     0.0f, z0 + t, x0 + t, BIN_H, z1 - t);     // left
    box_span(x1 - t, 0.0f, z0 + t, x1,     BIN_H, z1 - t);     // right
    glEndList();
}

// Blank i in the bin, nine to a layer as on the pallets.  At u = 0 it is still
// on top of the head roller, at u = 1 it has landed.  In between it falls:
// down goes as u*u, slow then fast; out, the push off the belt, fast then slow.
inline void draw_in_bin(int i, float u) {
    const float x = BIN_X + (i % 3 - 1) * PAL_PITCH;
    const float y = BIN_T + (i / 9) * BLANK_H_FLAT;
    const float z = BIN_Z + (i / 3 % 3 - 1) * PAL_PITCH;
    const float down = u * u;
    const float out  = 1.0f - (1.0f - u) * (1.0f - u) * (1.0f - u);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, z * out);
    draw_blank(HEAD_X + (x - HEAD_X) * out, BELT_TOP_Y + (y - BELT_TOP_Y) * down,
               BLANK_H_FLAT);
    glPopMatrix();
}

// Every index carries one stamped blank off the head roller, so after B
// indexes the bin holds floor(B) of them.  The next one waits on the roller
// and falls in while the belt indexes; u is the time through that index.
inline void draw_bin(float th, float B) {
    glCallList(bin_list);
    const int full = 9 * BIN_LAYERS;
    int n = (int)floorf(B);
    if (n > full) n = full;                          // full: it stops growing
    mat::use(mat::SILVER);
    for (int i = 0; i < n; ++i) draw_in_bin(i, 1.0f);

    float u = 0.0f;
    if (B > floorf(B))                               // the belt is indexing
        u = (lay::alpha_deg(th) + lay::ENGAGE_DEG) / (2.0f * lay::ENGAGE_DEG);
    draw_in_bin(n < full ? n : full - 1, u);
}

}
#endif
