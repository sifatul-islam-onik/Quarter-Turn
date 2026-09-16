// config.h - every tunable number in "Quarter Turn", in one place.
// Section references are to PRD.md.  Numbers that the PRD *derives* are not
// typed in here; they are computed in kinematics.h and asserted at startup.
#ifndef CONFIG_H
#define CONFIG_H

namespace cfg {

constexpr float PI  = 3.14159265358979f;
constexpr float DEG = 180.0f / PI;
constexpr float RAD = PI / 180.0f;

// ---- Camera (PRD 4.1) ------------------------------------------------------
// The eye is 11.10 from the look-at point.  Nearest geometry inside the
// frustum is 9.23 away and the farthest visible point is 16.76, so near=4.0
// clears everything with orbit margin and far/near is 6.25 rather than the 250
// a habitual near plane of 0.1 would give.  That is the z-fighting margin.
constexpr float EYE_X = 7.0f, EYE_Y = 6.2f, EYE_Z = 9.0f;
constexpr float AT_X  = 1.4f, AT_Y  = 2.9f, AT_Z  = 0.0f;
constexpr float FOVY  = 45.0f, ZNEAR = 4.0f, ZFAR = 25.0f;

// ---- Timing and state (PRD FR-1, FR-2) -------------------------------------
// One crankshaft revolution is one part, so omega is shown as parts/minute.
constexpr float PPM_MIN = 6.0f, PPM_MAX = 120.0f, PPM_DEFAULT = 30.0f;
constexpr float PPM_STEP = 6.0f;
constexpr float DT_CLAMP = 0.1f;         // a window drag must not skip a phase
constexpr float STEP_DEG = 5.0f;         // the '.' key
constexpr float RESET_THETA_DEG = 90.0f; // belt at rest, ram descending, B = 0

// ---- Gear train (PRD FR-3) -------------------------------------------------
// r = m*N/2, and two meshing gears sit exactly r_i + r_j apart.
constexpr float MODULE  = 0.05f;
constexpr float GEAR_Z  = -0.85f;     // all five gears share this plane
constexpr float GEAR_T  = 0.08f;      // back face is 0.06 clear of the panel
constexpr int   TEETH[5] = { 12, 36, 20, 20, 36 };      // G1..G5
constexpr float TOOTH_W_FRAC = 0.45f; // of the circular pitch pi*m; under 1/2
                                      // so block teeth clear at the mesh
constexpr float TOOTH_DEDENDUM = 1.25f; // root radius = r - 1.25m
constexpr float TOOTH_ADDENDUM = 1.00f; // tip  radius = r + 1.00m
constexpr float HUB_R_FRAC = 0.34f, HUB_T = 0.14f;
// Fixed centres.  G3 and G4 are derived from these two (PRD FR-3).
constexpr float G1_X = 2.115f, G1_Y = 5.950f;
constexpr float G2_X = 2.115f, G2_Y = 4.750f;
constexpr float G5_X = 4.550f, G5_Y = 2.040f;
constexpr int   GEAR_SLICES = 28;

// ---- Crank-slider press (PRD FR-4) -----------------------------------------
// s(t) = Y_C + r*cos t - sqrt(L^2 - r^2 sin^2 t);  stroke is exactly 2r = 0.50.
// L/r = 4.0, so the largest rod obliquity is asin(r/L) = 14.48 deg.
constexpr float PRESS_X  = 2.115f;    // station 7, and G1/G2's axis
constexpr float CRANK_R  = 0.25f;     // throw
constexpr float ROD_L    = 1.00f;
constexpr float CRANK_Y  = 4.750f;    // = s_min + r + L
constexpr float MIN_L_OVER_R = 2.5f;  // asserted at startup
constexpr float RAM_W = 0.36f, RAM_H = 0.40f, RAM_D = 0.36f;
constexpr float ROD_W = 0.08f, ROD_D = 0.08f;
constexpr float CRANK_SHAFT_R = 0.10f;
constexpr float CRANK_DISC_R  = 0.35f, CRANK_DISC_T = 0.06f;
constexpr float CRANK_PIN_R   = 0.07f;
constexpr float CRANK_DISC_Z0 = -0.11f; // shaft runs from the panel to here
constexpr float RAIL_X_OFF = 0.24f, RAIL_W = 0.06f, RAIL_D = 0.10f;
constexpr float RAIL_Y0 = 3.20f, RAIL_Y1 = 4.10f;
constexpr float BRACKET_Y0 = 3.60f, BRACKET_Y1 = 3.76f; // under G2's tip, 3.80

// ---- Drive panel, floor, motor (PRD 4.2) -----------------------------------
constexpr float PANEL_CX = 3.2f, PANEL_CY = 2.75f, PANEL_CZ = -1.0f;
constexpr float PANEL_W = 5.2f, PANEL_H = 5.5f, PANEL_D = 0.10f;
constexpr int   PANEL_NX = 26, PANEL_NY = 22;  // subdivided for the spot cone
constexpr float FLOOR_X0 = -4.5f, FLOOR_X1 = 7.5f;
constexpr float FLOOR_Z0 = -1.5f, FLOOR_Z1 = 3.0f;
constexpr int   FLOOR_NX = 48, FLOOR_NZ = 18;
constexpr float MOTOR_R = 0.35f, MOTOR_Z0 = -1.75f, MOTOR_Z1 = -0.95f;
constexpr float MOTOR_SHAFT_R = 0.08f;

// ---- Conveyor, belt, cleats (PRD FR-6) -------------------------------------
constexpr float ROLLER_R = 0.39f, ROLLER_LEN = 1.04f, ROLLER_Y = 2.59f;
constexpr float HEAD_X   = 4.000f;    // head roller axis; station 10
constexpr float BELT_T   = 0.02f, BELT_W = 1.00f;
constexpr float BELT_R_C = 0.40f;     // centreline radius: pitch p = R_c*pi/2
constexpr float BELT_R_O = 0.41f;     // outer surface, where cleats sit
constexpr float BELT_TOP_Y = 3.00f;   // 2.59 + 0.39 + 0.02
constexpr int   ROLL_PITCHES = 10;    // roller centres are exactly 10p apart
constexpr int   CLEAT_N = 24;         // loop is 2D + 2*pi*R_c = 24p
constexpr float CLEAT_W = 0.03f, CLEAT_H = 0.03f, CLEAT_LEN = 0.96f;
constexpr int   BELT_TOP_SEGS = 40;   // subdivided for the spot cone
constexpr int   SHELL_SEGS = 24;
constexpr float CFRAME_Z  = 0.55f;    // side frame rail centre plane
constexpr float CFRAME_W  = 0.08f, CFRAME_H = 0.20f, CFRAME_LEN = 6.70f;
constexpr float CFRAME_Y  = 2.34f;    // top face 2.44, clear under the roller
constexpr float LEG_X0 = -2.00f, LEG_X1 = 3.70f, LEG_W = 0.12f;

// ---- Blanks (PRD FR-6, FR-7) -----------------------------------------------
constexpr float BLANK_R = 0.18f, BLANK_H = 0.20f, BLANK_H_FLAT = 0.10f;
constexpr int   BLANK_SLICES = 14;    // deliberately coarse: FR-12 acceptance
// A broken edge on the polished parts.  Without it nothing in the scene faces
// the half vector and the mandatory specular highlight does not exist at all -
// the reasoning is in prim.h, above cyl().
constexpr float BLANK_CHAMFER = 0.030f;
constexpr float PART_CHAMFER  = 0.025f;   // rollers, crank disc, pins, shafts
constexpr int   PRESS_STATION = 7;    // furthest upstream station the
                                      // two-idler train can reach
constexpr int   LABELS = 9;           // blanks carry labels 1..9
constexpr float FRESH_G = 0.605f;     // reveal once the departing blank has
constexpr float FRESH_SPAN = 0.10f;   // cleared by 2r + 0.02 = 0.38
constexpr float MAG_BOTTOM = 3.24f;   // tube's lower edge, above a blank top

// ---- Geneva mechanism (PRD FR-5) -------------------------------------------
// c = a/sin(pi/n) = 0.7778 and wheel radius sqrt(c^2 - a^2) = 0.55 are both
// derived in kinematics.h; only the free choices are typed here.
constexpr int   GEN_SLOTS = 4;
constexpr float GEN_A = 0.55f;        // driver pin orbit radius
constexpr float GEN_LOC_DEG = 135.0f; // line of centres, driver -> wheel
constexpr float GEN_WHEEL_T = 0.08f;
// Deviation from PRD 4.2, stated: the PRD centres the wheel at z 0.62, 0.06
// clear of the roller's end cap at 0.52 - but the near conveyor frame rail
// also has to live between them, and it occupies 0.51 to 0.59.  The wheel
// therefore sits outboard of the rail at 0.66 to 0.74, still 0.07 clear of the
// nearest same-facing surface, and the roller reaches it through a stub shaft.
constexpr float GEN_WHEEL_Z0 = 0.66f;
constexpr float GEN_SLOT_HW = 0.070f; // slot half width
constexpr float GEN_HUB_R   = 0.175f; // slot bottom; see note in scene.h
constexpr float GEN_PIN_R   = 0.045f;
constexpr float GEN_SHAFT_R = 0.05f;  // leaves 0.318 to the belt's surface
constexpr float GEN_SHAFT_Z0 = -0.95f, GEN_SHAFT_Z1 = 0.88f;
constexpr float GEN_PIN_Z0 = 0.66f, GEN_PIN_Z1 = 0.80f;
constexpr float GEN_ARM_Z0 = 0.80f, GEN_ARM_T = 0.08f, GEN_ARM_W = 0.10f;
constexpr float GEN_STUB_R = 0.09f;   // roller stub through the frame rail
constexpr int   GEN_ARC_SEGS = 10;

// ---- Fixtures (PRD 4.2 row 13) ---------------------------------------------
constexpr float MAG_W = 0.50f, MAG_D = 0.50f, MAG_WALL = 0.06f;
constexpr float MAG_Y0 = 3.24f, MAG_Y1 = 4.60f;
constexpr float HOOD_X0 = 3.67f, HOOD_X1 = 4.45f;  // 0.30 past station 9
constexpr float HOOD_Y0 = 3.50f, HOOD_TOP_T = 0.12f;
constexpr float HOOD_Z_IN = 0.46f, HOOD_Z_OUT = 0.52f;
constexpr float BIN_X0 = 5.0f, BIN_X1 = 6.2f, BIN_Z = 0.60f;
constexpr float BIN_H = 1.00f, BIN_WALL = 0.06f;
// The chute is kept short and narrow and held to the near side: G5 is a
// 0.9-radius gear centred at (4.550, 2.040), so a full-width chute from the
// roller to the bin draws right across the Geneva drive gear.  It is 0.85 in
// front of the gear plane, so this is a composition choice, not a collision.
constexpr float CHUTE_X0 = 4.55f, CHUTE_Y0 = 2.35f;
constexpr float CHUTE_X1 = 5.45f, CHUTE_Y1 = 1.45f;
constexpr float CHUTE_T = 0.06f, CHUTE_Z = 0.50f, CHUTE_ZC = 0.22f;

// ---- Stack light (PRD 4.2 row 14, FR-14) -----------------------------------
constexpr float STACK_X = 5.50f, STACK_Z = -0.80f;
constexpr float STACK_POST_R = 0.05f, STACK_POST_H = 5.50f;
constexpr float STACK_R = 0.10f, STACK_SEG_H = 0.25f;
constexpr float STACK_Y0 = 5.55f;     // green, amber, red upward

// ---- Lighting (PRD FR-10) --------------------------------------------------
// Light 0 - press lamp, a spotlight over the die.  The cone is about 2.4 in
// radius at die height, so the pool covers the press, the stations either side
// of it, and the crank gear behind.  The attenuation coefficients are chosen
// so that the slides' radial formula 1/(a0 + a1 d + a2 d^2) is genuinely in
// play: it gives 0.70 at the die, 5.22 away, and about 0.5 on the floor.
constexpr float SPOT_X = 2.115f, SPOT_Y = 8.0f, SPOT_Z = 1.8f;
constexpr float AIM_X  = 2.115f, AIM_Y  = 3.10f, AIM_Z = 0.0f;
constexpr float SPOT_CUTOFF = 25.0f, SPOT_EXPONENT = 10.0f;
constexpr float SPOT_A0 = 1.0f, SPOT_A1 = 0.03f, SPOT_A2 = 0.01f;

// Light 1 - fill, a plain point light.  Its job is to keep the Geneva wheel,
// the tail of the line and the dark motor readable outside the spot, without
// washing out the cone.  A key toggles it, so the two-light sum can be
// demonstrated as a sum.
constexpr float FILL_X = 9.0f, FILL_Y = 6.0f, FILL_Z = 8.0f;
constexpr float FILL_A0 = 1.0f, FILL_A1 = 0.01f, FILL_A2 = 0.002f;

} // namespace cfg
#endif
