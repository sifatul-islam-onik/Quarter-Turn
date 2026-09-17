// prim.h - one box routine and one cylinder routine generate every part in the
// scene at its true size (PRD 4.3: glScalef appears exactly once in the frame,
// and it is the blank squash).
//
// Winding is CCW seen from outside on every face, without exception.  Back-face
// culling is on throughout, so a strip that runs the wrong way silently deletes
// half a cylinder - which reads as a modelling mistake rather than a state one
// (PRD FR-15).  Culling is also what stops the scene's coincident,
// opposite-facing surfaces from fighting in the depth buffer (PRD 4.3).
#ifndef PRIM_H
#define PRIM_H

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cmath>
#include "config.h"

namespace prim {

using cfg::PI;

struct V2 { float x, y; };

// ---------------------------------------------------------------------------
// Box, centred on the origin.
// ---------------------------------------------------------------------------
inline void box(float sx, float sy, float sz) {
    const float x = sx * 0.5f, y = sy * 0.5f, z = sz * 0.5f;
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex3f(-x,-y, z); glVertex3f( x,-y, z); glVertex3f( x, y, z); glVertex3f(-x, y, z);
    glNormal3f(0, 0,-1);
    glVertex3f( x,-y,-z); glVertex3f(-x,-y,-z); glVertex3f(-x, y,-z); glVertex3f( x, y,-z);
    glNormal3f(1, 0, 0);
    glVertex3f( x,-y, z); glVertex3f( x,-y,-z); glVertex3f( x, y,-z); glVertex3f( x, y, z);
    glNormal3f(-1,0, 0);
    glVertex3f(-x,-y,-z); glVertex3f(-x,-y, z); glVertex3f(-x, y, z); glVertex3f(-x, y,-z);
    glNormal3f(0, 1, 0);
    glVertex3f(-x, y, z); glVertex3f( x, y, z); glVertex3f( x, y,-z); glVertex3f(-x, y,-z);
    glNormal3f(0,-1, 0);
    glVertex3f(-x,-y,-z); glVertex3f( x,-y,-z); glVertex3f( x,-y, z); glVertex3f(-x,-y, z);
    glEnd();
}

// Box from one corner to the other, which is how most of the scene's numbers
// are stated in the PRD ("y 3.24 -> 4.60", "x 3.67 -> 4.45").
inline void box_span(float x0, float y0, float z0,
                     float x1, float y1, float z1) {
    glPushMatrix();
    glTranslatef(0.5f * (x0 + x1), 0.5f * (y0 + y1), 0.5f * (z0 + z1));
    box(x1 - x0, y1 - y0, z1 - z0);
    glPopMatrix();
}

// ---------------------------------------------------------------------------
// Cylinder along +Y with its base at y = 0, capped at both ends.  FR-7 needs
// the blank to rise from y = 0 so the squash can be scaled about its base.
//
// `chamfer` breaks both end edges at 45 degrees, and it is not decoration.
// Every other normal in this scene lies in an axis plane - boxes face along
// +/-x, +/-y, +/-z, and a cylinder's wall normals stay in the plane normal to
// its axis.  With the original press lamp 69 degrees above the die and the
// default camera 16 degrees above the belt, the half vector sat at about 43
// degrees, so NO surface in the scene faced it and the mandatory specular
// highlight evaluated to about 1e-12 everywhere at ns 89.6.  A 45-degree
// chamfer supplies normals at every azimuth on a 45-degree cone, which
// contained that half vector to within about 1.5 degrees - the whole
// difference between a blazing highlight and none at all.  The hanging bulbs
// that replaced the lamp still rely on it: a blank catches a highlight on its
// chamfer as it passes under a bulb.  Real stamped parts have a broken edge
// anyway, so this costs nothing in honesty.
// ---------------------------------------------------------------------------
inline void cyl(float r, float h, int slices, float chamfer = 0.0f) {
    float c = chamfer;
    if (c > 0.4f * h) c = 0.4f * h;
    if (c > 0.4f * r) c = 0.4f * r;
    const float ri = r - c;                 // radius at the two end caps
    const float k  = 0.70710678f;           // a 45-degree chamfer's normal

    // Surface of revolution, walked bottom to top.  For every band the lower
    // ring is emitted before the upper one and the azimuth increases, which is
    // what makes the quads wind CCW as seen from outside.
    if (c > 0.0f) {                         // bottom chamfer, normal down+out
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= slices; ++i) {
            const float a = 2.0f * PI * i / slices, cs = cosf(a), sn = sinf(a);
            glNormal3f(k * cs, -k, k * sn);
            glVertex3f(ri * cs, 0.0f, ri * sn);
            glVertex3f(r  * cs, c,    r  * sn);
        }
        glEnd();
    }

