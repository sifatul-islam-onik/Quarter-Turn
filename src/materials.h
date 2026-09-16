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
// tail of the line sitting outside the spotlight's cone, that left them
// unreadable.  ka is raised to 0.02 and kd to 0.05, exactly the escape hatch
// the PRD specifies.  ks and ns are untouched, so the material still reads as
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
// floor is where the spotlight's radial attenuation is most visible.
constexpr Material CONCRETE = {
    { 0.10f, 0.10f, 0.09f, 1.0f },
    { 0.45f, 0.44f, 0.42f, 1.0f },
    { 0.03f, 0.03f, 0.03f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    4.0f
};

// --- emissive --------------------------------------------------------------

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
