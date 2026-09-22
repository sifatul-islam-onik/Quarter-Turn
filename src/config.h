#ifndef CONFIG_H
#define CONFIG_H

namespace cfg {

constexpr float PI  = 3.14159265358979f;
constexpr float DEG = 180.0f / PI;
constexpr float RAD = PI / 180.0f;

constexpr int   CYL_SLICES = 10;

constexpr float EYE_X = 7.0f, EYE_Y = 6.2f, EYE_Z = 9.0f;
constexpr float AT_X  = 1.4f, AT_Y  = 2.9f, AT_Z  = 0.0f;
constexpr float FOVY  = 45.0f, ZNEAR = 4.0f, ZFAR = 40.0f;

constexpr float OVER_EYE_X = 14.5f, OVER_EYE_Y = 7.0f, OVER_EYE_Z = 17.0f;
constexpr float OVER_AT_X  = 1.5f,  OVER_AT_Y  = 3.6f, OVER_AT_Z  = 1.0f;

constexpr float FREE_ZNEAR = 0.2f;
constexpr float FREE_SPEED = 3.0f;          // units per second
constexpr float FREE_TURN  = 90.0f;         // degrees per second of turning
constexpr float FREE_RADIUS = 22.0f;
constexpr float FREE_Y_MIN = 0.3f, FREE_Y_MAX = 20.0f;


constexpr float DT_CLAMP  = 0.1f;        // a window drag must not jump anything
constexpr float CRANK_DPS = 72.0f;       // crank degrees per second: 5 s a cycle
constexpr float FAN_DPS   = 220.0f;      // the exhaust fan, which drives nothing


constexpr float MODULE  = 0.05f;
constexpr float GEAR_Z  = -0.85f;     // all five gears share this plane
constexpr float GEAR_T  = 0.08f;      // back face is 0.06 clear of the panel
constexpr int   TEETH[5] = { 12, 36, 20, 20, 36 };      // G1..G5
constexpr float TOOTH_W_FRAC = 0.45f; // of the circular pitch pi*m; under 1/2
                                      // so block teeth clear at the mesh
constexpr float TOOTH_DEDENDUM = 1.25f; // root radius = r - 1.25m
constexpr float TOOTH_ADDENDUM = 1.00f; // tip  radius = r + 1.00m
constexpr float HUB_R_FRAC = 0.34f, HUB_T = 0.14f;

constexpr float G1_X = 2.115f, G1_Y = 5.950f;
constexpr float G2_X = 2.115f, G2_Y = 4.750f;
constexpr float G5_X = 4.550f, G5_Y = 2.040f;

constexpr float G3_X = 3.341857f, G3_Y = 4.075595f;
constexpr float G4_X = 4.010215f, G4_Y = 3.331756f;
constexpr float TOOTH_SINK = 0.05f;   

constexpr float PRESS_X  = 2.115f;    // station 7, and G1/G2's axis
constexpr float CRANK_R  = 0.25f;     // throw
constexpr float ROD_L    = 1.00f;
constexpr float CRANK_Y  = 4.750f;    // = s_min + r + L
constexpr float RAM_W = 0.36f, RAM_H = 0.40f, RAM_D = 0.36f;
constexpr float ROD_W = 0.08f, ROD_D = 0.08f;
constexpr float CRANK_SHAFT_R = 0.10f;
constexpr float CRANK_DISC_R  = 0.35f, CRANK_DISC_T = 0.06f;
constexpr float CRANK_PIN_R   = 0.07f;
constexpr float CRANK_DISC_Z0 = -0.11f; // shaft runs from the panel to here
constexpr float RAIL_X_OFF = 0.24f, RAIL_W = 0.06f, RAIL_D = 0.10f;
constexpr float RAIL_Y0 = 3.20f, RAIL_Y1 = 4.10f;
constexpr float BRACKET_Y0 = 3.60f, BRACKET_Y1 = 3.76f; // under G2's tip, 3.80

// ---- Drive panel, floor, motor  -----------------------------------
constexpr float PANEL_CX = 3.2f, PANEL_CY = 2.75f, PANEL_CZ = -1.0f;
constexpr float PANEL_W = 5.2f, PANEL_H = 5.5f, PANEL_D = 0.10f;

constexpr float FLOOR_X0 = -6.0f, FLOOR_X1 = 9.0f;
constexpr float FLOOR_Z0 = -2.5f, FLOOR_Z1 = 5.5f;
constexpr int   FLOOR_NX = 15, FLOOR_NZ = 8;
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
constexpr float CFRAME_Z  = 0.55f;    // side frame rail centre plane
constexpr float CFRAME_W  = 0.08f, CFRAME_H = 0.20f, CFRAME_LEN = 6.70f;
constexpr float CFRAME_Y  = 2.34f;    // top face 2.44, clear under the roller
constexpr float LEG_X0 = -2.00f, LEG_X1 = 3.70f, LEG_W = 0.12f;

// ---- Blanks (PRD FR-6, FR-7) -----------------------------------------------
constexpr float BLANK_R = 0.18f, BLANK_H = 0.20f, BLANK_H_FLAT = 0.10f;

constexpr int   PRESS_STATION = 7;    // furthest upstream station the
                                      // two-idler train can reach
constexpr int   LABELS = 9;           // blanks carry labels 1..9

constexpr int   GEN_SLOTS = 4;
constexpr float GEN_A = 0.55f;        // driver pin orbit radius
constexpr float GEN_LOC_DEG = 135.0f; // line of centres, driver -> wheel
constexpr float GEN_WHEEL_T = 0.08f;

constexpr float GEN_WHEEL_Z0 = 0.66f;
constexpr float GEN_ARM_HW  = 0.210f; // wheel arm half width; the gaps
                                      // between the four arms are the slots
constexpr float GEN_HUB_R   = 0.175f; // slot bottom; see note in scene.h
constexpr float GEN_PIN_R   = 0.045f;
constexpr float GEN_SHAFT_R = 0.05f;  // leaves 0.318 to the belt's surface
constexpr float GEN_SHAFT_Z0 = -0.95f, GEN_SHAFT_Z1 = 0.88f;
constexpr float GEN_PIN_Z0 = 0.66f, GEN_PIN_Z1 = 0.80f;
constexpr float GEN_ARM_Z0 = 0.80f, GEN_ARM_T = 0.08f, GEN_ARM_W = 0.10f;
constexpr float GEN_STUB_R = 0.09f;   // roller stub through the frame rail

// ---- Fixtures (PRD 4.2 row 13) ---------------------------------------------
constexpr float MAG_W = 0.50f, MAG_WALL = 0.06f;
constexpr float MAG_Y0 = 3.24f, MAG_Y1 = 4.60f;
constexpr float HOOD_X0 = 3.67f, HOOD_X1 = 4.45f;  // 0.30 past station 9;
                                                   // open at the far end
constexpr float HOOD_Y0 = 3.50f, HOOD_TOP_T = 0.12f;
constexpr float HOOD_Z_OUT = 0.52f;

// ---- Stack light (PRD 4.2 row 14, FR-14) -----------------------------------
constexpr float STACK_X = 5.50f, STACK_Z = -0.80f;
constexpr float STACK_POST_R = 0.05f, STACK_POST_H = 5.50f;
constexpr float STACK_R = 0.10f, STACK_SEG_H = 0.25f;
constexpr float STACK_Y0 = 5.55f;     // green, amber, red upward

constexpr float BULB_X[2] = { -0.57f, 4.80f };     // left, right
constexpr float BULB_Y = 5.40f, BULB_Z = 1.00f;    // centre of the glass
constexpr float BULB_R = 0.12f;       // glass globe

constexpr float ROOM_X0 = -6.0f, ROOM_X1 = 9.0f;   // = the floor
constexpr float ROOM_Z0 = -2.5f, ROOM_Z1 = 5.5f;
constexpr float CEIL_Y  = 9.5f;
constexpr float WALL_CELL = 1.0f;     // outlined: reads as wall panels
constexpr float DADO_H = 1.20f, TRIM_H = 0.06f;    // concrete plinth band

constexpr float WIN_Y0 = 5.20f, WIN_H = 1.80f, WIN_W = 1.80f;
constexpr float WIN_FRAME = 0.08f, WIN_DEPTH = 0.10f;

constexpr float HAZ_X0 = -3.20f, HAZ_X1 = 6.60f;
constexpr float HAZ_Z0 = -1.80f, HAZ_Z1 = 1.50f;
constexpr float HAZ_W = 0.10f, HAZ_Y = 0.004f;

constexpr float SHELF_Z0 = -2.30f, SHELF_Z1 = -0.30f;
constexpr float SHELF_Y = 2.90f, SHELF_D = 0.34f;
constexpr float LWIN_Z[2] = { -1.30f, 2.20f };

constexpr float BWIN_X[2] = { -4.50f, 7.60f };
constexpr float FAN_X = -1.70f, FAN_Y = 5.00f, FAN_SIZE = 1.00f;
constexpr int   FAN_BLADES = 6;

// Right wall (x = ROOM_X1) as world z, front wall (z = ROOM_Z1) as world x.
constexpr float RWIN_Z[2] = { -0.40f, 3.00f };
constexpr float FWIN_X[3] = { -3.00f, 1.50f, 6.00f };

// Free-standing on the floor.
constexpr float CAB_X0 = 6.20f, CAB_X1 = 7.60f, CAB_H = 2.40f, CAB_D = 0.51f;
constexpr float PAL_X = -4.10f, PAL_Z = 2.50f, PAL_S = 1.20f;  // feedstock
constexpr float PAL_PITCH = 0.38f;    // 3 x 3 blanks per layer
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
