#ifndef SHADOWS_H
#define SHADOWS_H

#include <GL/glew.h>
#include <cmath>

#include "scene.h"
#include "camera.h"

// Ray-traced shadows, for shading mode SHADOW RAYS.  The rasterizer finds the
// point on screen; the shader then traces one ray from it to each bulb, and
// if the ray hits one of the shapes listed here, that bulb is blocked.
//
// The shapes are the parts with clear shadows, each reduced to a box or a
// cylinder and placed with the same numbers it is drawn with.  Thin or small
// parts (cleats, rod, Geneva arms, the blanks in the bin, the room's fittings)
// cast none.
namespace shadows {

using namespace cfg;

constexpr int MAX_BOX = 24, MAX_CYL = 32;     // the shader's array sizes

enum Axis { X = 0, Y, Z };

inline GLfloat box_lo[MAX_BOX][3], box_hi[MAX_BOX][3];
inline GLfloat cyl[MAX_CYL][4];       // two centre coordinates, radius, axis
inline GLfloat cyl_ends[MAX_CYL][2];  // where it starts and stops on its axis
inline int n_box = 0, n_cyl = 0;

inline void box(float x0, float y0, float z0, float x1, float y1, float z1) {
    if (n_box == MAX_BOX) return;
    GLfloat* lo = box_lo[n_box];
    GLfloat* hi = box_hi[n_box];
    lo[0] = x0; lo[1] = y0; lo[2] = z0;
    hi[0] = x1; hi[1] = y1; hi[2] = z1;
    ++n_box;
}

// A cylinder along an axis: (a, b) is its centre in the other two coordinates,
// in x, y, z order, and it runs from e0 to e1 along the axis.
inline void cylinder(Axis axis, float a, float b, float r, float e0, float e1) {
    if (n_cyl == MAX_CYL) return;
    GLfloat* c = cyl[n_cyl];
    c[0] = a; c[1] = b; c[2] = r; c[3] = (float)axis;
    cyl_ends[n_cyl][0] = e0;
    cyl_ends[n_cyl][1] = e1;
    ++n_cyl;
}

// The machine at crank angle th: the ram and the blanks move with it.
inline void machine(float th, long turns) {
    const float pz = PANEL_CZ + 0.5f * PANEL_D;            // the panel's face
    box(PANEL_CX - 0.5f * PANEL_W, PANEL_CY - 0.5f * PANEL_H, PANEL_CZ - 0.5f * PANEL_D,
        PANEL_CX + 0.5f * PANEL_W, PANEL_CY + 0.5f * PANEL_H, pz);

    for (int i = 0; i < 5; ++i)                 // gear bodies; the teeth cast none
        cylinder(Z, GEAR_X[i], GEAR_Y[i], scene::gear_root_r(i),
                 GEAR_Z - 0.5f * GEAR_T, GEAR_Z + 0.5f * GEAR_T);
    cylinder(Z, G1_X, G1_Y, MOTOR_R, MOTOR_Z0, MOTOR_Z1);

    // the press, as press.h draws it
    cylinder(Z, PRESS_X, CRANK_Y, CRANK_SHAFT_R, pz, CRANK_DISC_Z0);
    cylinder(Z, PRESS_X, CRANK_Y, CRANK_DISC_R, CRANK_DISC_Z0, CRANK_DISC_Z0 + CRANK_DISC_T);
    for (int s = -1; s <= 1; s += 2) {
        const float x = PRESS_X + s * RAIL_X_OFF;
        box(x - 0.5f * RAIL_W, RAIL_Y0, -0.5f * RAIL_D, x + 0.5f * RAIL_W, RAIL_Y1, 0.5f * RAIL_D);
        box(x - 0.5f * RAIL_W, BRACKET_Y0, pz, x + 0.5f * RAIL_W, BRACKET_Y1, -0.5f * RAIL_D);
    }
    const float ram = lay::ram_top(th);
    box(PRESS_X - 0.5f * RAM_W, ram - RAM_H, -0.5f * RAM_D,
        PRESS_X + 0.5f * RAM_W, ram,          0.5f * RAM_D);

    // the conveyor, as conveyor.h draws it
    const float xt = lay::tail_x();
    box(xt, BELT_TOP_Y - BELT_T, -0.5f * BELT_W, HEAD_X, BELT_TOP_Y, 0.5f * BELT_W);
    box(xt, ROLLER_Y - BELT_R_O, -0.5f * BELT_W, HEAD_X, ROLLER_Y - ROLLER_R, 0.5f * BELT_W);
    for (int e = 0; e < 2; ++e)                 // roller with the belt round it
        cylinder(Z, e ? HEAD_X : xt, ROLLER_Y, BELT_R_O,
                 -0.5f * ROLLER_LEN, 0.5f * ROLLER_LEN);
    const float fx0 = 0.5f * (xt + HEAD_X) - 0.5f * CFRAME_LEN, fx1 = fx0 + CFRAME_LEN;
    for (int s = -1; s <= 1; s += 2) {
        const float z = s * CFRAME_Z;
        box(fx0, CFRAME_Y - 0.5f * CFRAME_H, z - 0.5f * CFRAME_W,
            fx1, CFRAME_Y + 0.5f * CFRAME_H, z + 0.5f * CFRAME_W);
        for (int e = 0; e < 2; ++e) {
            const float lx = e ? LEG_X1 : LEG_X0;
            box(lx - 0.5f * LEG_W, 0.0f, z - 0.5f * CFRAME_W,
                lx + 0.5f * LEG_W, CFRAME_Y - 0.5f * CFRAME_H, z + 0.5f * CFRAME_W);
        }
    }

    // the blanks, where draw_blanks() puts them
    const float B = lay::belt_travel(th, turns), f = B - floorf(B);
    for (int j = 1; j <= LABELS; ++j) {
        const float x = lay::station_x(j) + f * lay::pitch();
        cylinder(Y, x, 0.0f, BLANK_R, BELT_TOP_Y, BELT_TOP_Y + lay::blank_h_at(x, th));
    }

    cylinder(Y, STACK_X, STACK_Z, STACK_POST_R, 0.0f, STACK_POST_H);
    cylinder(Y, STACK_X, STACK_Z, STACK_R, STACK_POST_H, STACK_Y0 + 3.0f * STACK_SEG_H);
}

// The room's floor items and the shelf, as room.h draws them.
inline void room_parts() {
    box(CAB_X0, 0.0f, ROOM_Z0 + 0.04f, CAB_X1, CAB_H, ROOM_Z0 + CAB_D);
    for (int k = 0; k < 2; ++k) {
        const float h = 0.5f * PAL_S;
        cylinder(Y, DRUM_X[k], DRUM_Z[k], DRUM_R, 0.0f, DRUM_H);
        box(PAL_X[k] - h, 0.0f, PAL_Z[k] - h, PAL_X[k] + h, PAL_TOP, PAL_Z[k] + h);
    }
    box(ROOM_X0, SHELF_Y - 0.05f, SHELF_Z0, ROOM_X0 + SHELF_D, SHELF_Y, SHELF_Z1);

    // the bin's four walls, as build_bin() draws them: open at the top, so
    // the blanks inside still get light
    const float x0 = BIN_X - 0.5f * BIN_S, x1 = BIN_X + 0.5f * BIN_S;
    const float z0 = BIN_Z - 0.5f * BIN_S, z1 = BIN_Z + 0.5f * BIN_S, t = BIN_T;
    box(x0,     0.0f, z0,     x1,     BIN_H, z0 + t);
    box(x0,     0.0f, z1 - t, x1,     BIN_H, z1);
    box(x0,     0.0f, z0 + t, x0 + t, BIN_H, z1 - t);
    box(x1 - t, 0.0f, z0 + t, x1,     BIN_H, z1 - t);
}

// Rebuilds the list for this frame and hands it to the program, which must be
// in use.  The camera must be the only thing on the modelview matrix.
inline void upload(GLuint prog, float th, long turns) {
    n_box = n_cyl = 0;
    machine(th, turns);
    room_parts();

    // Eye space back to world, for the shader: the camera's rotation
    // transposed (a rotation's inverse), then moved back out to the eye.
    GLfloat m[16], inv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) inv[c * 4 + r] = m[r * 4 + c];
    inv[3] = inv[7] = inv[11] = 0.0f;
    inv[12] = cam::eye_pos[0]; inv[13] = cam::eye_pos[1]; inv[14] = cam::eye_pos[2];
    inv[15] = 1.0f;

    glUniformMatrix4fv(glGetUniformLocation(prog, "uEyeToWorld"), 1, GL_FALSE, inv);
    glUniform1i (glGetUniformLocation(prog, "uBoxes"), n_box);
    glUniform3fv(glGetUniformLocation(prog, "uBoxLo"), n_box, &box_lo[0][0]);
    glUniform3fv(glGetUniformLocation(prog, "uBoxHi"), n_box, &box_hi[0][0]);
    glUniform1i (glGetUniformLocation(prog, "uCyls"), n_cyl);
    glUniform4fv(glGetUniformLocation(prog, "uCyl"), n_cyl, &cyl[0][0]);
    glUniform2fv(glGetUniformLocation(prog, "uCylEnds"), n_cyl, &cyl_ends[0][0]);
}

} // namespace shadows
#endif
