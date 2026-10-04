#ifndef CONFIG_H
#define CONFIG_H

namespace cfg {

constexpr float PI  = 3.14159265358979f;
constexpr float DEG = 180.0f / PI;
constexpr float RAD = PI / 180.0f;

constexpr int   CYL_SLICES = 10;      // [free] sides on every cylinder

// ---- Camera [free] ---------------------------------------------------------
constexpr float EYE_X = 7.0f, EYE_Y = 6.2f, EYE_Z = 9.0f;     // view 1
constexpr float AT_X  = 1.4f, AT_Y  = 2.9f, AT_Z  = 0.0f;
// The eye never leaves the room, so it can always be close to something:
// one small near plane for every view.  40 still reaches the far corner (30).
constexpr float FOVY  = 45.0f, ZNEAR = 0.2f, ZFAR = 40.0f;

constexpr float OVER_EYE_X = 12.2f, OVER_EYE_Y = 8.4f, OVER_EYE_Z = 12.7f;  // view 2,
constexpr float OVER_AT_X  = 1.5f,  OVER_AT_Y  = 3.0f, OVER_AT_Z  = 1.0f;   // a corner

constexpr float ORBIT_MAX  = 90.0f;         // degrees either way: never behind the panel
constexpr float FREE_SPEED = 3.0f;          // units per second
constexpr float FREE_TURN  = 90.0f;         // degrees per second of turning

// Every view, preset or free, keeps its eye this far inside the walls and
// between these heights; the top stays under the ceiling beams (9.05).
constexpr float CAM_MARGIN = 0.5f;
constexpr float CAM_Y_MIN = 0.3f, CAM_Y_MAX = 8.5f;

// ---- Speed [free] ----------------------------------------------------------
constexpr float DT_CLAMP  = 0.1f;        // a window drag must not jump anything
constexpr float CRANK_DPS = 72.0f;       // crank degrees per second: 5 s a cycle
constexpr float FAN_DPS   = 220.0f;      // the exhaust fan, which drives nothing

// ---- Drive panel and motor [free] ------------------------------------------
constexpr float PANEL_CX = 3.2f, PANEL_CY = 2.75f, PANEL_CZ = -1.0f;
constexpr float PANEL_W = 5.2f, PANEL_H = 5.5f, PANEL_D = 0.10f;
constexpr float MOTOR_R = 0.35f, MOTOR_Z0 = -1.75f, MOTOR_Z1 = -0.95f;
constexpr float MOTOR_SHAFT_R = 0.08f;

// ---- Gears [coupled] -------------------------------------------------------
constexpr float MODULE  = 0.05f;
constexpr int   TEETH[5] = { 12, 36, 20, 20, 36 };      // G1..G5
constexpr float G1_X = 2.115f, G1_Y = 5.950f;
constexpr float G2_X = 2.115f, G2_Y = 4.750f;
constexpr float G3_X = 3.341857f, G3_Y = 4.075595f;     // solved, not chosen
constexpr float G4_X = 4.010215f, G4_Y = 3.331756f;     // solved, not chosen
constexpr float G5_X = 4.550f, G5_Y = 2.040f;
constexpr float GEAR_X[5] = { G1_X, G2_X, G3_X, G4_X, G5_X };
constexpr float GEAR_Y[5] = { G1_Y, G2_Y, G3_Y, G4_Y, G5_Y };

constexpr float GEAR_Z  = -0.85f;     // [free] all five gears share this plane
constexpr float GEAR_T  = 0.08f;      // [free] back face is 0.06 clear of the panel
constexpr float TOOTH_W_FRAC = 0.45f; // of the circular pitch pi*m; under 1/2
                                      // so block teeth clear at the mesh
constexpr float TOOTH_DEDENDUM = 1.25f; // root radius = r - 1.25m
constexpr float TOOTH_ADDENDUM = 1.00f; // tip  radius = r + 1.00m
constexpr float TOOTH_SINK = 0.05f;
constexpr float HUB_R_FRAC = 0.34f, HUB_T = 0.14f;      // [free]

// ---- Press [coupled] -------------------------------------------------------
constexpr float PRESS_X  = 2.115f;    // = G2_X, and exactly station 7
constexpr float CRANK_Y  = 4.750f;    // = G2_Y
constexpr float CRANK_R  = 0.25f;     // throw
constexpr float ROD_L    = 1.00f;
constexpr float RAM_W = 0.36f, RAM_H = 0.40f, RAM_D = 0.36f;    // RAM_W, RAM_D free
// the rest of the press is [free]
constexpr float ROD_W = 0.08f, ROD_D = 0.08f;
constexpr float CRANK_SHAFT_R = 0.10f;
constexpr float CRANK_DISC_R  = 0.35f, CRANK_DISC_T = 0.06f;
constexpr float CRANK_PIN_R   = 0.07f;
constexpr float CRANK_DISC_Z0 = -0.11f; // shaft runs from the panel to here
constexpr float RAIL_X_OFF = 0.24f, RAIL_W = 0.06f, RAIL_D = 0.10f;
constexpr float RAIL_Y0 = 3.20f, RAIL_Y1 = 4.10f;
constexpr float BRACKET_Y0 = 3.60f, BRACKET_Y1 = 3.76f; // under G2's tip, 3.80

// ---- Geneva drive [coupled] ------------------------------------------------
constexpr int   GEN_SLOTS = 4;
constexpr float GEN_A = 0.55f;        // driver pin orbit radius
constexpr float GEN_LOC_DEG = 135.0f; // line of centres, driver -> wheel
// the rest of the Geneva is [free]
constexpr float GEN_WHEEL_T = 0.08f;
constexpr float GEN_WHEEL_Z0 = 0.66f;
constexpr float GEN_ARM_HW  = 0.210f; // wheel arm half width; the gaps
                                      // between the four arms are the slots
constexpr float GEN_HUB_R   = 0.175f; // slot bottom
constexpr float GEN_PIN_R   = 0.045f;
constexpr float GEN_SHAFT_R = 0.05f;  // leaves 0.318 to the belt's surface
constexpr float GEN_SHAFT_Z0 = -0.95f, GEN_SHAFT_Z1 = 0.88f;
constexpr float GEN_PIN_Z0 = 0.66f, GEN_PIN_Z1 = 0.80f;
constexpr float GEN_ARM_Z0 = 0.80f, GEN_ARM_T = 0.08f, GEN_ARM_W = 0.10f;
constexpr float GEN_STUB_R = 0.09f;   // roller stub through the frame rail

// ---- Conveyor [coupled] ----------------------------------------------------
constexpr float HEAD_X   = 4.000f;    // head roller axis; station 10, c from G5
constexpr float ROLLER_Y = 2.59f;     // also c from G5
constexpr float ROLLER_R = 0.39f;
constexpr float BELT_R_C = 0.40f;     // centreline radius: pitch p = R_c*pi/2
constexpr float BELT_R_O = 0.41f;     // outer surface, where cleats sit
constexpr float BELT_TOP_Y = 3.00f;   // 2.59 + 0.39 + 0.02
constexpr int   ROLL_PITCHES = 10;    // roller centres are exactly 10p apart
constexpr int   CLEAT_N = 24;         // loop is 2D + 2*pi*R_c = 24p
// the rest of the conveyor is [free]
constexpr float ROLLER_LEN = 1.04f;
constexpr float BELT_T   = 0.02f, BELT_W = 1.00f;
constexpr float CLEAT_W = 0.03f, CLEAT_H = 0.03f, CLEAT_LEN = 0.96f;
constexpr float CFRAME_Z  = 0.55f;    // side frame rail centre plane
constexpr float CFRAME_W  = 0.08f, CFRAME_H = 0.20f, CFRAME_LEN = 6.70f;
constexpr float CFRAME_Y  = 2.34f;    // top face 2.44, clear under the roller
constexpr float LEG_X0 = -2.00f, LEG_X1 = 3.70f, LEG_W = 0.12f;

// ---- Blanks ----------------------------------------------------------------
constexpr float BLANK_R = 0.18f;                        // [free]
constexpr float BLANK_H = 0.20f, BLANK_H_FLAT = 0.10f;  // [coupled] to the stroke
constexpr int   LABELS = 9;           // blanks on the belt, stations 1..9

// ---- Stack light [free] ----------------------------------------------------
constexpr float STACK_X = 5.50f, STACK_Z = -0.80f;
constexpr float STACK_POST_R = 0.05f, STACK_POST_H = 5.50f;
constexpr float STACK_R = 0.10f, STACK_SEG_H = 0.25f;
constexpr float STACK_Y0 = 5.55f;     // green, amber, red upward

// ---- Room: floor, walls, ceiling [free] ------------------------------------
constexpr float ROOM_X0 = -10.0f, ROOM_X1 = 13.0f; // also the floor
constexpr float ROOM_Z0 = -2.5f,  ROOM_Z1 = 13.5f; // back wall stays behind the panel
constexpr int   FLOOR_NX = 23, FLOOR_NZ = 16;      // floor tiles
constexpr float CEIL_Y  = 9.5f;
constexpr float WALL_CELL = 1.0f;     // outlined: reads as wall panels
constexpr float DADO_H = 1.20f, TRIM_H = 0.06f;    // concrete plinth band

constexpr float BEAM_X0 = -7.00f, BEAM_PITCH = 3.00f;   // ceiling I-beams,
constexpr int   BEAM_N = 7;                             // front to back
constexpr float BEAM_D = 0.45f, BEAM_W = 0.30f, BEAM_T = 0.05f;

// ---- Room: windows [free] --------------------------------------------------
constexpr float WIN_Y0 = 5.20f, WIN_H = 1.80f, WIN_W = 1.80f;
constexpr float WIN_FRAME = 0.08f, WIN_DEPTH = 0.10f;
constexpr float WIN_BAR = 0.05f;      // the cross bar that splits it in four
constexpr float BWIN_X[4] = { -7.50f, -4.50f, 7.60f, 10.50f };          // back wall, world x
constexpr float LWIN_Z[4] = { -1.30f, 2.70f, 6.70f, 10.70f };           // left wall, world z
constexpr float RWIN_Z[4] = { -0.40f, 3.60f, 7.60f, 11.60f };           // right wall, world z
constexpr float FWIN_X[5] = { -7.00f, -2.50f, 2.00f, 6.50f, 11.00f };   // front wall, world x

// ---- Room: bulbs [free] ----------------------------------------------------
constexpr float BULB_X[2] = { -0.57f, 4.80f };     // left, right
constexpr float BULB_Y = 6.60f, BULB_Z = 1.00f;    // centre of the glass
constexpr float BULB_R = 0.12f;       // glass globe
constexpr float SHADE_R = 0.30f;      // metal shade over it
// Each bulb is a spotlight pointing straight down.  The flat shade blocks all
// the light above the bulb, a 90 degree cutoff, so BULB_Y must stay above the
// top of G1 and the motor (6.3) or they fall outside the cone.
constexpr float BULB_CUTOFF = 90.0f;  // degrees from straight down
constexpr float BULB_SPOT_EXP = 0.5f; // cos^0.5: fades out gently, so the
                                      // machine, well off the axis, stays lit
// Attenuation, as on the slides: the light reaching distance d is divided by
// a0 + a1*d + a2*d*d, so the walls far from a bulb get less of it.
constexpr float BULB_A0 = 1.0f, BULB_A1 = 0.03f, BULB_A2 = 0.008f;

// ---- Room: exhaust fan, back wall [free] -----------------------------------
constexpr float FAN_X = -1.70f, FAN_Y = 5.00f, FAN_SIZE = 1.00f;
constexpr int   FAN_BLADES = 6;

// ---- Room: spare-gear shelf, left wall [free] ------------------------------
constexpr float SHELF_Z0 = -2.30f, SHELF_Z1 = -0.30f;
constexpr float SHELF_Y = 2.90f, SHELF_D = 0.34f;

// ---- Room: floor items [free] ----------------------------------------------
constexpr float HAZ_X0 = -3.20f, HAZ_X1 = 6.60f;   // yellow hazard border
constexpr float HAZ_Z0 = -1.80f, HAZ_Z1 = 1.50f;
constexpr float HAZ_W = 0.10f, HAZ_Y = 0.004f;

// Two pallets: raw blanks by the tail of the belt, stamped ones by the head.
constexpr float PAL_X[2] = { -4.10f, 5.40f }, PAL_Z[2] = { 2.50f, 2.70f };
constexpr float PAL_S = 1.20f, PAL_TOP = 0.13f;     // size, deck height
constexpr float PAL_PITCH = 0.38f;    // 3 x 3 blanks per layer

// The bin past the head roller: an open box.  Each stamped blank that leaves
// the belt lands in it, nine to a layer, until BIN_LAYERS layers fill it.
constexpr float BIN_X = 5.40f, BIN_Z = 0.00f;
constexpr float BIN_S = 1.30f, BIN_H = 0.50f, BIN_T = 0.05f;   // size, height, wall
constexpr int   BIN_LAYERS = 4;

constexpr float CAB_X0 = 6.20f, CAB_X1 = 7.60f, CAB_H = 2.40f, CAB_D = 0.51f;
// Switches on the cabinet door, left to right: machine, left bulb, right bulb,
// fan.  Two each side of the door seam, with a status lamp over each.
constexpr float SWITCH_X[4] = { 6.40f, 6.66f, 7.14f, 7.40f };
constexpr float SWITCH_Y = 1.76f;

constexpr float DRUM_R = 0.30f, DRUM_H = 0.88f;
constexpr float DRUM_X[2] = { 7.25f, 8.05f }, DRUM_Z[2] = { -1.25f, -0.95f };

} // namespace cfg
#endif
