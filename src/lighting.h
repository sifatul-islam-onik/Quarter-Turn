// lighting.h - the two hanging bulbs (replacing PRD FR-10's rig) and FR-12
// (three shading modes).
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
// Both bulbs are the same warm white, GL_LIGHT0 on the left and GL_LIGHT1 on
// the right.  Each carries a little ambient of its own, so switching both off
// visibly darkens the room instead of only removing the direct light.
constexpr GLfloat BULB_AMBIENT[4]  = { 0.08f, 0.07f, 0.06f, 1.0f };
constexpr GLfloat BULB_DIFFUSE[4]  = { 1.00f, 0.93f, 0.80f, 1.0f };
constexpr GLfloat BULB_SPECULAR[4] = { 1.00f, 0.97f, 0.90f, 1.0f };

constexpr GLfloat BLACK[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
// Global ambient.  Raised from 0.10 with the bulbs, so a room lit only by
// point lights still reads in the corners they barely reach.
constexpr GLfloat MODEL_AMBIENT[4] = { 0.20f, 0.20f, 0.22f, 1.0f };

inline GLenum light_id(int i) { return (GLenum)(GL_LIGHT0 + i); }

// ---------------------------------------------------------------------------
// A bulb switch has to turn the light OFF for both pipelines, and they do not
// agree by default: GLSL 1.10 gives a shader no way to ask whether a light is
// enabled, so glDisable alone would leave Phong mode still summing it.
// Zeroing the colours as well makes the light contribute nothing in either
// pipeline, which keeps FR-12's "both modes read the same state" claim true.
// ---------------------------------------------------------------------------
inline void set_bulb(int i, bool on) {
    const GLenum l = light_id(i);
    glLightfv(l, GL_AMBIENT,  on ? BULB_AMBIENT  : BLACK);
    glLightfv(l, GL_DIFFUSE,  on ? BULB_DIFFUSE  : BLACK);
    glLightfv(l, GL_SPECULAR, on ? BULB_SPECULAR : BLACK);
    if (on) glEnable(l); else glDisable(l);
}

// Everything about the lights that does not change per frame.
inline void init_lights() {
    glEnable(GL_LIGHTING);
    for (int i = 0; i < 2; ++i) {
        const GLenum l = light_id(i);
        glLightf(l, GL_SPOT_CUTOFF,           180.0f);   // a point light
        glLightf(l, GL_CONSTANT_ATTENUATION,  BULB_A0);
        glLightf(l, GL_LINEAR_ATTENUATION,    BULB_A1);
        glLightf(l, GL_QUADRATIC_ATTENUATION, BULB_A2);
        set_bulb(i, true);
    }

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
// modelview matrix.  Getting this wrong produces lights that swim with the
// objects - the single most common fixed-function lighting bug.  Here it
// would show at once: each bulb's glow is drawn at the same world position,
// so a misplaced light would visibly leave its bulb behind.
//
// The fourth component is given as 1.0 explicitly.  A w of 0.0 silently makes
// the light directional, which discards both the position and the attenuation,
// and the resulting flat, unattenuated scene looks plausible enough to go
// unnoticed for an hour.
// ---------------------------------------------------------------------------
inline void place_lights() {
    for (int i = 0; i < 2; ++i) {
        const GLfloat pos[4] = { BULB_X[i], BULB_Y, BULB_Z, 1.0f };
        glLightfv(light_id(i), GL_POSITION, pos);
    }
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
