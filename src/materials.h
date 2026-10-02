#ifndef MATERIALS_H
#define MATERIALS_H

#include <GL/freeglut.h>

namespace mat {

// The Phong model's terms for one surface: its colour (ka and kd, through
// glColor and GL_COLOR_MATERIAL), its specular colour ks and exponent ns.
struct Material {
    GLfloat rgb[3];
    GLfloat ks[3];
    GLfloat ns;
};

constexpr GLfloat NONE[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

inline void use(const Material& m) {
    const GLfloat ks[4] = { m.ks[0], m.ks[1], m.ks[2], 1.0f };
    glColor3fv(m.rgb);
    glMaterialfv(GL_FRONT, GL_SPECULAR, ks);
    glMaterialf (GL_FRONT, GL_SHININESS, m.ns);
    glMaterialfv(GL_FRONT, GL_EMISSION, NONE);
}

// A part that gives out light of its own: lit bulb glass, a lit lamp lens,
// daylight in a window.  This is the emission term, so the colour shows
// whatever the lamps do, and ambient, diffuse and specular are zeroed.
inline void glow(const Material& m) {
    const GLfloat ke[4] = { m.rgb[0], m.rgb[1], m.rgb[2], 1.0f };
    glColor3f(0.0f, 0.0f, 0.0f);
    glMaterialfv(GL_FRONT, GL_SPECULAR, NONE);
    glMaterialfv(GL_FRONT, GL_EMISSION, ke);
}

// --- ks and ns from the slides' table (Hill, after McReynolds and Blythe) ---
constexpr Material SILVER        = { { 0.80f, 0.82f, 0.85f }, { 0.77f, 0.77f, 0.77f }, 89.6f };
constexpr Material BRASS         = { { 0.88f, 0.68f, 0.20f }, { 0.99f, 0.94f, 0.81f }, 27.9f };
constexpr Material COPPER        = { { 0.78f, 0.42f, 0.20f }, { 0.58f, 0.22f, 0.07f }, 51.2f };
// Motor body, stack-light housing.
constexpr Material BLACK_PLASTIC = { { 0.20f, 0.21f, 0.23f }, { 0.50f, 0.50f, 0.50f }, 32.0f };

// --- ks and ns chosen by eye ------------------------------------------------
constexpr Material DARK_STEEL    = { { 0.46f, 0.50f, 0.56f }, { 0.50f, 0.50f, 0.50f }, 40.0f };
// Drive panel, conveyor frame, guide rails.  Semi-gloss: the panel is one big
// quad, and a strong highlight on one corner would smear across all of it.
constexpr Material MACHINE_PAINT = { { 0.26f, 0.48f, 0.36f }, { 0.10f, 0.10f, 0.10f }, 10.0f };
constexpr Material RUBBER        = { { 0.14f, 0.14f, 0.15f }, { 0.05f, 0.05f, 0.05f },  6.0f };
constexpr Material CONCRETE      = { { 0.52f, 0.51f, 0.48f }, { 0.03f, 0.03f, 0.03f },  4.0f };

// --- the room --------------------------------------------------------------
constexpr Material WALL_PAINT    = { { 0.78f, 0.77f, 0.72f }, { 0.04f, 0.04f, 0.04f },  6.0f };
constexpr Material SAFETY_YELLOW = { { 0.95f, 0.76f, 0.10f }, { 0.30f, 0.30f, 0.25f }, 16.0f };
constexpr Material WOOD          = { { 0.66f, 0.46f, 0.25f }, { 0.08f, 0.07f, 0.05f }, 10.0f };
constexpr Material SIGNAL_RED    = { { 0.80f, 0.12f, 0.08f }, { 0.50f, 0.45f, 0.45f }, 40.0f };
constexpr Material GALVANIZED    = { { 0.58f, 0.60f, 0.63f }, { 0.35f, 0.35f, 0.36f }, 18.0f };
// These three are drawn with glow(): only their colour is used.
constexpr Material WINDOW_GLASS  = { { 0.62f, 0.76f, 0.90f }, { 0, 0, 0 }, 1.0f };   // daylight
constexpr Material BULB_ON       = { { 1.00f, 0.95f, 0.78f }, { 0, 0, 0 }, 1.0f };
// A switched-off bulb is plain glass: dark, with a sharp highlight.
constexpr Material BULB_OFF      = { { 0.34f, 0.35f, 0.37f }, { 0.90f, 0.90f, 0.90f }, 80.0f };

// A lamp lens - the stack light and the switch lamps.  Lit, it glows in its
// full colour; dark, it is dim plastic that only reflects the room's light.
inline void lens(float r, float g, float b, bool lit) {
    const Material lamp = { { r, g, b }, { 0, 0, 0 }, 1.0f };
    const Material dark = { { 0.25f * r + 0.04f, 0.25f * g + 0.04f, 0.25f * b + 0.04f },
                            { 0.50f, 0.50f, 0.50f }, 32.0f };
    if (lit) glow(lamp); else use(dark);
}

} // namespace mat
#endif
