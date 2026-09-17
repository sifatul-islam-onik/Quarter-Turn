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
// frustum is 8.9 away (the floor at the bottom edge), so near=4.0 clears
// everything with orbit margin.  The room put the far plane out: the overview
// preset (key 4) puts the furthest room corner at a depth of 29 (30 at worst
// while orbiting), so far is 40 and far/near is 10 - still 25 times better
// than the 250 a habitual near plane of 0.1 would give.  That is the
// z-fighting margin.
constexpr float EYE_X = 7.0f, EYE_Y = 6.2f, EYE_Z = 9.0f;
constexpr float AT_X  = 1.4f, AT_Y  = 2.9f, AT_Z  = 0.0f;
constexpr float FOVY  = 45.0f, ZNEAR = 4.0f, ZFAR = 40.0f;
// Overview preset: far enough back that the whole room, ceiling included, is
// in frame.  Outside the room in plan for every orbit angle.
constexpr float OVER_EYE_X = 14.5f, OVER_EYE_Y = 7.0f, OVER_EYE_Z = 17.0f;
constexpr float OVER_AT_X  = 1.5f,  OVER_AT_Y  = 3.6f, OVER_AT_Z  = 1.0f;
// Free camera (key c).  It flies right up to the parts, so it has its own near
// plane.  It is held inside a cylinder about the room's centre, wide enough for
// the orbited overview eye (21.1), and below FREE_Y_MAX, which keeps every room
// corner within a depth of 36.5 - inside the far plane.  There a 24-bit depth
// step at near 0.2 is 0.0004, so the hazard marks 0.004 proud of the floor
// still sit ten steps clear.  far/near is 200 in this mode.
constexpr float FREE_ZNEAR = 0.2f;
constexpr float FREE_SPEED = 3.0f;          // units per second
constexpr float FREE_LOOK  = 0.25f;         // degrees per pixel of mouse drag
constexpr float FREE_PITCH_MAX = 89.0f;     // at 90 gluLookAt's up is degenerate
constexpr float FREE_RADIUS = 22.0f;
constexpr float FREE_Y_MIN = 0.3f, FREE_Y_MAX = 20.0f;

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
constexpr int   PANEL_NX = 26, PANEL_NY = 22;  // subdivided: lit per vertex
// The floor fills the room (see ROOM_* below) at the original 0.25 cell.
constexpr float FLOOR_X0 = -6.0f, FLOOR_X1 = 9.0f;
constexpr float FLOOR_Z0 = -2.5f, FLOOR_Z1 = 5.5f;
constexpr int   FLOOR_NX = 60, FLOOR_NZ = 32;
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
constexpr int   BELT_TOP_SEGS = 40;   // subdivided: lit per vertex
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
constexpr float HOOD_X0 = 3.67f, HOOD_X1 = 4.45f;  // 0.30 past station 9;
                                                   // open at the far end
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

// Finished parts.  The belt carries each one over the head roller, where it
// tips off at EXIT_TIP_DEG of wrap and is tossed onto the chute, slides down
// and drops into the bin.  A part rests at station 10 while the belt is
// locked, rides and lands on the chute during the next index, then slides and
// drops before the index after that.  Times are in crank revolutions, so the
// exit is keyed to theta like every other motion and scales with the speed.
constexpr float EXIT_TIP_DEG = 40.0f;
constexpr float EXIT_TOSS  = 0.35f;   // reach of the toss along the roller's tangent
constexpr float EXIT_SLIDE = 0.30f, EXIT_DROP = 0.22f;
// The bin piles parts round-robin in 2 x 2 columns, 9 high: 36 parts.  Past
// that it stays full, and each new part lands on the top of the pile.
constexpr int   BIN_COLS = 2, BIN_LEVELS = 9;
constexpr float BIN_PITCH = 0.54f, BIN_JITTER = 0.01f;

// ---- Stack light (PRD 4.2 row 14, FR-14) -----------------------------------
constexpr float STACK_X = 5.50f, STACK_Z = -0.80f;
constexpr float STACK_POST_R = 0.05f, STACK_POST_H = 5.50f;
constexpr float STACK_R = 0.10f, STACK_SEG_H = 0.25f;
constexpr float STACK_Y0 = 5.55f;     // green, amber, red upward

// ---- Lighting: two hanging bulbs (replaces PRD FR-10's rig; see README) ----
// Two point lights, one each side of the press and mirrored about it, each
// drawn as the bulb that gives it off and each on its own switch.  They hang
// 1.0 in front of the belt and 2.4 above it, which does two jobs:
//  - both stay inside the default frame, clear of the HUD and the gear train;
//  - a blank passing under a bulb sees it 50 to 65 degrees up, so the half
//    vector to the camera lands within a few degrees of the blank's 45-degree
//    chamfer and the specular highlight appears, then slides off as it moves.
// Bulbs out at the side walls were tried first: seen from the belt they sit
// under 30 degrees up, and no surface in the scene faced the half vector.
constexpr float BULB_X[2] = { -0.57f, 4.80f };     // light 0 left, light 1 right
constexpr float BULB_Y = 5.40f, BULB_Z = 1.00f;    // centre of the glass
// The slides' radial formula 1/(a0 + a1 d + a2 d^2) stays in play: about 0.9
// on the blanks under a bulb, 2.5 away, and under 0.5 in the room's far corners.
constexpr float BULB_A0 = 1.0f, BULB_A1 = 0.03f, BULB_A2 = 0.008f;
constexpr float BULB_R = 0.12f;       // glass globe
constexpr float GLOW_R = 0.45f;       // additive halo round a lit bulb

