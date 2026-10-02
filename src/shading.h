#ifndef SHADING_H
#define SHADING_H

#include <GL/glew.h>
#include <cstdio>

namespace shade {

enum Mode { FLAT = 0, GOURAUD, PHONG, MODE_COUNT };
inline const char* NAME[MODE_COUNT] = { "FLAT", "GOURAUD", "PHONG" };

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

inline const char* FRAGMENT = R"GLSL(#version 110
varying vec3 vNormal;
varying vec3 vPosition;
void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(-vPosition);         // the eye is at the origin
    vec3 k = gl_Color.rgb;
    vec3 I = gl_FrontMaterial.emission.rgb + k * gl_LightModel.ambient.rgb;

    for (int i = 0; i < 2; ++i) {
        vec3  toL = gl_LightSource[i].position.xyz - vPosition;
        float d = length(toL);
        vec3  L = toL / d;
        float att = 1.0 / (gl_LightSource[i].constantAttenuation
                         + gl_LightSource[i].linearAttenuation * d
                         + gl_LightSource[i].quadraticAttenuation * d * d);

        float spot = 1.0;                   // the cone: 0 outside the cutoff,
        if (gl_LightSource[i].spotCosCutoff > -0.5) {   // none for 180
            float cs = dot(-L, normalize(gl_LightSource[i].spotDirection));
            spot = cs < gl_LightSource[i].spotCosCutoff ? 0.0
                 : pow(max(cs, 0.0), gl_LightSource[i].spotExponent);
        }

        float NL = max(dot(N, L), 0.0);
        vec3 c = k * gl_LightSource[i].diffuse.rgb * NL;
        if (NL > 0.0) {
            vec3 H = normalize(L + V);      // the half-way vector, as GL uses
            c += gl_FrontMaterial.specular.rgb * gl_LightSource[i].specular.rgb
               * pow(max(dot(N, H), 0.0), gl_FrontMaterial.shininess);
        }
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
        char log[512];
        glGetShaderInfoLog(s, sizeof log, NULL, log);
        fprintf(stderr, "shader: %s\n", log);
    }
    return s;
}

// Without OpenGL 2.0 there are no shaders: program stays 0 and the s key
// skips PHONG.
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

// Flat and Gouraud are the fixed pipeline, lit per vertex; Phong is the
// program, lit per pixel.
inline void apply(Mode m) {
    if (program) glUseProgram(m == PHONG ? program : 0);
    glShadeModel(m == FLAT ? GL_FLAT : GL_SMOOTH);
}

inline void off() {             // the HUD is drawn without the program
    if (program) glUseProgram(0);
}

} // namespace shade
#endif
