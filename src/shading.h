#ifndef SHADING_H
#define SHADING_H

#include <GL/glew.h>
#include <cstdio>

namespace shade {

enum Mode { FLAT = 0, GOURAUD, PHONG, RAYS, MODE_COUNT };
inline const char* NAME[MODE_COUNT] = { "FLAT", "GOURAUD", "PHONG", "SHADOW RAYS" };

inline GLuint program = 0;      // the Phong program; 0 if it could not be built

// Phong shading: the normal is passed on to every pixel and the lighting is
// worked out there.  It is the same equation the fixed pipeline works out per
// vertex, and it reads the same lights and materials (gl_LightSource ...).
inline const char* VERTEX = R"GLSL(#version 110
varying vec3 vNormal;       // eye space
varying vec3 vPosition;     // eye space
void main() {
    vPosition = vec3(gl_ModelViewMatrix * gl_Vertex);
    vNormal = gl_NormalMatrix * gl_Normal;
    gl_FrontColor = gl_Color;               // the part's colour: ka and kd
    gl_Position = ftransform();
}
)GLSL";

// In SHADOW RAYS mode it also traces a ray from the pixel's point to each
// bulb, against the boxes and cylinders in shadows.h; a bulb the ray cannot
// reach adds no diffuse or specular light there.  The array sizes match
// MAX_BOX and MAX_CYL in shadows.h.
inline const char* FRAGMENT = R"GLSL(#version 110
uniform bool uShadows;
uniform mat4 uEyeToWorld;       // undoes the camera: eye space back to world
uniform int  uBoxes, uCyls;     // how much of each array is in use
uniform vec3 uBoxLo[24], uBoxHi[24];
uniform vec4 uCyl[32];          // two centre coordinates, radius, axis 0/1/2 = x/y/z
uniform vec2 uCylEnds[32];      // where it starts and stops along its axis

varying vec3 vNormal;
varying vec3 vPosition;

// The ray runs from the point (t = 0) to the bulb (t = 1).  Each test finds
// the stretch of t the ray spends inside a shape; the ray is blocked if some
// of that stretch lies between 0 and 1.
bool inside(float t0, float t1) { return max(t0, 0.0) < min(t1, 1.0); }

// A box: inside all three pairs of planes at once (the slab test).
bool hit_box(vec3 P, vec3 D, vec3 lo, vec3 hi) {
    vec3 a = (lo - P) / D, b = (hi - P) / D;
    vec3 n = min(a, b), f = max(a, b);
    return inside(max(max(n.x, n.y), n.z), min(min(f.x, f.y), f.z));
}

// A cylinder: inside its circle, a quadratic in t, and between its two ends.
// p, d are the ray across the axis; pa, da the ray along it.
bool hit_cyl(vec2 p, vec2 d, float pa, float da, vec4 c, vec2 ends) {
    p -= c.xy;
    float A = dot(d, d), B = dot(p, d), C = dot(p, p) - c.z * c.z;
    float disc = B * B - A * C;
    if (disc < 0.0) return false;           // the ray never meets the circle
    float s = sqrt(disc);
    float e0 = (ends.x - pa) / da, e1 = (ends.y - pa) / da;
    return inside(max((-B - s) / A, min(e0, e1)), min((-B + s) / A, max(e0, e1)));
}

bool blocked(vec3 P, vec3 D) {
    for (int i = 0; i < 24; ++i) {
        if (i >= uBoxes) break;
        if (hit_box(P, D, uBoxLo[i], uBoxHi[i])) return true;
    }
    for (int i = 0; i < 32; ++i) {
        if (i >= uCyls) break;
        vec4 c = uCyl[i];
        vec2 e = uCylEnds[i];
        if      (c.w < 0.5) { if (hit_cyl(P.yz, D.yz, P.x, D.x, c, e)) return true; }
        else if (c.w < 1.5) { if (hit_cyl(P.xz, D.xz, P.y, D.y, c, e)) return true; }
        else                { if (hit_cyl(P.xy, D.xy, P.z, D.z, c, e)) return true; }
    }
    return false;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(-vPosition);         // the eye is at the origin
    vec3 k = gl_Color.rgb;
    vec3 I = gl_FrontMaterial.emission.rgb + k * gl_LightModel.ambient.rgb;

    for (int i = 0; i < 3; ++i) {
        vec4 lp = gl_LightSource[i].position;
        vec3 L;
        float att = 1.0;
        if (lp.w == 0.0) {                  // daylight: only a direction
            L = normalize(lp.xyz);
        } else {                            // a bulb: a point, attenuated
            vec3 toL = lp.xyz - vPosition;
            float d = length(toL);
            L = toL / d;
            att = 1.0 / (gl_LightSource[i].constantAttenuation
                       + gl_LightSource[i].linearAttenuation * d
                       + gl_LightSource[i].quadraticAttenuation * d * d);
        }

        float spot = 1.0;                   // the cone: 0 outside the cutoff,
        if (gl_LightSource[i].spotCosCutoff > -0.5) {   // none for 180
            float cs = dot(-L, normalize(gl_LightSource[i].spotDirection));
            spot = cs < gl_LightSource[i].spotCosCutoff ? 0.0
                 : pow(max(cs, 0.0), gl_LightSource[i].spotExponent);
        }

        float NL = max(dot(N, L), 0.0);
        if (NL == 0.0 || spot == 0.0) continue;     // no light here anyway

        if (uShadows && lp.w != 0.0) {      // the shadow ray, in world space,
            vec3 P = vec3(uEyeToWorld * vec4(vPosition + 0.01 * N, 1.0));
            if (blocked(P, vec3(uEyeToWorld * lp) - P)) continue;
        }                                   // started just off the surface

        vec3 H = normalize(L + V);          // the half-way vector, as GL uses
        vec3 c = k * gl_LightSource[i].diffuse.rgb * NL
               + gl_FrontMaterial.specular.rgb * gl_LightSource[i].specular.rgb
                 * pow(max(dot(N, H), 0.0), gl_FrontMaterial.shininess);
        I += att * spot * c;
    }
    gl_FragColor = vec4(I, 1.0);
}
)GLSL";

inline GLuint compile(GLenum type, const char* src) {
    const GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof log, NULL, log);
        fprintf(stderr, "shader: %s\n", log);
    }
    return s;
}

// Without OpenGL 2.0 there are no shaders: program stays 0 and the s key
// skips PHONG and SHADOW RAYS.
inline void build() {
    if (!GLEW_VERSION_2_0) return;
    const GLuint p = glCreateProgram();
    glAttachShader(p, compile(GL_VERTEX_SHADER, VERTEX));
    glAttachShader(p, compile(GL_FRAGMENT_SHADER, FRAGMENT));
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (ok) program = p;
    else    fprintf(stderr, "shader: the Phong program did not link\n");
}

// Flat and Gouraud are the fixed pipeline, lit per vertex; Phong and SHADOW
// RAYS are the program, lit per pixel.
inline void apply(Mode m) {
    const bool per_pixel = (m == PHONG || m == RAYS);
    if (program) glUseProgram(per_pixel ? program : 0);
    if (program && per_pixel)
        glUniform1i(glGetUniformLocation(program, "uShadows"), m == RAYS);
    glShadeModel(m == FLAT ? GL_FLAT : GL_SMOOTH);
}

inline void off() {             // the HUD is drawn without the program
    if (program) glUseProgram(0);
}

} // namespace shade
#endif