// ---- The room (not in the PRD; see README, "Deviations") --------------------
// A cutaway building.  A wall, and everything mounted on it, is drawn only
// while the eye is on the room side of that wall, so the walls between the
// camera and the machine vanish as it orbits.  In plan the furthest room corner
// is 9.4 from the look-at point and the default eye 10.6, so the eye stays
// outside the room at every orbit angle.
constexpr float ROOM_X0 = -6.0f, ROOM_X1 = 9.0f;   // = the floor
constexpr float ROOM_Z0 = -2.5f, ROOM_Z1 = 5.5f;
constexpr float CEIL_Y  = 9.5f;
constexpr float WALL_CELL = 0.5f;     // walls and ceiling are lit per vertex too
constexpr float DADO_H = 1.20f, TRIM_H = 0.06f;    // concrete plinth band

// Windows: y is the sill.  The glass glows (emission) and lights nothing.
constexpr float WIN_Y0 = 5.20f, WIN_H = 1.80f, WIN_W = 1.80f;
constexpr float WIN_FRAME = 0.08f, WIN_DEPTH = 0.10f, WIN_BAR = 0.05f;

// Floor markings: a dashed hazard border round the machine.  0.004 proud of
// the floor is thousands of depth-buffer steps at this near plane.
constexpr float HAZ_X0 = -3.20f, HAZ_X1 = 6.60f;
constexpr float HAZ_Z0 = -1.80f, HAZ_Z1 = 1.50f;
constexpr float HAZ_W = 0.10f, HAZ_DASH = 0.40f, HAZ_Y = 0.004f;

// Left wall (x = ROOM_X0), positions as world z: workbench, tool board and a
// shelf of spare gears, an extinguisher, a roll-up door, two windows.  The back
// wall's left half is hidden behind the conveyor's tail from the default view,
// which is why the bench is here.
constexpr float BENCH_Z0 = -2.30f, BENCH_Z1 = -0.30f, BENCH_D = 0.75f;
constexpr float BENCH_H = 0.98f, BENCH_TOP_T = 0.06f, BENCH_LEG = 0.07f;
constexpr float PEG_Y0 = 1.30f, PEG_Y1 = 2.50f;
constexpr float SHELF_Y = 2.90f, SHELF_D = 0.34f;
constexpr float EXT_Z = 0.15f, EXT_R = 0.11f, EXT_H = 0.60f;
constexpr float DOOR_Z0 = 0.70f, DOOR_Z1 = 3.70f, DOOR_H = 3.70f;
constexpr int   DOOR_SLATS = 18;
constexpr float LWIN_Z[2] = { -1.30f, 2.20f };

// Back wall (z = ROOM_Z0), positions as world x.
constexpr float BWIN_X[2] = { -4.50f, 7.60f };
constexpr float FAN_X = -1.70f, FAN_Y = 5.00f, FAN_SIZE = 1.00f;
// The fan has its own switch, so it is not geared to theta: it runs at
// FAN_RPS and spins up or runs down with a first-order lag of FAN_TAU seconds.
constexpr int   FAN_BLADES = 6;
constexpr float FAN_RPS = 2.5f, FAN_TAU = 0.8f;
constexpr float COUNTER_X = 4.50f, COUNTER_Y = 6.45f;   // over the panel, clear
                                      // of the HUD text in views 1 and 2
constexpr int   COUNTER_DIGITS = 4;
constexpr float DIG_W = 0.30f, DIG_H = 0.52f, SEG_T = 0.06f, DIG_PITCH = 0.44f;
constexpr float PIPE_R = 0.09f, PIPE_Y[2] = { 8.30f, 8.65f }, PIPE_OFF = 0.20f;
constexpr float RISER_X = 8.80f;      // vertical pipe down the right-hand end
constexpr float CONDUIT_X = 6.40f;    // cabinet up to the pipe run
constexpr float BREAKER_X0 = 8.00f, BREAKER_X1 = 8.60f;

// Right wall (x = ROOM_X1) as world z, front wall (z = ROOM_Z1) as world x.
constexpr float RWIN_Z[2] = { -0.40f, 3.00f };
constexpr float FWIN_X[3] = { -3.00f, 1.50f, 6.00f };

// Free-standing on the floor.
constexpr float CAB_X0 = 6.20f, CAB_X1 = 7.60f, CAB_H = 2.40f, CAB_D = 0.51f;
constexpr float PAL_X = -4.10f, PAL_Z = 2.50f, PAL_S = 1.20f;  // feedstock
constexpr float PAL_PITCH = 0.38f;    // 3 x 3 blanks per layer
constexpr int   PAL_LAYERS = 2;
constexpr float DRUM_R = 0.30f, DRUM_H = 0.88f;
constexpr float DRUM_X[2] = { 7.25f, 8.05f }, DRUM_Z[2] = { -1.25f, -0.95f };

// Ceiling: I-beams running front to back.
constexpr float BEAM_X0 = -3.90f, BEAM_PITCH = 3.00f;
constexpr int   BEAM_N = 5;
constexpr float BEAM_D = 0.45f, BEAM_W = 0.30f, BEAM_T = 0.05f;

// Switches on the cabinet door, left to right: machine, left bulb, right bulb,
// fan.  Two each side of the door seam, with a status lamp over each.
constexpr float SWITCH_X[4] = { 6.40f, 6.66f, 7.14f, 7.40f };
constexpr float SWITCH_Y = 1.76f;

} // namespace cfg
#endif
