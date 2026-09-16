// shaders.h - PRD FR-12.  The per-pixel Phong mode, in GLSL 1.10.
//
// Embedded as string literals rather than loaded from shaders/*.glsl so there
// is no runtime path to get wrong; the source below is plain GLSL and reads
// exactly as a .frag file would.
//
// The shader needs NO uniforms for its lighting.  GLSL 1.10 exposes the
// fixed-function state directly - gl_LightSource[i], gl_FrontMaterial,
// gl_LightModel - so the three shading modes read the *identical* lights and
// materials.  That makes the toggle a genuine controlled comparison rather
// than three differently tuned looks, and removes an entire category of bug.
//
// The one uniform present is uNormalize, and it is there deliberately.  The
// 'n' key toggles GL_NORMALIZE, which is fixed-function state that this mode
// does not use, so pressing it in Phong mode would otherwise do nothing and
// look broken.  PRD FR-13 gives two acceptable answers and asks which was
// chosen: this is the second one - the shader reads the flag and skips its own
// normalize() call, mirroring the fixed pipeline's behaviour exactly.
#ifndef SHADERS_H
#define SHADERS_H

namespace shader_src {

// gl_NormalMatrix is exactly the inverse transpose of the modelview's upper
// 3x3 - literally the (M^-1)^T of the slide.  It is not normalised, though, so
// the normalize() below is the shader-side equivalent of GL_NORMALIZE.
//
// ftransform() rather than the matrices by hand, so that vertex positions are
// bit-identical to the fixed pipeline's and switching modes cannot shift a
// single fragment.
inline const char* VERTEX = R"GLSL(#version 110
varying vec3 vNormal;     // eye space, deliberately NOT normalised here
varying vec3 vPosition;   // eye space

void main()
{
    vec4 p = gl_ModelViewMatrix * gl_Vertex;
    vPosition = p.xyz;

    // The raw transformed normal is handed on with its length intact, so the
    // fragment stage can decide whether to normalise it.  Normalising here
    // instead would make uNormalize unobservable: the fragment stage has to
    // renormalise after interpolation anyway, which would silently repair the
    // very error the 'n' key exists to show.
    vNormal = gl_NormalMatrix * gl_Normal;

    gl_Position = ftransform();
}
)GLSL";

// The illumination equation, in the slides' own form:
//
//     I = ka*Ia + kd*Il*max(N.L, 0) + ks*Il*max(V.R, 0)^ns + Ie
//     R = 2(N.L)N - L
//
// R is spelled out rather than using GLSL's reflect() so the source visibly
// matches the slide.
//
// This is TRUE Phong, and that is the point of the mode.  Fixed-function
// OpenGL does not implement the Phong specular term at all: it forms the half
// vector H = (L + V)/|L + V| and raises N.H to the shininess, which is
// Blinn-Phong - the cheaper alternative the slides present alongside.  The
// angle between N and H is roughly half the angle between V and R, so for the
// same exponent the half-vector highlight is noticeably broader.  Highlights
// therefore TIGHTEN when this mode engages, before the per-fragment effect is
// even considered, and matching a Blinn highlight to a Phong one needs about
// four times the exponent.
//
// Two details that cost time if unknown.  spotCosCutoff is the COSINE of the
// cutoff angle, not the angle.  And gl_LightSource[i].position is already in
// EYE SPACE, because glLightfv transformed it by the modelview when it was
// set - so every vector here is an eye-space vector and nothing is transformed
// a second time.
//
// Every literal is written as a float.  Intel's GLSL compiler rejects an
// implicit int-to-float conversion outright rather than warning, which
// produces a confusing first-run failure with nothing to do with the lighting.
inline const char* FRAGMENT = R"GLSL(#version 110
uniform bool uNormalize;

varying vec3 vNormal;
varying vec3 vPosition;

void main()
{
    // This one line is the shader-side GL_NORMALIZE.  With the flag set it is
    // the ordinary per-fragment renormalisation Phong shading needs; with it
    // clear the interpolated normal is used at whatever length the normal
    // matrix gave it, exactly as the fixed pipeline does, so the 'n' key
    // produces the same blown-out stamped blanks in all three modes.
    vec3 N = uNormalize ? normalize(vNormal) : vNormal;
    vec3 V = normalize(-vPosition);     // the eye is the origin in eye space

    // ka*Ia, plus the emission term the stack light uses
    vec3 I = gl_FrontMaterial.emission.rgb
           + gl_FrontMaterial.ambient.rgb * gl_LightModel.ambient.rgb;

    for (int i = 0; i < 2; ++i) {
        vec3  toLight = gl_LightSource[i].position.xyz - vPosition;
        float d = length(toLight);
        vec3  L = toLight / d;

        // the slides' radial attenuation, evaluated per fragment
        float att = 1.0 / (gl_LightSource[i].constantAttenuation
                         + gl_LightSource[i].linearAttenuation    * d
                         + gl_LightSource[i].quadraticAttenuation * d * d);

        // the spotlight cone, also per fragment.  A cutoff of 180 degrees
        // gives spotCosCutoff = -1 and means "no cone at all".
        float spot = 1.0;
        if (gl_LightSource[i].spotCosCutoff > -0.99) {
            float cd = dot(-L, normalize(gl_LightSource[i].spotDirection));
            if (cd < gl_LightSource[i].spotCosCutoff)
                spot = 0.0;
            else
                spot = pow(cd, gl_LightSource[i].spotExponent);
        }

        float ndotl = max(dot(N, L), 0.0);

        vec3 c = gl_FrontMaterial.ambient.rgb * gl_LightSource[i].ambient.rgb
               + gl_FrontMaterial.diffuse.rgb * gl_LightSource[i].diffuse.rgb
                 * ndotl;

        if (ndotl > 0.0) {
            vec3 R = 2.0 * ndotl * N - L;
            c += gl_FrontMaterial.specular.rgb * gl_LightSource[i].specular.rgb
               * pow(max(dot(V, R), 0.0), gl_FrontMaterial.shininess);
        }

        I += att * spot * c;
    }

    gl_FragColor = vec4(I, gl_FrontMaterial.diffuse.a);
}
)GLSL";

} // namespace shader_src
#endif
