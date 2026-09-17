// materials.h - PRD FR-11.  Six materials: three taken verbatim from the
// slides' coefficient table (credited there to Hill, after McReynolds and
// Blythe) and three authored.  The ns values span 4 to 89.6, which puts the
// slides' specular-exponent figure on screen inside a single frame.
#ifndef MATERIALS_H
#define MATERIALS_H

#include <GL/glew.h>

namespace mat {

struct Material {
    GLfloat ka[4], kd[4], ks[4], ke[4];
    GLfloat ns;
};

// Back faces are culled throughout, so only GL_FRONT is ever specified.
// A display list captures these *values* when it is compiled, not a reference
// to them, so any material that changes at runtime - the stack light's
// emission - must be set outside the list, immediately before calling it
// (PRD FR-15).
inline void use(const Material& m) {
    glMaterialfv(GL_FRONT, GL_AMBIENT,   m.ka);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   m.kd);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  m.ks);
    glMaterialfv(GL_FRONT, GL_EMISSION,  m.ke);
    glMaterialf (GL_FRONT, GL_SHININESS, m.ns);
}

constexpr GLfloat NO_EMISSION[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

// --- from the slides -------------------------------------------------------

// Polished silver.  ns 89.6 is the highest exponent in the scene, and this is
// the material that carries the mandatory specular highlight: blanks, punch,
// rod, shafts, crank disc and pin, Geneva pin, rollers.
constexpr Material SILVER = {
    { 0.23125f,  0.23125f,  0.23125f,  1.0f },
    { 0.2775f,   0.2775f,   0.2775f,   1.0f },
    { 0.773911f, 0.773911f, 0.773911f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    89.6f
};

// Brass.  The five gears and the Geneva wheel; highlights sweep across the
// teeth as the train turns.
constexpr Material BRASS = {
    { 0.329412f, 0.223529f, 0.027451f, 1.0f },
    { 0.780392f, 0.568627f, 0.113725f, 1.0f },
    { 0.992157f, 0.941176f, 0.807843f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    27.8974f
};

// Black plastic - motor body, magazine, exit hood, stack-light housing.
//
// DOCUMENTED DEVIATION (PRD FR-11 anticipates it and asks for exactly this
// note).  The slide's values are ka (0, 0, 0) and kd (0.01, 0.01, 0.01), which
// leave these parts essentially black except where the specular term catches
// them.  Against this scene's dark drive panel, and with the motor and the
// tail of the line well away from the light, that left them unreadable.  ka
// is raised to 0.02 and kd to 0.05, exactly the escape hatch the PRD
// specifies.  ks and ns are untouched, so the material still reads as
// plastic: a broad, dim body with a sharp highlight.
constexpr Material BLACK_PLASTIC = {
    { 0.02f, 0.02f, 0.02f, 1.0f },      // slide: 0, 0, 0
    { 0.05f, 0.05f, 0.05f, 1.0f },      // slide: 0.01, 0.01, 0.01
    { 0.50f, 0.50f, 0.50f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    32.0f
};

// --- authored --------------------------------------------------------------

// Machine paint - drive panel, conveyor frame, guide rails, chute, bin.
constexpr Material MACHINE_PAINT = {
    { 0.05f, 0.08f, 0.06f, 1.0f },
    { 0.20f, 0.34f, 0.26f, 1.0f },
    { 0.30f, 0.30f, 0.30f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    20.0f
};

// Rubber - belt and cleats.  ns 6 is the low end of the exponent range.
constexpr Material RUBBER = {
    { 0.02f, 0.02f, 0.02f, 1.0f },
    { 0.06f, 0.06f, 0.06f, 1.0f },
    { 0.05f, 0.05f, 0.05f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    6.0f
};

// Concrete - the floor.  ns 4 is the lowest exponent in the scene, and the
// floor is where the bulbs' radial attenuation is most visible: it darkens
// toward the corners of the room.
constexpr Material CONCRETE = {
    { 0.10f, 0.10f, 0.09f, 1.0f },
    { 0.45f, 0.44f, 0.42f, 1.0f },
    { 0.03f, 0.03f, 0.03f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    4.0f
};

// --- the room (added with it, not among the PRD's six) ----------------------
// None of these carries a demonstration.  The wall's ka is high so the room
// still reads, dimly, with both bulbs switched off.

// Painted blockwork, upper walls and ceiling.
constexpr Material WALL_PAINT = {
    { 0.55f, 0.54f, 0.50f, 1.0f },
    { 0.62f, 0.61f, 0.56f, 1.0f },
    { 0.04f, 0.04f, 0.04f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    6.0f
};

// Floor markings, the stripe along the walls, the door's bottom bar.
constexpr Material SAFETY_YELLOW = {
    { 0.30f, 0.24f, 0.02f, 1.0f },
    { 0.88f, 0.70f, 0.06f, 1.0f },
    { 0.30f, 0.30f, 0.25f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    16.0f
};

// Pallet, bench top, tool board, shelf.
constexpr Material WOOD = {
    { 0.22f, 0.15f, 0.08f, 1.0f },
    { 0.58f, 0.40f, 0.21f, 1.0f },
    { 0.08f, 0.07f, 0.05f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    10.0f
};

// Extinguisher, toolbox, drum, screwdriver handles.
constexpr Material SIGNAL_RED = {
    { 0.20f, 0.03f, 0.02f, 1.0f },
    { 0.72f, 0.09f, 0.06f, 1.0f },
    { 0.50f, 0.45f, 0.45f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    40.0f
};

// Dull zinc-coated steel: beams, pipes, frames, the shutter, bulb fittings.
// Deliberately far below silver's ns 89.6, so the polished parts still own
// the only tight highlights in the frame.
constexpr Material GALVANIZED = {
    { 0.16f, 0.16f, 0.17f, 1.0f },
    { 0.42f, 0.43f, 0.45f, 1.0f },
    { 0.35f, 0.35f, 0.36f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    18.0f
};

// Window glass.  Its emission stands in for daylight beyond it; the pane lights
// nothing else, which is honest in a scene with no daylight source.
constexpr Material WINDOW_GLASS = {
    { 0.02f, 0.02f, 0.02f, 1.0f },
    { 0.10f, 0.12f, 0.14f, 1.0f },
    { 0.60f, 0.60f, 0.60f, 1.0f },
    { 0.36f, 0.46f, 0.58f, 1.0f },
    60.0f
};

// --- emissive --------------------------------------------------------------

// A lit bulb is pure emission, so it reads as the source of its light.  An
// unlit one is clear glass: dark, with a sharp highlight from the other bulb.
// The switch chooses between them per frame, outside any display list.
constexpr Material BULB_ON = {
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 1.00f, 0.94f, 0.76f, 1.0f },
    1.0f
};
constexpr Material BULB_OFF = {
    { 0.04f, 0.04f, 0.04f, 1.0f },
    { 0.12f, 0.12f, 0.12f, 1.0f },
    { 0.90f, 0.90f, 0.90f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    80.0f
};

// The stack light's lens.  The lit segment gets an emission matching its
// colour and the unlit ones a dim one, which puts the + I_e term of the
// slides' illumination equation on screen doing useful work.  It is the one
// component of that equation nothing else in a scene like this demonstrates.
// Built per frame because the emission changes with the phase (PRD FR-10).
inline Material lens(float r, float g, float b, bool lit) {
    const float e = lit ? 1.0f : 0.06f;
    Material m = {
        { 0.02f, 0.02f, 0.02f, 1.0f },
        { r * 0.25f, g * 0.25f, b * 0.25f, 1.0f },
        { 0.45f, 0.45f, 0.45f, 1.0f },
        { r * e, g * e, b * e, 1.0f },
        24.0f
    };
    return m;
}

} // namespace mat
#endif
