// lighting.h - PRD FR-10 (the two-light rig) and FR-12 (three shading modes).
#ifndef LIGHTING_H
#define LIGHTING_H

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cstdio>

#include "config.h"
#include "shaders.h"

namespace lighting {

using namespace cfg;

enum Mode { FLAT = 0, GOURAUD, PHONG };
inline const char* mode_name(Mode m) {
    static const char* n[3] = { "FLAT", "GOURAUD", "PHONG (per-pixel)" };
    return n[m];
}

inline GLuint g_program = 0;
inline bool   g_shader_ok = false;
inline GLint  g_loc_normalize = -1;
inline char   g_shader_error[512] = { 0 };

// ---------------------------------------------------------------------------
// Light colours.  Kept here rather than in config.h because they are GLfloat
// arrays; every scalar they depend on is in config.h.
// ---------------------------------------------------------------------------
// Light 0, the press lamp.  Warm white, and its ambient is deliberately zero:
// a spotlight that contributed ambient would defeat its own cone.
constexpr GLfloat L0_AMBIENT[4]  = { 0.00f, 0.00f, 0.00f, 1.0f };
constexpr GLfloat L0_DIFFUSE[4]  = { 1.00f, 0.96f, 0.88f, 1.0f };
constexpr GLfloat L0_SPECULAR[4] = { 1.00f, 1.00f, 1.00f, 1.0f };

// Light 1, the fill.  Low and cool, so it reads as a different light rather
// than as more of the same one.
constexpr GLfloat L1_AMBIENT[4]  = { 0.05f, 0.05f, 0.06f, 1.0f };
constexpr GLfloat L1_DIFFUSE[4]  = { 0.25f, 0.26f, 0.30f, 1.0f };
constexpr GLfloat L1_SPECULAR[4] = { 0.15f, 0.15f, 0.18f, 1.0f };

constexpr GLfloat BLACK[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
constexpr GLfloat MODEL_AMBIENT[4] = { 0.10f, 0.10f, 0.12f, 1.0f };

// ---------------------------------------------------------------------------
// The fill-light toggle has to switch the light OFF for both pipelines, and
// they do not agree by default: GLSL 1.10 gives a shader no way to ask whether
// a light is enabled, so glDisable(GL_LIGHT1) alone would leave Phong mode
// still summing it.  Zeroing the colours as well makes the light contribute
// nothing in either pipeline, which keeps FR-12's "both modes read the same
// state" claim true through the toggle.
// ---------------------------------------------------------------------------
inline void set_fill(bool on) {
    glLightfv(GL_LIGHT1, GL_AMBIENT,  on ? L1_AMBIENT  : BLACK);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  on ? L1_DIFFUSE  : BLACK);
    glLightfv(GL_LIGHT1, GL_SPECULAR, on ? L1_SPECULAR : BLACK);
    if (on) glEnable(GL_LIGHT1); else glDisable(GL_LIGHT1);
}

// Everything about the lights that does not change per frame.
inline void init_lights() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT0, GL_AMBIENT,  L0_AMBIENT);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  L0_DIFFUSE);
    glLightfv(GL_LIGHT0, GL_SPECULAR, L0_SPECULAR);
    glLightf (GL_LIGHT0, GL_SPOT_CUTOFF,           SPOT_CUTOFF);
    glLightf (GL_LIGHT0, GL_SPOT_EXPONENT,         SPOT_EXPONENT);
    glLightf (GL_LIGHT0, GL_CONSTANT_ATTENUATION,  SPOT_A0);
    glLightf (GL_LIGHT0, GL_LINEAR_ATTENUATION,    SPOT_A1);
    glLightf (GL_LIGHT0, GL_QUADRATIC_ATTENUATION, SPOT_A2);

    glLightf (GL_LIGHT1, GL_SPOT_CUTOFF,           180.0f);  // a point light
    glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION,  FILL_A0);
    glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION,    FILL_A1);
    glLightf (GL_LIGHT1, GL_QUADRATIC_ATTENUATION, FILL_A2);
    set_fill(true);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, MODEL_AMBIENT);
    // GL_FALSE, the default, does not merely dim highlights: it substitutes a
    // constant view direction (0, 0, 1) in eye space for the true per-vertex
    // direction to the eye, so highlights stop shifting correctly as a surface
    // crosses the frame.  Moving highlights - sweeping across brass teeth,
    // sliding along blank tops as they travel - are exactly what this scene
    // shows, so the accurate view vector is not optional.
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
}

