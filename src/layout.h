
#ifndef LAYOUT_H
#define LAYOUT_H

#include <cmath>
#include "config.h"

namespace lay {

using namespace cfg;


constexpr float PHI_DEG[5] = { 105.0000f, 270.0000f, 270.3666f,
                               182.5143f, 178.8809f };  // each gear's angle at theta = 90 deg

inline float gear_r(int i) { return MODULE * TEETH[i] * 0.5f; }  // pitch radius

constexpr float POSE_DEG = 90.0f;

inline float wrap180(float d) {
    while (d >   180.0f) d -= 360.0f;
    while (d <= -180.0f) d += 360.0f;
    return d;
}

constexpr float GEAR_RATE[5] = { 3.0f, -1.0f, 1.8f, -1.8f, 1.0f };

inline float gear_deg(int i, float th) {
    return PHI_DEG[i] + GEAR_RATE[i] * (th * DEG - POSE_DEG);
}

inline float ram_top(float th) {
    const float sn = sinf(th);
    return CRANK_Y + CRANK_R * cosf(th)
         - sqrtf(ROD_L * ROD_L - CRANK_R * CRANK_R * sn * sn);
}
inline float punch_face(float th) { return ram_top(th) - RAM_H; }

inline float crank_deg(float th) { return POSE_DEG - th * DEG; }
inline float pin_x(float th) { return PRESS_X + CRANK_R * sinf(th); }
inline float pin_y(float th) { return CRANK_Y + CRANK_R * cosf(th); }

inline float rod_deg(float th) {
    return atan2f(ram_top(th) - pin_y(th), PRESS_X - pin_x(th)) * DEG;
}

inline float gen_lambda()      { return sinf(PI / (float)GEN_SLOTS); }
inline float gen_centre_dist() { return GEN_A / gen_lambda(); }          // c
inline float gen_wheel_r() {
    const float c = gen_centre_dist();
    return sqrtf(c * c - GEN_A * GEN_A);
}

inline float alpha_deg(float th) { return wrap180(th * DEG); }
inline float arm_deg(float th)   { return GEN_LOC_DEG + alpha_deg(th); }

constexpr float ENGAGE_DEG = 90.0f - 180.0f / (float)GEN_SLOTS;

inline float geneva_beta_deg(float a_deg) {
    const float lam = gen_lambda(), a = a_deg * RAD;
    return atan2f(lam * sinf(a), 1.0f - lam * cosf(a)) * DEG;
}

inline float index_progress(float th) {
    const float a = alpha_deg(th);
    if (fabsf(a) > ENGAGE_DEG) return 0.0f;
    return (ENGAGE_DEG + geneva_beta_deg(a)) / (2.0f * ENGAGE_DEG);
}

inline float belt_travel(float th, long turns) {
    const float d = th * DEG;
    float B;
    if (d < ENGAGE_DEG)                B = (float)(turns - 1) + index_progress(th);
    else if (d < 360.0f - ENGAGE_DEG)  B = (float)turns;
    else                               B = (float)turns + index_progress(th);
    return B < 0.0f ? 0.0f : B;        // only before the first index completes
}

// The wheel, and the head roller keyed to it, turn a quarter turn per station.
inline float wheel_deg(float B) { return -(360.0f / (float)GEN_SLOTS) * B; }


inline float pitch()   { return BELT_R_C * PI * 0.5f; }          // 0.628319
inline float span()    { return ROLL_PITCHES * pitch(); }        // 10p
inline float tail_x()  { return HEAD_X - span(); }               // -2.283185
inline float loop_len(){ return CLEAT_N * pitch(); }             // 24p
inline float station_x(int j) { return tail_x() + j * pitch(); }

struct PathPt { float x, y, rot_deg; };


inline PathPt belt_path(float s) {
    const float p = pitch(), Rc = BELT_R_C, Ro = BELT_R_O, L = loop_len();
    s = fmodf(s, L);
    if (s < 0.0f) s += L;

    PathPt q;
    if (s < 10.0f * p) {                       // top run
        q.x = tail_x() + s; q.y = BELT_TOP_Y; q.rot_deg = 0.0f;
    } else if (s < 12.0f * p) {                // around the head roller
        const float phi = 0.5f * PI - (s - 10.0f * p) / Rc;
        q.x = HEAD_X + Ro * cosf(phi);
        q.y = ROLLER_Y + Ro * sinf(phi);
        q.rot_deg = phi * DEG - 90.0f;
    } else if (s < 22.0f * p) {                // bottom run
        q.x = HEAD_X - (s - 12.0f * p);
        q.y = ROLLER_Y - Ro; q.rot_deg = 180.0f;
    } else {                                   // around the tail roller
        const float phi = 1.5f * PI - (s - 22.0f * p) / Rc;
        q.x = tail_x() + Ro * cosf(phi);
        q.y = ROLLER_Y + Ro * sinf(phi);
        q.rot_deg = phi * DEG - 90.0f;
    }
    return q;
}

inline float cleat_s(int k) { return ((float)k + 0.5f) * pitch(); }

inline float blank_h_at(float x, float th) {
    if (x < PRESS_X - 0.02f) return BLANK_H;
    if (x > PRESS_X + 0.02f) return BLANK_H_FLAT;
    return fminf(BLANK_H, fmaxf(BLANK_H_FLAT, punch_face(th) - BELT_TOP_Y));
}

} // namespace lay
#endif