    glBegin(GL_QUAD_STRIP);                 // the wall
    for (int i = 0; i <= slices; ++i) {
        const float a = 2.0f * PI * i / slices, cs = cosf(a), sn = sinf(a);
        glNormal3f(cs, 0.0f, sn);
        glVertex3f(r * cs, c,     r * sn);
        glVertex3f(r * cs, h - c, r * sn);
    }
    glEnd();

    if (c > 0.0f) {                         // top chamfer, normal up+out
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= slices; ++i) {
            const float a = 2.0f * PI * i / slices, cs = cosf(a), sn = sinf(a);
            glNormal3f(k * cs, k, k * sn);
            glVertex3f(r  * cs, h - c, r  * sn);
            glVertex3f(ri * cs, h,     ri * sn);
        }
        glEnd();
    }

    glBegin(GL_TRIANGLE_FAN);                       // top cap, +Y
    glNormal3f(0, 1, 0);
    glVertex3f(0, h, 0);
    for (int i = slices; i >= 0; --i) {
        const float a = 2.0f * PI * i / slices;
        glVertex3f(ri * cosf(a), h, ri * sinf(a));
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);                       // bottom cap, -Y
    glNormal3f(0, -1, 0);
    glVertex3f(0, 0, 0);
    for (int i = 0; i <= slices; ++i) {
        const float a = 2.0f * PI * i / slices;
        glVertex3f(ri * cosf(a), 0.0f, ri * sinf(a));
    }
    glEnd();
}

// Every rotating shaft in the scene is parallel to z (PRD 4.1), so this is the
// form most of the machine uses.  Base at z = 0, extending to z = +h.
inline void cyl_z(float r, float h, int slices, float chamfer = 0.0f) {
    glPushMatrix();
    glRotatef(90.0f, 1, 0, 0);      // maps +Y to +Z
    cyl(r, h, slices, chamfer);
    glPopMatrix();
}

// ---------------------------------------------------------------------------
// Subdivided surfaces.  In fixed-function mode lighting is evaluated per
// vertex, so a large surface near a light must be subdivided or the falloff
// across it is lost to interpolation between four corners.  These are the
// only places the project deliberately spends triangles (PRD FR-10).
// ---------------------------------------------------------------------------

// Flat grid in the XZ plane, facing +Y.
inline void grid_xz(float x0, float x1, float z0, float z1,
                    int nx, int nz, float y) {
    const float dx = (x1 - x0) / nx, dz = (z1 - z0) / nz;
    glNormal3f(0, 1, 0);
    for (int i = 0; i < nx; ++i) {
        const float xa = x0 + i * dx, xb = xa + dx;
        glBegin(GL_QUAD_STRIP);
        for (int k = 0; k <= nz; ++k) {
            const float z = z0 + k * dz;
            glVertex3f(xb, y, z);   // +x first: a quad strip emitted the other
            glVertex3f(xa, y, z);   // way round faces -Y and is culled away
        }
        glEnd();
    }
}

