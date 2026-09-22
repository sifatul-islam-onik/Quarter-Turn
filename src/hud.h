#ifndef HUD_H
#define HUD_H

#include <GL/freeglut.h>
#include <cmath>

#include "config.h"
#include "camera.h"
#include "room.h"

namespace hud {

using namespace cfg;

struct State {
    const bool* sw;         // room::SW_COUNT switch flags
    bool edges;
};

inline void* const SANS = GLUT_BITMAP_HELVETICA_12;


inline void text(float x, float y, const char* s, void* font = SANS) {
    glRasterPos2f(x, y);
    while (*s) glutBitmapCharacter(font, *s++);
}

inline float text_w(const char* s, void* font = SANS) {
    return (float)glutBitmapLength(font, (const unsigned char*)s);
}

inline void text_right(float x_right, float y, const char* s, void* font = SANS) {
    text(x_right - text_w(s, font), y, s, font);
}

// Translucent backing, so the text reads over a bright window or a dark wall.
inline void backing(float x0, float y0, float x1, float y1) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.04f, 0.05f, 0.07f, 0.74f);
    glRectf(x0, y0, x1, y1);
    glDisable(GL_BLEND);
}

inline void dot(float cx, float cy, const float* rgb, bool on) {
    if (on) glColor3fv(rgb); else glColor3f(0.30f, 0.31f, 0.34f);
    glBegin(GL_TRIANGLE_FAN);                   // anticlockwise: survives culling
    glVertex2f(cx, cy);
    for (int k = 0; k <= 16; ++k) {
        const float a = 2.0f * PI * k / 16;
        glVertex2f(cx + 4.0f * cosf(a), cy + 4.0f * sinf(a));
    }
    glEnd();
}

// ---------------------------------------------------------------------------
// The status panel
// ---------------------------------------------------------------------------
// One label / value / key row.
inline void status_row(float x0, float x1, float y, const char* label,
                       const char* value, bool bright, const char* key) {
    const float pad = 12.0f;
    glColor3f(0.72f, 0.74f, 0.78f);
    text(x0 + pad + 14.0f, y, label);
    if (bright) glColor3f(0.96f, 0.96f, 0.94f); else glColor3f(0.48f, 0.50f, 0.54f);
    text_right(x1 - pad - 44.0f, y, value);
    glColor3f(0.56f, 0.58f, 0.62f);
    text_right(x1 - pad, y, key);
}

inline void status_panel(const State& st) {
    static const char* LABEL[room::SW_COUNT] = { "Machine", "Left bulb",
                                                 "Right bulb", "Fan" };
    static const char* KEY[room::SW_COUNT]   = { "space", "[", "]", "f" };

    const float x0 = 12.0f, x1 = x0 + 236.0f, pad = 12.0f, row = 20.0f;
    const float top    = (float)cam::win_h - 12.0f;
    const float y_ttl  = top - pad - 10.0f;
    const float y_sub  = y_ttl - 15.0f;
    const float y_row0 = y_sub - 8.0f - row;        // five rows, always the same
    backing(x0, y_row0 - 4.0f * row - pad + 2.0f, x1, top);

    glColor3f(0.96f, 0.96f, 0.94f);
    text(x0 + pad, y_ttl, "QUARTER TURN");
    glColor3f(0.56f, 0.58f, 0.62f);
    text(x0 + pad, y_sub, st.sw[room::SW_MACHINE] ? "the line is running"
                                                  : "the line is stopped");

    for (int k = 0; k < room::SW_COUNT; ++k) {
        const float y = y_row0 - k * row;
        dot(x0 + pad + 4.0f, y + 4.0f, room::SWITCH_RGB[k], st.sw[k]);
        status_row(x0, x1, y, LABEL[k], st.sw[k] ? "ON" : "OFF", st.sw[k], KEY[k]);
    }
    status_row(x0, x1, y_row0 - room::SW_COUNT * row, "Edges",
               st.edges ? "ON" : "OFF", st.edges, "e");
}

// The key hint
// ---------------------------------------------------------------------------
inline void key_hint() {
    static const char* VIEW[][2] = { { "1-2", "view" },
                                     { "c", "free camera" },
                                     { "left right", "orbit" },
                                     { "r", "reset view" }, { "esc", "quit" } };
    static const char* FREE[][2] = { { "up down", "fly" }, { "left right", "turn" },
                                     { "pgup pgdn", "rise, sink" },
                                     { "c", "exit free camera" }, { "esc", "quit" } };
    const char* (*K)[2] = cam::free_cam ? FREE : VIEW;
    const int n = cam::free_cam ? (int)(sizeof FREE / sizeof FREE[0])
                                : (int)(sizeof VIEW / sizeof VIEW[0]);
    float w = 0.0f;
    for (int i = 0; i < n; ++i)
        w += text_w(K[i][0]) + 5.0f + text_w(K[i][1]) + (i + 1 < n ? 18.0f : 0.0f);
    backing(12.0f, 10.0f, 12.0f + w + 24.0f, 32.0f);
    float x = 24.0f;
    for (int i = 0; i < n; ++i) {
        glColor3f(0.90f, 0.91f, 0.93f);
        text(x, 17.0f, K[i][0]);
        x += text_w(K[i][0]) + 5.0f;
        glColor3f(0.52f, 0.54f, 0.58f);
        text(x, 17.0f, K[i][1]);
        x += text_w(K[i][1]) + 18.0f;
    }
}

// ---------------------------------------------------------------------------
// The whole HUD, in screen space
// ---------------------------------------------------------------------------
inline void draw(const State& st) {
    glDisable(GL_LIGHTING);                     // 1. glColor, not a material
    glDisable(GL_DEPTH_TEST);                   // 2. never behind the machine
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, cam::win_w, 0, cam::win_h);   // 3. raster pos in screen space
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();

    status_panel(st);
    key_hint();

    glPopMatrix();                              // 4. restore both matrices
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

} // namespace hud
#endif
