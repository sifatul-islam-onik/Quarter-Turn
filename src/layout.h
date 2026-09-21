// layout.h - where every part of the line stands.  No OpenGL here, and no
// time: this is the static branch, so nothing in this header takes an angle,
// a speed or a step, and nothing in it is recomputed from one frame to the
// next.  It is evaluated once, at startup.
//
// The animated build derives each frame from a crankshaft angle.  Here every
// number is either fixed by the geometry alone - pitch radii, the gear
// centres, the stations along the belt, the belt path itself - or a single
// constant naming the pose the machine is held in: DRIVE_DEG for the gear
// train, CRANK_DEG for the press, ARM_DEG and WHEEL_DEG for the Geneva drive.
//
// That pose is the one the animated build starts from: the belt parked
// between indexes, the ram part way down and still clear of the blanks, and
// the driver's pin outside the wheel's slot, so the wheel is locked.
#ifndef LAYOUT_H
#define LAYOUT_H

#include <cmath>
#include "config.h"

namespace lay {

using namespace cfg;

// ===========================================================================
// FR-3  Gear train
// ===========================================================================

struct Gear {
    int   N;
    float r;        // pitch radius, m*N/2
    float cx, cy;   // centre, in the gear plane z = GEAR_Z
};

// Where G2 - the gear on the crankshaft, the one the motor drives - stands in
// this pose.  It is the one angle chosen here; the other four follow from it
// and from the meshes.
constexpr float DRIVE_DEG = -90.0f;

// Angles follow the glRotatef convention: anticlockwise positive about +z as
// seen from the front.  Tooth 0 lies along the gear's local +x axis.
//
//   phi_j = -(Ni/Nj)*(phi_i - psi_ij) + psi_ij + pi + pi/Nj
//
// This is not a motion: it is what "meshed" means, written down once.  The
// first term is the tooth-count ratio and the reversal at the mesh; psi_ij +
// pi turns j's frame to face back along the line of centres; pi/Nj is the
// half tooth that puts a *gap* opposite i's tooth instead of another tooth.
// Omitting that last term leaves every pair of teeth interpenetrating, which
// reads as a modelling error rather than a maths error (PRD 8).
//
// Solving it here rather than typing five angles in is what keeps the teeth
// meshed if the layout is ever changed.
inline void mesh_angles(const Gear* g, const int* drivenBy, float drive,
                        float* phi) {
    // Dependency order: G2 is the root, then G1, G3, G4, G5.
    static const int order[5] = { 1, 0, 2, 3, 4 };
    for (int k = 0; k < 5; ++k) {
        const int j = order[k];
        const int i = drivenBy[j];
        if (i < 0) { phi[j] = drive; continue; }
        const float psi = atan2f(g[j].cy - g[i].cy, g[j].cx - g[i].cx);
        phi[j] = -((float)g[i].N / (float)g[j].N) * (phi[i] - psi)
                 + psi + PI + PI / (float)g[j].N;
    }
}

// ===========================================================================
// Layout - the numbers the PRD derives rather than chooses
// ===========================================================================

struct Layout {
    Gear  g[5];
    int   drivenBy[5];
    float phi[5];           // each gear's angle in this pose, radians
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

    mesh_angles(g, drivenBy, DRIVE_DEG * RAD, phi);
}

inline const Layout LAY;

// ===========================================================================
// FR-4  Crank-slider press, standing where this pose leaves it
// ===========================================================================

// The crank disc's frame, and the crank pin it carries.  In this pose the
// frame is unrotated, which puts the pin a throw to the +x side of the shaft.
constexpr float CRANK_DEG = 0.0f;
constexpr float PIN_X = PRESS_X + CRANK_R;
constexpr float PIN_Y = CRANK_Y;

// The wrist pin, which is the ram's top face.  The rod runs from the crank pin
// down to the ram's centreline, so with the pin exactly to one side the ram
// hangs sqrt(L^2 - r^2) below the shaft: 3.78, between the 4.00 at the top of
// the stroke and the 3.50 at the bottom, and clear of a 0.20 blank whose top
// is at 3.40.
inline float ram_top()    { return CRANK_Y - sqrtf(ROD_L * ROD_L
                                                 - CRANK_R * CRANK_R); }
inline float punch_face() { return ram_top() - RAM_H; }

// The rod, placed from both of its endpoints - the crank pin and the wrist
// pin.  Its length is exactly ROD_L, because that is what ram_top() solves.
inline float rod_deg() {
    return atan2f(ram_top() - PIN_Y, PRESS_X - PIN_X) * DEG;
}

// ===========================================================================
// FR-5  Geneva drive
// ===========================================================================

// The wheel's own shape: a 4-slot wheel whose centre distance is set by the
// driver's crank radius, c = a/sin(pi/n), and whose radius follows from it.
// Half the driver's engaged sweep is 90 - 180/n degrees, 45 for four slots.
inline float engage_half()     { return 0.5f * PI - PI / (float)GEN_SLOTS; }
inline float gen_lambda()      { return sinf(PI / (float)GEN_SLOTS); }
inline float gen_centre_dist() { return GEN_A / gen_lambda(); }          // c
inline float gen_wheel_r() {
    const float c = gen_centre_dist();
    return sqrtf(c * c - GEN_A * GEN_A);
}

// The driver arm is keyed to G5's shaft.  In this pose it lies 90 deg past the
// line of centres to the wheel (GEN_LOC_DEG), which puts its pin outside the
// slot: the wheel is locked by the driver's disc, which is why the belt is
// parked and every blank sits square on its station.
constexpr float ARM_DEG   = GEN_LOC_DEG + 90.0f;
// The head roller and the wheel share one angle.  The belt is parked at a
// whole number of stations, so that angle is zero.
constexpr float WHEEL_DEG = 0.0f;

// ===========================================================================
// FR-6  Belt path, cleats, stations
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
inline float cleat_s(int k) { return ((float)k + 0.5f) * pitch(); }

// ===========================================================================
// FR-7  The blanks on the belt
// ===========================================================================

// Station 7 is the press.  Everything upstream of it is still a full blank;
// everything downstream has been through the punch and is flat.  The blank
// under the punch is untouched in this pose - the ram is on its way down and
// has not reached it - so it is full height too.
inline float blank_h(int label) {
    return label <= PRESS_STATION ? BLANK_H : BLANK_H_FLAT;
}

// Volume-preserving squash: q = h/h0, scaled by (1/sqrt q, q, 1/sqrt q) about
// the blank's base.  This is the only glScalef in the frame (PRD 4.3).
inline float squash_q(float h) { return h / BLANK_H; }

// ===========================================================================
// Indications
// ===========================================================================

// green, amber, red upward.  Green: the press is on its approach with the belt
// locked, which is what this pose shows.
constexpr int  STACK_LIT  = 0;
// The counter on the wall, which in the animated build counts finished parts.
constexpr long PARTS_MADE = 0;

} // namespace lay
#endif