// Centred box whose +Z face is subdivided nx by ny.  The drive panel.
inline void plate_z(float sx, float sy, float sz, int nx, int ny) {
    const float x = sx * 0.5f, y = sy * 0.5f, z = sz * 0.5f;
    const float dx = sx / nx, dy = sy / ny;
    glNormal3f(0, 0, 1);
    for (int j = 0; j < ny; ++j) {
        const float ya = -y + j * dy, yb = ya + dy;
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= nx; ++i) {
            const float xi = -x + i * dx;
            glVertex3f(xi, yb, z);  // +y first, so the strip faces +Z
            glVertex3f(xi, ya, z);
        }
        glEnd();
    }
    glBegin(GL_QUADS);
    glNormal3f(0, 0,-1);
    glVertex3f( x,-y,-z); glVertex3f(-x,-y,-z); glVertex3f(-x, y,-z); glVertex3f( x, y,-z);
    glNormal3f(1, 0, 0);
    glVertex3f( x,-y, z); glVertex3f( x,-y,-z); glVertex3f( x, y,-z); glVertex3f( x, y, z);
    glNormal3f(-1,0, 0);
    glVertex3f(-x,-y,-z); glVertex3f(-x,-y, z); glVertex3f(-x, y, z); glVertex3f(-x, y,-z);
    glNormal3f(0, 1, 0);
    glVertex3f(-x, y, z); glVertex3f( x, y, z); glVertex3f( x, y,-z); glVertex3f(-x, y,-z);
    glNormal3f(0,-1, 0);
    glVertex3f(-x,-y,-z); glVertex3f( x,-y,-z); glVertex3f( x,-y, z); glVertex3f(-x,-y, z);
    glEnd();
}

// Box spanning x0..x1 whose +Y face is split into n segments along x.  The
// belt's static top strip.
inline void slab_x(float x0, float x1, float y0, float y1,
                   float z0, float z1, int n) {
    const float dx = (x1 - x0) / n;
    glNormal3f(0, 1, 0);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= n; ++i) {
        const float x = x0 + i * dx;
        glVertex3f(x, y1, z0);      // -z first, so the strip faces +Y
        glVertex3f(x, y1, z1);
    }
    glEnd();
    glBegin(GL_QUADS);
    glNormal3f(0,-1, 0);
    glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1); glVertex3f(x0, y0, z1);
    glNormal3f(0, 0, 1);
    glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
    glNormal3f(0, 0,-1);
    glVertex3f(x1, y0, z0); glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
    glNormal3f(1, 0, 0);
    glVertex3f(x1, y0, z1); glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
    glNormal3f(-1,0, 0);
    glVertex3f(x0, y0, z0); glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);
    glEnd();
}

// Rectangle from corner o along edges u and v, split nu by nv, facing u x v.
// The room's walls and ceiling: the bulbs are evaluated per vertex, so a wall
// drawn as a single quad would light as one flat colour.
inline void grid(float ox, float oy, float oz,
                 float ux, float uy, float uz,
                 float vx, float vy, float vz, int nu, int nv) {
    const float nx = uy * vz - uz * vy;
    const float ny = uz * vx - ux * vz;
    const float nz = ux * vy - uy * vx;
    const float len = sqrtf(nx * nx + ny * ny + nz * nz);
    glNormal3f(nx / len, ny / len, nz / len);
    for (int i = 0; i < nu; ++i) {
        const float a = (float)i / nu, b = (float)(i + 1) / nu;
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= nv; ++j) {     // i before i+1: the strip faces u x v
            const float t = (float)j / nv;
            glVertex3f(ox + a * ux + t * vx, oy + a * uy + t * vy, oz + a * uz + t * vz);
            glVertex3f(ox + b * ux + t * vx, oy + b * uy + t * vy, oz + b * uz + t * vz);
        }
        glEnd();
    }
}

