#ifndef MATERIALS_H
#define MATERIALS_H

#include <GL/freeglut.h>

namespace mat {

struct Material {
    GLfloat rgb[3];
};

inline void use(const Material& m) { glColor3fv(m.rgb); }

constexpr Material SILVER        = { { 0.80f, 0.82f, 0.85f } };

constexpr Material DARK_STEEL    = { { 0.46f, 0.50f, 0.56f } };

constexpr Material BRASS         = { { 0.88f, 0.68f, 0.20f } };
constexpr Material COPPER        = { { 0.78f, 0.42f, 0.20f } };
// Motor body, magazine, exit hood, stack-light housing.
constexpr Material BLACK_PLASTIC = { { 0.20f, 0.21f, 0.23f } };
// Drive panel, conveyor frame, guide rails.
constexpr Material MACHINE_PAINT = { { 0.26f, 0.48f, 0.36f } };

constexpr Material RUBBER        = { { 0.14f, 0.14f, 0.15f } };
constexpr Material CONCRETE      = { { 0.52f, 0.51f, 0.48f } };

// --- the room --------------------------------------------------------------

constexpr Material WALL_PAINT    = { { 0.78f, 0.77f, 0.72f } };
constexpr Material SAFETY_YELLOW = { { 0.95f, 0.76f, 0.10f } };
constexpr Material WOOD          = { { 0.66f, 0.46f, 0.25f } };
constexpr Material SIGNAL_RED    = { { 0.80f, 0.12f, 0.08f } };
constexpr Material GALVANIZED    = { { 0.58f, 0.60f, 0.63f } };
constexpr Material WINDOW_GLASS  = { { 0.62f, 0.76f, 0.90f } };   // daylight
constexpr Material BULB_ON       = { { 1.00f, 0.95f, 0.78f } };
constexpr Material BULB_OFF      = { { 0.34f, 0.35f, 0.37f } };

// A lamp lens - the stack light and the switch lamps:
// its full colour when lit, a dim version of it when not.
inline Material lens(float r, float g, float b, bool lit) {
    const float k = lit ? 1.0f : 0.25f, d = lit ? 0.0f : 0.04f;
    Material m = { { r * k + d, g * k + d, b * k + d } };
    return m;
}

} // namespace mat
#endif
