// kinematics.h - the closed-form motion of the whole line.  No OpenGL here, so
// tests/mathcheck.cpp verifies the *same* code the renderer runs (PRD 10).
//
// PRD FR-2: the entire animation state is `theta` (crankshaft angle, radians,
// clockwise seen from the front) plus `cycles` (an integer).  Everything in
// this header is a pure function of those two and the constants in config.h.
#ifndef KINEMATICS_H
#define KINEMATICS_H

#include <cmath>
#include "config.h"

namespace kin {

using namespace cfg;

inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
inline float wrap2pi(float a) {
    while (a >= 2.0f * PI) a -= 2.0f * PI;
    while (a < 0.0f)       a += 2.0f * PI;
    return a;
}
// Wrap to (-180, 180].  The Geneva driver's angle from the line of centres.
inline float wrap180(float d) {
    while (d >   180.0f) d -= 360.0f;
    while (d <= -180.0f) d += 360.0f;
    return d;
}

// ===========================================================================
// FR-3  Gear train
// ===========================================================================

struct Gear {
    int   N;
    float r;        // pitch radius, m*N/2
    float cx, cy;   // centre, in the gear plane z = GEAR_Z
};

// Angles follow the glRotatef convention: anticlockwise positive about +z as
// seen from the front.  Tooth 0 lies along the gear's local +x axis.
//
//   phi_j = -(Ni/Nj)*(phi_i - psi_ij) + psi_ij + pi + pi/Nj
//
// The first term is the speed ratio and the reversal at the mesh; psi_ij + pi
// turns j's frame to face back along the line of centres; pi/Nj is the half
// tooth that puts a *gap* opposite i's tooth instead of another tooth.
// Omitting that last term leaves every pair of teeth interpenetrating for all
// time, which reads as a modelling error rather than a maths error (PRD 8).
inline void gear_angles(const Gear* g, const int* drivenBy, float theta,
                        float* phi) {
    // Dependency order: G2 is the root (phi2 = -theta), then G1, G3, G4, G5.
    static const int order[5] = { 1, 0, 2, 3, 4 };
    for (int k = 0; k < 5; ++k) {
        const int j = order[k];
        const int i = drivenBy[j];
        if (i < 0) { phi[j] = -theta; continue; }
        const float psi = atan2f(g[j].cy - g[i].cy, g[j].cx - g[i].cx);
        phi[j] = -((float)g[i].N / (float)g[j].N) * (phi[i] - psi)
                 + psi + PI + PI / (float)g[j].N;
    }
}

// ===========================================================================
// FR-4  Crank-slider press
// ===========================================================================

// Height of the wrist pin, which is the ram's top face.  Max 4.00 at theta=0,
// min 3.50 at theta=pi; the stroke is exactly 2r because sin theta vanishes at
// both extremes and the square root equals L at each.
inline float ram_top(float th) {
    const float s = sinf(th);
    return CRANK_Y + CRANK_R * cosf(th)
         - sqrtf(ROD_L * ROD_L - CRANK_R * CRANK_R * s * s);
}
inline float punch_face(float th) { return ram_top(th) - RAM_H; }

inline void crank_pin(float th, float* x, float* y) {
    *x = PRESS_X + CRANK_R * sinf(th);   // at the top when theta = 0, then
    *y = CRANK_Y + CRANK_R * cosf(th);   // clockwise as theta increases
}

// ===========================================================================
// FR-5  Geneva indexing and the press interlock
// ===========================================================================

// Half the driver's engaged sweep: 90 - 180/n degrees, 45 for a 4-slot wheel.
inline float engage_half() { return 0.5f * PI - PI / (float)GEN_SLOTS; }
inline float gen_lambda()  { return sinf(PI / (float)GEN_SLOTS); }
inline float gen_centre_dist() { return GEN_A / gen_lambda(); }          // c
inline float gen_wheel_r() {
    const float c = gen_centre_dist();
    return sqrtf(c * c - GEN_A * GEN_A);
}

// Wheel rotation during engagement, |alpha| <= engage_half().
inline float geneva_beta(float alpha) {
    const float lam = gen_lambda();
    return atan2f(lam * sinf(alpha), 1.0f - lam * cosf(alpha));
}

// cos(theta) at which the punch face touches the top of an *unstamped* blank.
// Derived, not typed: substituting s(theta) - RAM_H = blank top into the
// crank-slider solution and squaring once gives a linear equation in cos.
// Change the throw, the rod, the ram or the belt height and this moves with
// them - which is the whole point (PRD 8, "the interlock silently violating
// its own rule").
inline float blank_zone_cos() {
    const float T = BELT_TOP_Y + BLANK_H + RAM_H;   // 3.60
    const float A = CRANK_Y - T;                    // 1.15
    return (ROD_L * ROD_L - CRANK_R * CRANK_R - A * A) / (2.0f * A * CRANK_R);
}
inline float blank_zone_lo_deg() { return acosf(blank_zone_cos()) * DEG; }
inline float blank_zone_hi_deg() { return 360.0f - blank_zone_lo_deg(); }

enum Phase { PH_INDEX = 0, PH_APPROACH, PH_STAMP, PH_RETREAT };
inline const char* phase_name(Phase p) {
    static const char* n[4] = { "INDEX", "APPROACH", "STAMP", "RETREAT" };
    return n[p];
}
inline Phase phase_of(float th) {
    const float d  = th * DEG;
    const float E  = engage_half() * DEG;          // 45
    const float lo = blank_zone_lo_deg();          // 132.03
    if (d >= 360.0f - E || d < E) return PH_INDEX;
    if (d < lo)                   return PH_APPROACH;
    if (d < 360.0f - lo)          return PH_STAMP;
    return PH_RETREAT;
}
inline bool belt_locked(Phase p) { return p != PH_INDEX; }

// Fraction of the current index completed, 0 outside INDEX.  The comparison is
// strict on both sides so that B stays continuous at exactly alpha = +/-45 deg.
inline float index_progress(float th) {
    const float E = engage_half();
    const float a = wrap180(th * DEG) * RAD;
    if (fabsf(a) >= E) return 0.0f;
    return (geneva_beta(a) + E) / (2.0f * E);
}

// Belt travel in pitches.  I counts indices completed at the END of INDEX, not
// at the wrap: INDEX straddles theta = 0, which is where `cycles` increments,
// so taking I = cycles directly would jump the belt a whole pitch at top dead
// centre once per part - at the one moment the belt is visibly moving (FR-5).
inline float belt_travel(float th, long cycles) {
    const float E = engage_half() * DEG;
    long I = (th * DEG < E) ? cycles - 1 : cycles;
    if (I < 0) I = 0;          // unreachable from the reset state; a guard only
    return (float)I + index_progress(th);
}

// The head roller and the Geneva wheel share this angle, glRotatef convention.
inline float wheel_angle_deg(float B) { return -(360.0f / GEN_SLOTS) * B; }

// ===========================================================================
// FR-6  Belt path, cleats, blanks
// ===========================================================================

inline float pitch()   { return BELT_R_C * PI * 0.5f; }          // 0.628319
inline float span()    { return ROLL_PITCHES * pitch(); }        // 10p
inline float tail_x()  { return HEAD_X - span(); }               // -2.283185
inline float loop_len(){ return CLEAT_N * pitch(); }             // 24p
inline float station_x(int j) { return tail_x() + j * pitch(); }

struct PathPt { float x, y, rot_deg; };

// Arclength s runs along the belt centreline from the top of the tail roller,
// moving +x.  Points are placed on the outer surface, radius BELT_R_O.  By 4.3
// every piece of the path is a whole number of pitches long.
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

// Cleat k sits half a pitch off the stations, so a blank never overlaps one.
inline float cleat_s(int k, float B) {
    return fmodf((B + (float)k + 0.5f) * pitch(), loop_len());
}

// ===========================================================================
// FR-7  Stamping
// ===========================================================================

// Only the blank at the press changes height, and only as a function of theta.
// Every boundary is continuous: at 45 deg it is 0.20 (the fresh arrival), from
// 132.03 deg it flattens exactly as far as the punch descends, and from 180
// deg it stays flat through the index that carries it away.
inline float press_blank_h(float th) {
    const float d = th * DEG;
    if (d >= engage_half() * DEG && d < 180.0f)
        return clampf(punch_face(th) - BELT_TOP_Y, BLANK_H_FLAT, BLANK_H);
    return BLANK_H_FLAT;
}
inline float blank_h(int label, float th) {
    if (label <  PRESS_STATION) return BLANK_H;
    if (label == PRESS_STATION) return press_blank_h(th);
    return BLANK_H_FLAT;
}
// Volume-preserving squash: q = h/h0, scaled by (1/sqrt q, q, 1/sqrt q) about
// the blank's base.  This is the only glScalef in the frame (PRD 4.3).
inline float squash_q(float h) { return h / BLANK_H; }

// A fresh blank is revealed at station 1 once the departing blank has cleared
// it by 2r + 0.02 = 0.38, then drops from inside the magazine tube onto the
// belt.  At g = 1 it is exactly where label 1 is drawn after the index, so the
// handover is seamless and never visible.
inline bool fresh_visible(float g) { return g >= FRESH_G; }
inline float fresh_blank_y(float g) {
    return MAG_BOTTOM - (MAG_BOTTOM - BELT_TOP_Y)
                        * clampf((g - FRESH_G) / FRESH_SPAN, 0.0f, 1.0f);
}

// ===========================================================================
// Layout - the numbers the PRD derives rather than chooses
// ===========================================================================

struct Layout {
    Gear  g[5];
    int   drivenBy[5];
    float arm_offset_deg;   // driver arm angle - phi5, constant (FR-5)
    Layout();
};

inline Layout::Layout() {
    for (int i = 0; i < 5; ++i) {
        g[i].N = TEETH[i];
        g[i].r = MODULE * TEETH[i] * 0.5f;
    }
    g[0].cx = G1_X; g[0].cy = G1_Y;      // fixed by the motor over the panel
    g[1].cx = G2_X; g[1].cy = G2_Y;      // fixed by the press stack (FR-4)
    g[4].cx = G5_X; g[4].cy = G5_Y;      // fixed by the Geneva geometry (FR-5)

    // G3 and G4 are not placed by eye.  The chain of centre distances
    // 1.40 + 1.00 + 1.40 is laid out symmetrically between G2 and G5; because
    // the two outer links are equal, the middle one is parallel to the line of
    // centres and the whole chain bends 19.26 deg off it, upwards.
    const float d1 = g[1].r + g[2].r;      // 1.40
    const float d2 = g[2].r + g[3].r;      // 1.00
    const float dx = g[4].cx - g[1].cx, dy = g[4].cy - g[1].cy;
    const float D  = sqrtf(dx * dx + dy * dy);
    const float ux = dx / D, uy = dy / D;
    const float nx = -uy,    ny = ux;      // left normal: bends the chain up
    const float along = 0.5f * (D - d2);
    const float off   = sqrtf(d1 * d1 - along * along);
    g[2].cx = g[1].cx + along * ux + off * nx;
    g[2].cy = g[1].cy + along * uy + off * ny;
    g[3].cx = g[2].cx + d2 * ux;
    g[3].cy = g[2].cy + d2 * uy;

    drivenBy[0] = 1; drivenBy[1] = -1; drivenBy[2] = 1;
    drivenBy[3] = 2; drivenBy[4] = 3;

    // The driver arm is keyed to G5's shaft, so arm - phi5 is a constant.
    // Compute it once from theta = 0 rather than typing a number in, so the
    // arm cannot drift out of step with its own gear (FR-5).
    float phi[5];
    gear_angles(g, drivenBy, 0.0f, phi);
    arm_offset_deg = GEN_LOC_DEG - phi[4] * DEG;
}

inline const Layout LAY;

// The Geneva driver's angle from the line of centres.  Because the arm is at
// GEN_LOC_DEG + theta, alpha is simply theta wrapped, and the index is centred
// on the press's top dead centre.
inline float driver_arm_deg(const float* phi) {
    return phi[4] * DEG + LAY.arm_offset_deg;
}

} // namespace kin
#endif
