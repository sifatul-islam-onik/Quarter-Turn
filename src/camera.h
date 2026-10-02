#ifndef CAMERA_H
#define CAMERA_H

#include <GL/freeglut.h>
#include <cmath>

#include "config.h"

namespace cam {

using namespace cfg;

inline int   preset = 1;                        // 1 the machine, 2 the room
inline float orbit  = 0.0f;                     // degrees about the look-at
inline float eye_pos[3] = { EYE_X, EYE_Y, EYE_Z };
inline int   win_w = 1180, win_h = 700;

inline bool  free_cam = false;
inline float free_yaw = 0.0f;                   // degrees
enum { MV_FWD = 0, MV_BACK, MV_TURN_L, MV_TURN_R, MV_UP, MV_DOWN, MV_COUNT };
inline bool  held[MV_COUNT] = {};

inline void preset_view(float* eye, float* at) {
    float ex, ey, ez, ax, ay, az;
    if (preset == 2) {                          // the whole room
        ex = OVER_EYE_X; ey = OVER_EYE_Y; ez = OVER_EYE_Z;
        ax = OVER_AT_X;  ay = OVER_AT_Y;  az = OVER_AT_Z;
    } else {                                    // three-quarter, front right
        ex = EYE_X; ey = EYE_Y; ez = EYE_Z;
        ax = AT_X;  ay = AT_Y;  az = AT_Z;
    }
    // Spin the eye about the look-at point by the orbit angle.
    const float c = cosf(orbit * RAD), s = sinf(orbit * RAD);
    const float dx = ex - ax, dz = ez - az;
    eye[0] = ax + dx * c + dz * s;
    eye[1] = ey;
    eye[2] = az - dx * s + dz * c;
    at[0] = ax; at[1] = ay; at[2] = az;
}

inline void forward(float* f) {
    f[0] = sinf(free_yaw * RAD); f[1] = 0.0f; f[2] = -cosf(free_yaw * RAD);
}

// The free camera flies right up to the parts, which the presets' 4.0 near
// plane would cut away, so each mode has its own (config.h).
inline void apply_projection() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOVY, (double)win_w / win_h,
                   free_cam ? FREE_ZNEAR : ZNEAR, ZFAR);
    glMatrixMode(GL_MODELVIEW);
}

inline void set_free(bool on) {
    if (on && !free_cam) {
        float at[3];
        preset_view(eye_pos, at);
        free_yaw = atan2f(at[0] - eye_pos[0], -(at[2] - eye_pos[2])) * DEG;
    }
    free_cam = on;
    apply_projection();
}

// FR-1 again: everything here moves speed * dt rather than a step per frame.
inline void fly(float dt) {
    if (!free_cam) return;
    free_yaw += FREE_TURN * dt * ((float)held[MV_TURN_R] - (float)held[MV_TURN_L]);

    float f[3];
    forward(f);
    const float fwd  = (float)held[MV_FWD] - (float)held[MV_BACK];
    const float rise = (float)held[MV_UP]  - (float)held[MV_DOWN];
    const float step = FREE_SPEED * dt;
    for (int i = 0; i < 3; ++i) eye_pos[i] += step * fwd * f[i];
    eye_pos[1] = fminf(FREE_Y_MAX, fmaxf(FREE_Y_MIN, eye_pos[1] + step * rise));

    // Held inside a cylinder about the room's centre, so it cannot fly off.
    const float cx = 0.5f * (ROOM_X0 + ROOM_X1), cz = 0.5f * (ROOM_Z0 + ROOM_Z1);
    const float dx = eye_pos[0] - cx, dz = eye_pos[2] - cz;
    const float d = sqrtf(dx * dx + dz * dz);
    if (d > FREE_RADIUS) {
        eye_pos[0] = cx + dx * FREE_RADIUS / d;
        eye_pos[2] = cz + dz * FREE_RADIUS / d;
    }
}

inline void apply() {
    float at[3];
    if (free_cam) {
        forward(at);
        for (int i = 0; i < 3; ++i) at[i] += eye_pos[i];
    } else {
        preset_view(eye_pos, at);
    }
    gluLookAt(eye_pos[0], eye_pos[1], eye_pos[2], at[0], at[1], at[2],
              0.0f, 1.0f, 0.0f);
}

} // namespace cam
#endif