// Surface of revolution about +Y.  `p` is the profile as (radius, height),
// walked bottom to top; a radius of 0 closes the end.  Each vertex normal is
// the average of its two segments' normals, so the bulb's glass shades as a
// smooth globe rather than a stack of bands.
inline void lathe(const V2* p, int n, int slices) {
    V2 nrm[32];
    for (int i = 0; i < n; ++i) {
        float nr = 0.0f, ny = 0.0f;
        for (int j = i - 1; j <= i; ++j) {          // segments j -> j+1 touching i
            if (j < 0 || j + 1 >= n) continue;
            const float dr = p[j + 1].x - p[j].x, dy = p[j + 1].y - p[j].y;
            const float len = sqrtf(dr * dr + dy * dy);
            if (len < 1e-7f) continue;
            nr += dy / len;                         // outward for a walk upward
            ny -= dr / len;
        }
        const float len = sqrtf(nr * nr + ny * ny);
        nrm[i].x = len > 0.0f ? nr / len : 0.0f;
        nrm[i].y = len > 0.0f ? ny / len : 1.0f;
    }
    for (int i = 0; i + 1 < n; ++i) {
        glBegin(GL_QUAD_STRIP);                     // lower ring first, as cyl()
        for (int k = 0; k <= slices; ++k) {
            const float a = 2.0f * PI * k / slices, cs = cosf(a), sn = sinf(a);
            glNormal3f(nrm[i].x * cs, nrm[i].y, nrm[i].x * sn);
            glVertex3f(p[i].x * cs, p[i].y, p[i].x * sn);
            glNormal3f(nrm[i + 1].x * cs, nrm[i + 1].y, nrm[i + 1].x * sn);
            glVertex3f(p[i + 1].x * cs, p[i + 1].y, p[i + 1].x * sn);
        }
        glEnd();
    }
}

// ---------------------------------------------------------------------------
// Extruded strip.  `in` and `out` are two 2D boundaries of the same length,
// both walked with increasing index and anticlockwise about the origin; the
// material lies between them.  Extruded from z0 to z1 as a closed shell.
//
// This builds the belt's two half-shells (inner and outer arcs of the wrap) and
// the Geneva wheel's four arms (a constant inner radius across the arm, rising
// to meet the rim along each slot wall).  Where in[k] == out[k] the end cap
// collapses to nothing, which is exactly what the wheel's arms want.
// ---------------------------------------------------------------------------
inline void wall_quad(V2 p, V2 q, float z0, float z1) {
    const float dx = q.x - p.x, dy = q.y - p.y;
    const float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-7f) return;                        // degenerate cap
    glNormal3f(dy / len, -dx / len, 0.0f);          // outward for a CCW walk
    glVertex3f(p.x, p.y, z0); glVertex3f(q.x, q.y, z0);
    glVertex3f(q.x, q.y, z1); glVertex3f(p.x, p.y, z1);
}

inline void extrude_strip(const V2* in, const V2* out, int n,
                          float z0, float z1) {
    glNormal3f(0, 0, 1);                            // front cap
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i < n; ++i) {
        glVertex3f(in[i].x,  in[i].y,  z1);
        glVertex3f(out[i].x, out[i].y, z1);
    }
    glEnd();
    glNormal3f(0, 0,-1);                            // back cap
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i < n; ++i) {
        glVertex3f(out[i].x, out[i].y, z0);
        glVertex3f(in[i].x,  in[i].y,  z0);
    }
    glEnd();

    glBegin(GL_QUADS);                              // walls, CCW around the face
    wall_quad(in[0], out[0], z0, z1);               //   start cap
    for (int i = 0; i + 1 < n; ++i)
        wall_quad(out[i], out[i + 1], z0, z1);      //   outer
    wall_quad(out[n - 1], in[n - 1], z0, z1);       //   end cap
    for (int i = n - 1; i > 0; --i)
        wall_quad(in[i], in[i - 1], z0, z1);        //   inner
    glEnd();
}

} // namespace prim
#endif
