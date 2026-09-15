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
// ---------------------------------------------------------------------------
inline void cyl(float r, float h, int slices) {
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; ++i) {
        const float a = 2.0f * PI * i / slices, c = cosf(a), s = sinf(a);
        glNormal3f(c, 0.0f, s);
        glVertex3f(r * c, 0.0f, r * s);
        glVertex3f(r * c, h,    r * s);
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);                       // top cap, +Y
    glNormal3f(0, 1, 0);
    glVertex3f(0, h, 0);
    for (int i = slices; i >= 0; --i) {
        const float a = 2.0f * PI * i / slices;
        glVertex3f(r * cosf(a), h, r * sinf(a));
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);                       // bottom cap, -Y
    glNormal3f(0, -1, 0);
    glVertex3f(0, 0, 0);
    for (int i = 0; i <= slices; ++i) {
        const float a = 2.0f * PI * i / slices;
        glVertex3f(r * cosf(a), 0.0f, r * sinf(a));
    }
    glEnd();
}

// Every rotating shaft in the scene is parallel to z (PRD 4.1), so this is the
// form most of the machine uses.  Base at z = 0, extending to z = +h.
inline void cyl_z(float r, float h, int slices) {
    glPushMatrix();
    glRotatef(90.0f, 1, 0, 0);      // maps +Y to +Z
    cyl(r, h, slices);
    glPopMatrix();
}

// ---------------------------------------------------------------------------
// Subdivided surfaces.  In fixed-function mode the spotlight cone is evaluated
// per vertex, so the surfaces it falls on must be subdivided or its edge is
// invisible.  These are the only places the project deliberately spends
// triangles (PRD FR-10).
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
