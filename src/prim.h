
#ifndef PRIM_H
#define PRIM_H

#include <GL/freeglut.h>
#include <cmath>
#include "config.h"

namespace prim {

using cfg::PI;
using cfg::CYL_SLICES;

inline void box(float sx, float sy, float sz) {
    const float x = sx * 0.5f, y = sy * 0.5f, z = sz * 0.5f;
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex3f(-x,-y, z); glVertex3f( x,-y, z); glVertex3f( x, y, z); glVertex3f(-x, y, z);
    glNormal3f(0, 0,-1);
    glVertex3f( x,-y,-z); glVertex3f(-x,-y,-z); glVertex3f(-x, y,-z); glVertex3f( x, y,-z);
    glNormal3f(1, 0, 0);
    glVertex3f( x,-y, z); glVertex3f( x,-y,-z); glVertex3f( x, y,-z); glVertex3f( x, y, z);
    glNormal3f(-1, 0, 0);
    glVertex3f(-x,-y,-z); glVertex3f(-x,-y, z); glVertex3f(-x, y, z); glVertex3f(-x, y,-z);
    glNormal3f(0, 1, 0);
    glVertex3f(-x, y, z); glVertex3f( x, y, z); glVertex3f( x, y,-z); glVertex3f(-x, y,-z);
    glNormal3f(0,-1, 0);
    glVertex3f(-x,-y,-z); glVertex3f( x,-y,-z); glVertex3f( x,-y, z); glVertex3f(-x,-y, z);
    glEnd();
}
inline void box_span(float x0, float y0, float z0,
                     float x1, float y1, float z1) {
    glPushMatrix();
    glTranslatef(0.5f * (x0 + x1), 0.5f * (y0 + y1), 0.5f * (z0 + z1));
    box(x1 - x0, y1 - y0, z1 - z0);
    glPopMatrix();
}

inline void cyl(float r, float h, int slices = CYL_SLICES) {
    glBegin(GL_QUAD_STRIP);                         // the wall, bottom to top.
    for (int i = 0; i <= slices; ++i) {             // Azimuth increases and the
        const float a = 2.0f * PI * i / slices;     // lower vertex comes first,
        const float cs = cosf(a), sn = sinf(a);     // which winds it CCW outside
        glNormal3f(cs, 0.0f, sn);                   // radial: the two vertices
        glVertex3f(r * cs, 0.0f, r * sn);           // of a rung share it, so
        glVertex3f(r * cs, h,    r * sn);           // GL_SMOOTH rounds the wall
    }
    glEnd();

    glBegin(GL_POLYGON);                            // top cap, +Y
    glNormal3f(0, 1, 0);
    for (int i = slices - 1; i >= 0; --i) {
        const float a = 2.0f * PI * i / slices;
        glVertex3f(r * cosf(a), h, r * sinf(a));
    }
    glEnd();

    glBegin(GL_POLYGON);                            // bottom cap, -Y
    glNormal3f(0,-1, 0);
    for (int i = 0; i < slices; ++i) {
        const float a = 2.0f * PI * i / slices;
        glVertex3f(r * cosf(a), 0.0f, r * sinf(a));
    }
    glEnd();
}

inline void cyl_z(float r, float h, int slices = CYL_SLICES) {
    glPushMatrix();
    glRotatef(90.0f, 1, 0, 0);      // maps +Y to +Z
    cyl(r, h, slices);
    glPopMatrix();
}

inline void tiles_y(float x0, float z0, float x1, float z1, float y,
                    int nx, int nz, bool up) {
    for (int i = 0; i < nz; ++i) {
        const float za = z0 + (z1 - z0) * i / nz;
        const float zb = z0 + (z1 - z0) * (i + 1) / nz;
        glBegin(GL_QUAD_STRIP);
        glNormal3f(0, up ? 1.0f : -1.0f, 0);
        for (int j = 0; j <= nx; ++j) {
            const float x = x0 + (x1 - x0) * j / nx;
            glVertex3f(x, y, up ? za : zb);     // za before zb winds the strip
            glVertex3f(x, y, up ? zb : za);     // CCW seen from +Y
        }
        glEnd();
    }
}

inline void tiles_z(float x0, float y0, float x1, float y1, float z,
                    int nu, int nv) {
    for (int i = 0; i < nu; ++i) {
        const float xa = x0 + (x1 - x0) * i / nu;
        const float xb = x0 + (x1 - x0) * (i + 1) / nu;
        glBegin(GL_QUAD_STRIP);
        glNormal3f(0, 0, 1);
        for (int j = 0; j <= nv; ++j) {         // xa before xb winds the strip
            const float y = y0 + (y1 - y0) * j / nv;     // CCW seen from +Z
            glVertex3f(xa, y, z);
            glVertex3f(xb, y, z);
        }
        glEnd();
    }
}

} // namespace prim
#endif