// ---------------------------------------------------------------------------
// THE one function, called from THE one place: display(), immediately after
// the camera transform and before any model transform.
//
// glLightfv(..., GL_POSITION, ...) transforms the position by the current
// modelview matrix, and glLightfv(..., GL_SPOT_DIRECTION, ...) transforms the
// direction by its upper-left 3x3.  Getting this wrong produces lights that
// swim with the objects - the single most common fixed-function lighting bug.
// Getting the position right but forgetting the direction produces a spotlight
// whose cone points somewhere else: the same bug wearing a different hat.
//
// The fourth component is given as 1.0 explicitly.  A w of 0.0 silently makes
// the light directional, which discards both the position and the attenuation,
// and the resulting flat, unattenuated scene looks plausible enough to go
// unnoticed for an hour.
// ---------------------------------------------------------------------------
inline void place_lights() {
    const GLfloat pos0[4] = { SPOT_X, SPOT_Y, SPOT_Z, 1.0f };
    const GLfloat dir0[3] = { AIM_X - SPOT_X, AIM_Y - SPOT_Y, AIM_Z - SPOT_Z };
    glLightfv(GL_LIGHT0, GL_POSITION,       pos0);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, dir0);

    const GLfloat pos1[4] = { FILL_X, FILL_Y, FILL_Z, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, pos1);
}

// ---------------------------------------------------------------------------
// The GLSL program.  Windows ships OpenGL 1.1, so glCreateShader and friends
// do not exist in opengl32.dll and are resolved at runtime by GLEW (PRD 8).
// ---------------------------------------------------------------------------
inline GLuint compile(GLenum type, const char* src, const char* what) {
    const GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = GL_FALSE;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLchar log[512];
        glGetShaderInfoLog(s, sizeof log, NULL, log);
        snprintf(g_shader_error, sizeof g_shader_error, "%s: %s", what, log);
        fprintf(stderr, "shader compile failed (%s):\n%s\n", what, log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

inline void build_shader() {
    if (!GLEW_VERSION_2_0) {
        snprintf(g_shader_error, sizeof g_shader_error,
                 "no GLSL: this context is not OpenGL 2.0");
        fprintf(stderr, "%s\n", g_shader_error);
        return;
    }
    const GLuint vs = compile(GL_VERTEX_SHADER, shader_src::VERTEX, "vertex");
    if (!vs) return;
    const GLuint fs = compile(GL_FRAGMENT_SHADER, shader_src::FRAGMENT, "fragment");
    if (!fs) { glDeleteShader(vs); return; }

    g_program = glCreateProgram();
    glAttachShader(g_program, vs);
    glAttachShader(g_program, fs);
    glLinkProgram(g_program);
    glDeleteShader(vs);                 // flagged; freed when the program is
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(g_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLchar log[512];
        glGetProgramInfoLog(g_program, sizeof log, NULL, log);
        snprintf(g_shader_error, sizeof g_shader_error, "link: %s", log);
        fprintf(stderr, "shader link failed:\n%s\n", log);
        glDeleteProgram(g_program);
        g_program = 0;
        return;
    }
    g_loc_normalize = glGetUniformLocation(g_program, "uNormalize");
    g_shader_ok = true;
    printf("GLSL %s - Phong program ok\n",
           glGetString(GL_SHADING_LANGUAGE_VERSION));
}

// ---------------------------------------------------------------------------
// Flat and Gouraud differ only by glShadeModel and both use the fixed
// pipeline, which gives Blinn-Phong.  Phong binds the program, which gives
// the reflection-vector model evaluated per fragment.
// ---------------------------------------------------------------------------
inline void apply_mode(Mode m, bool normalize_on) {
    if (m == PHONG && g_shader_ok) {
        glUseProgram(g_program);
        if (g_loc_normalize >= 0)
            glUniform1i(g_loc_normalize, normalize_on ? 1 : 0);
        glShadeModel(GL_SMOOTH);
    } else {
        if (GLEW_VERSION_2_0) glUseProgram(0);
        glShadeModel(m == FLAT ? GL_FLAT : GL_SMOOTH);
    }
}

} // namespace lighting
#endif
