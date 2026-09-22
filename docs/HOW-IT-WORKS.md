# Quarter Turn — how it works

**An automated stamping line in OpenGL.** CSE 4207 Computer Graphics, KUET.
This document describes the `unlit-demo` build: no lighting and no shading. Every part is a flat colour, outlined by a second drawing pass.

It explains how the project was built, where every object is coded, the formula behind every motion and where each formula comes from.

---

## Contents

1. [The project in one page](#1-the-project-in-one-page)
2. [Files and program flow](#2-files-and-program-flow)
3. [Coordinates, units and angle conventions](#3-coordinates-units-and-angle-conventions)
4. [Time and the animation state](#4-time-and-the-animation-state)
5. [How shapes are built: the primitives](#5-how-shapes-are-built-the-primitives)
6. [Drawing without lighting](#6-drawing-without-lighting)
7. [Camera and projection](#7-camera-and-projection)
8. [The machine, object by object](#8-the-machine-object-by-object)
9. [The room, object by object](#9-the-room-object-by-object)
10. [Transformations used](#10-transformations-used)
11. [Hierarchy and the two closed loops](#11-hierarchy-and-the-two-closed-loops)
12. [Controls](#12-controls)
13. [Verification](#13-verification)
14. [Motion cheat sheet](#14-motion-cheat-sheet)
15. [Where the numbers came from](#15-where-the-numbers-came-from)

**Reading the references.** A link such as [kinematics.h:52](../src/kinematics.h#L52) opens the code at that line. Every tunable number lives in [config.h](../src/config.h) under the name given in the text, for example `CRANK_R`.

---

## 1. The project in one page

A motor on a green drive panel turns a train of five meshing gears. Power splits at the second gear:

```
motor ─► G1 ─► G2 (crank gear) ─┬─► crank ─► connecting rod ─► ram ─► flattens the blank
                                │
                                └─► G3 ─► G4 ─► G5 ─► Geneva driver ─► Geneva wheel
                                                                          │
                                         head roller ◄────────────────────┘
                                               │
                                      belt + cleats ─► blanks move one station
                                               │
                           finished part ─► over the roller ─► chute ─► bin
```

- The **press** strokes once per revolution of G2.
- The **Geneva mechanism** turns continuous rotation into exactly **one quarter turn** of the head roller, then locks the roller still. A quarter turn of the roller moves the belt one station, which gives the project its name.
- The belt moves **only while the press is up**. This interlock is derived from the geometry, not chosen by hand (§8.8).

**The central idea.** The whole machine's animation state is two variables:

| Variable | Type | Meaning |
|---|---|---|
| `theta` (θ) | float, radians, wrapped to [0, 2π) | angle of the crankshaft |
| `cycles` | integer, only increases | full crank revolutions so far |

Everything that moves is a **closed-form function** of these two: gears, press, Geneva wheel, belt, cleats, blanks, flying parts, the bin pile, the stack light and the counter. Nothing is keyframed, stored or interpolated between saved poses. Change the speed and every part stays in step, because every position is recomputed from θ each frame.

The exhaust fan is the only exception. It has its own on/off switch and spins up and down gradually, so it keeps its own angle (§4.4).

**Tools.** C++17, OpenGL fixed-function pipeline with freeglut for the window and input, and GLEW (used only for `glBlendColor`, an OpenGL 1.4 function that Windows' OpenGL 1.1 does not provide). Build with `.\build.ps1 -Run`.

---

## 2. Files and program flow

| File | What is in it |
|---|---|
| [config.h](../src/config.h) | every tunable number: sizes, positions, speeds, camera |
| [kinematics.h](../src/kinematics.h) | **all motion maths**. It has no OpenGL, so the test program checks the same code the renderer runs |
| [prim.h](../src/prim.h) | the shape builders: box, cylinder, grids, surface of revolution, extruded strip |
| [materials.h](../src/materials.h) | one flat colour per material |
| [scene.h](../src/scene.h) | the machine: display lists and per-frame drawing |
| [room.h](../src/room.h) | the workshop: walls, ceiling, bulbs, fan, counter, switches, floor items |
| [main.cpp](../src/main.cpp) | window, timing, state, keys, camera, the two drawing passes, the on-screen panel (HUD) |
| [tests/mathcheck.cpp](../tests/mathcheck.cpp) | headless checks of the motion maths |

### Startup — [main.cpp:main](../src/main.cpp)

1. `verify_layout()` ([main.cpp:597](../src/main.cpp#L597)) checks the key geometry before any window opens (§13).
2. `glutInit`, `glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH)` asks for a double buffer, colour and a depth buffer. Then `glutCreateWindow` and `glewInit()`, which must come after the window because it needs a live OpenGL context.
3. `init()` ([main.cpp:625](../src/main.cpp#L625)) enables the depth test and back-face culling, then compiles all display lists once: `scene::build_lists()` and `room::build_lists()`.
4. The GLUT callbacks are registered and `glutMainLoop()` starts.

### Every frame

```
idle()                                         main.cpp:496
 ├─ dt = time since last frame (clamped)
 ├─ update(dt)      θ, cycles, fan             main.cpp:81
 ├─ fly(dt)         free camera                main.cpp:194
 ├─ check_interlock()                          main.cpp:97
 └─ glutPostRedisplay()
display()                                      main.cpp:451
 ├─ clear colour and depth
 ├─ apply_camera()  gluLookAt                  main.cpp:151
 ├─ pass 1: filled faces, depth-offset         (§6.3)
 ├─ pass 2: the same geometry as dark lines
 ├─ room::draw_glow()  bulb halos              room.h:681
 ├─ hud()           2D overlay                 main.cpp:403
 └─ glutSwapBuffers()
```

`draw_world()` ([main.cpp:423](../src/main.cpp#L423)) draws everything once: `scene::draw(θ, cycles)` for the machine, then `room::draw(...)` for the room.

---

## 3. Coordinates, units and angle conventions

- **Right-handed, y up.** The floor is the plane y = 0.
- **+x** is the direction material flows, left to right. **+z** points out of the machine towards the viewer.
- **1 unit ≈ 25 cm.** The belt top at y = 3.00 is a 75 cm working height.
- **Every rotating shaft is parallel to z**, so every mechanism turns in a plane facing the camera.
- **Angle convention:** `glRotatef(angle, 0, 0, 1)` is **anticlockwise positive** seen from the front. Gear angles φ use this convention.
- **θ increases clockwise** seen from the front, which is why the crank is drawn with `glRotatef(90° − θ)` (§8.5).
- **Surfaces that share a plane face opposite ways.** Examples: a blank's bottom on the belt, a cleat's bottom on the belt, and the punch face on a flattened blank. Back-face culling discards one face of each pair, so they cannot flicker in the depth buffer. Surfaces that face the same way are kept apart: gear back faces sit 0.06 in front of the panel.

---

## 4. Time and the animation state

### 4.1 Frame time — [main.cpp:496](../src/main.cpp#L496)

```
dt = (now − previous) / 1000          seconds, from glutGet(GLUT_ELAPSED_TIME)
dt = 0                                on the very first frame
dt = min(dt, 0.1)                     DT_CLAMP
```

All motion is **speed × dt**, never "a fixed amount per frame", so the machine runs at the same real-world speed on a fast lab PC and on a throttled laptop. The clamp stops a window drag or a pause from throwing the machine forward by most of a cycle.

### 4.2 The crank angle — [main.cpp:81](../src/main.cpp#L81)

The speed is set in **parts per minute (ppm)**, because one crank revolution makes one part.

```
ω = ppm · 2π / 60                                   rad/s
θ ← θ + ω · dt
while θ ≥ 2π:   θ ← θ − 2π,   cycles ← cycles + 1
```

- **Default 30 ppm, range 6–120, step 6** (`PPM_*`). At 30 ppm one part takes 2 s.
- It is a `while`, not an `if`, so `cycles` goes up exactly once per revolution even when one `dt` covers a lot of rotation. The belt's position depends on `cycles`, so a missed increment would jump the belt.
- **Why the crankshaft is the state.** Any shaft would do, since the gear ratios are exact. The crankshaft is chosen because one revolution of it is one part, and every phase boundary (§8.8) is an angle of it.
- **Reset** (`r`) sets θ = 90°, cycles = 0. At 90° the belt is at rest (outside INDEX), the ram is descending, and the belt travel B is exactly 0.
- **Step** (`.`, machine stopped) does θ += 5° through the same wrap loop ([main.cpp:104](../src/main.cpp#L104)). It changes the state itself, not a separate animation.

### 4.3 What is computed from θ and cycles

| Quantity | Formula (section) | Function |
|---|---|---|
| gear angles φ₁…φ₅ | mesh law (§8.4) | [kinematics.h:52](../src/kinematics.h#L52) |
| ram height s | crank-slider (§8.6) | [kinematics.h:73](../src/kinematics.h#L73) |
| phase, stack light | angle ranges (§8.8) | [kinematics.h:123](../src/kinematics.h#L123) |
| Geneva wheel / roller angle | β(α), B (§8.7, §8.9) | [kinematics.h:142](../src/kinematics.h#L142), [kinematics.h:153](../src/kinematics.h#L153) |
| cleat positions | belt path (§8.11) | [kinematics.h:178](../src/kinematics.h#L178) |
| blank positions and heights | §8.12–§8.14 | [kinematics.h:216](../src/kinematics.h#L216) |
| flying parts, bin pile | stroke clock (§8.15) | [kinematics.h:251](../src/kinematics.h#L251), [kinematics.h:324](../src/kinematics.h#L324) |
| parts made | cycles + [θ ≥ 180°] (§8.18) | [kinematics.h:136](../src/kinematics.h#L136) |

### 4.4 The exception: the exhaust fan — [main.cpp:81](../src/main.cpp#L81)

The fan has its own switch (`f`) and should **spin up and run down gradually** rather than jump. That is a first-order lag towards the target speed ω\* (2.5 rev/s when on, 0 when off), with time constant τ = 0.8 s:

```
dω/dt = (ω* − ω) / τ
```

**Derivation of the update.** With ω\* held constant over one frame, the exact solution of that equation after a time dt is `ω(t+dt) = ω* + (ω − ω*)·e^(−dt/τ)`. Rearranged:

```
ω ← ω + (ω* − ω) · (1 − e^(−dt/τ))                  FAN_RPS, FAN_TAU
fan_deg ← (fan_deg + 360 · ω · dt)  mod 360
```

Because this is the exact solution rather than a step-by-step approximation, the spin-up looks the same at any frame rate.

---

## 5. How shapes are built: the primitives

There are **no loaded model files and no GLUT solid shapes.** Every object is generated by the routines in [prim.h](../src/prim.h), at its true size. Sizes are passed as arguments, so `glScalef` is used **exactly once** in the whole scene, for the blank squash (§8.14).

**Winding rule.** Every face's vertices go **anticlockwise as seen from outside**. Back-face culling is on (`glEnable(GL_CULL_FACE)`), so a face wound the wrong way would silently disappear.

### 5.1 Box — [prim.h:27](../src/prim.h#L27)

`box(sx, sy, sz)`: a box centred on the origin, six `GL_QUADS` faces with corners at `(±sx/2, ±sy/2, ±sz/2)`.
`box_span(x0,y0,z0, x1,y1,z1)` ([prim.h:47](../src/prim.h#L47)): a box from one corner to the other. It translates to the midpoint `((x0+x1)/2, …)` and draws `box(x1−x0, y1−y0, z1−z0)`. Most of the scene's boxes are stated this way ("rails from y 3.20 to 4.10").

### 5.2 Cylinder — [prim.h:62](../src/prim.h#L62)

`cyl(r, h, slices, chamfer)`: a cylinder along +y with its base at y = 0. A circle is sampled at `slices` points:

```
a_i = 2π·i / slices
point_i = (r·cos a_i,  y,  r·sin a_i)
```

- **Wall:** one `GL_QUAD_STRIP` joining the ring at the bottom to the ring at the top.
- **Chamfer** (optional): a 45° bevelled edge. One extra ring of quads at each end, from radius `r − c` at the cap to `r` at height `c`.
- **Caps:** one `GL_POLYGON` each at radius `r − c`. A polygon fills the same as a triangle fan, but in line mode it draws only its rim, with no spokes (§6.3).
- **Base at y = 0** because the blank must be scaled about its base (§8.14).

`cyl_z(...)` ([prim.h:125](../src/prim.h#L125)) turns it to point along +z, since every shaft runs along z. It applies `glRotatef(90°, 1, 0, 0)`, which maps +y to +z:

```
Rx(90°): (x, y, z) → (x,  y·cos90 − z·sin90,  y·sin90 + z·cos90) = (x, −z, y)
so (0, 1, 0) → (0, 0, 1)
```

### 5.3 Flat grids — [prim.h:138](../src/prim.h#L138)–[prim.h:212](../src/prim.h#L212)

- `grid(o, u, v, nu, nv)`: a rectangle from corner **o** along edge vectors **u** and **v**, split into nu × nv cells, facing **u × v**. This is how walls and ceiling are built.
- `grid_xz`: the floor, facing +y. `plate_z`: the drive panel. `slab_x`: the belt's top strip.
- Cell counts matter only for the look: every cell edge becomes an outline (§6.3). So the floor keeps 1.0-unit tiles, which show perspective, while the panel and belt are one cell each.

### 5.4 Surface of revolution — [prim.h:234](../src/prim.h#L234)

`lathe(profile, n, slices)` spins a 2D profile (radius, height) around the y axis. Each profile point is swept round a circle exactly as in the cylinder. It is used for the glass bulbs (§9.7).

### 5.5 Extruded strip — [prim.h:300](../src/prim.h#L300)

`extrude_strip(in[], out[], n, z0, z1)`: the flat region between an inner and an outer 2D boundary, pushed out from z0 to z1. It has a front cap, a back cap, and walls along both boundaries. It builds the **Geneva wheel's arms** (§8.7) and the **belt's two curved ends** (§8.10).

The boundaries are sampled finely, so every sample would show as a line. `glEdgeFlag(GL_FALSE)` marks the interior edges as "not an outline", so in line mode only the true outline is drawn. Edge flags are ignored by strips and fans, which is why this routine uses `GL_QUADS`.

### 5.6 Display lists (optimization)

Geometry that never changes shape is recorded **once** into a display list with `glNewList … glEndList`, then replayed with `glCallList` each frame.

| List | Contents | Drawn |
|---|---|---|
| `L_TOOTH` | one gear tooth | **124×** (12+36+20+20+36 teeth) |
| `L_CLEAT` | one cleat | **24×** |
| `L_BLANK` | one blank | up to 10× on the belt, plus the bin, the pallet and the bench |
| `L_GEAR0..4` | each gear's body and hub | 1× each, plus 2 spares on the shelf |
| `L_FLOOR`, `L_PANEL`, `L_CONVEYOR`, `L_BELT`, `L_FIXTURES`, `L_MOTOR`, `L_ROLLER`, `L_GENEVA`, `L_STACK_SEG` | static machine parts | 1× (rollers 2×, lens 3×) |
| room lists | floor items, 4 walls, ceiling, fan rotor, pendants, globe | 1× each |

Moving parts are **not** rebuilt: the list is fixed and only the matrix in front of it changes. Lists are built in [scene.h:128](../src/scene.h#L128) and [room.h:535](../src/room.h#L535).

Two other optimizations: sin θ and cos θ are computed once per frame and passed to the press ([scene.h:513](../src/scene.h#L513)), and all five gear angles come from one walk along the gear chain.

---

## 6. Drawing without lighting

### 6.1 Colour — [materials.h](../src/materials.h)

A "material" in this build is just an RGB colour: `mat::use(m)` calls `glColor3fv(m.rgb)`. `glColor` is recorded into a display list like any other call, so a static part's colour can live inside its list. A colour that **changes at runtime** is set outside the list, just before calling it: stack light lenses, counter digits, switch lamps and bulbs. A list stores the values it was compiled with and could never change them.

`mat::lens(r, g, b, lit)` gives a lamp lens its full colour when lit and `0.25·colour + 0.04` when not.

Colours were **chosen by eye** for contrast. Brass and copper alternate along the gear train, so the two gears at every mesh differ. The rod and ram are darker than the polished crank disc. The cleats are yellow, so the belt's movement is visible.

### 6.2 Hidden surfaces

- **Depth buffer** (`GLUT_DEPTH`, `glEnable(GL_DEPTH_TEST)`): each pixel keeps the nearest surface.
- **Back-face culling** (`glEnable(GL_CULL_FACE)`, `glCullFace(GL_BACK)`): faces pointing away from the camera are skipped. That is faster, and it resolves the coincident opposite-facing surface pairs (§3).
- **Near and far planes** are chosen from measurement (§7.1), which keeps depth precision high.

### 6.3 The edge pass — [main.cpp:451](../src/main.cpp#L451)

**Problem.** Without lighting, every face of a part is the same colour. A box becomes a flat hexagon, parts of the same colour merge, and a spinning cylinder looks still.

**Solution.** Draw the world twice.

**Pass 1: faces, pushed slightly back in depth.**

```
glEnable(GL_POLYGON_OFFSET_FILL);  glPolygonOffset(1, 1);
depth written = z + factor·DZ + units·r        (factor = 1, units = 1)
```

Here `DZ` is how steeply the polygon's depth changes across the screen and `r` is the smallest depth step the buffer can hold. Without the offset a line and its own face would have exactly the same depth, and the line would flicker along its length (z-fighting).

**Pass 2: the same geometry as lines** (`glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)`).

Every display list sets its own colours, so the line colour cannot simply be set to black. Instead the line's own colour is thrown away at the blending stage:

```
glBlendFunc(GL_ZERO, GL_CONSTANT_COLOR);  glBlendColor(K, K, K, 1);   K = 0.40

result = source·0 + destination·K = 0.40 × (face colour already on screen)
```

Each edge becomes a darker shade of the face under it. Without OpenGL 1.4, the fallback is the logic op `GL_CLEAR`, which writes black.

- **Hidden edges stay hidden.** Culling and the depth test apply to lines too.
- **No double darkening.** Neighbouring quads share an edge, so it is drawn twice. The first line writes its depth, and the second has *equal* depth, so it fails the default `GL_LESS` test.
- **Clean outlines.** Cylinder caps are single polygons (no spokes), and extruded shapes use edge flags (§5.5). The glass bulbs are left out of pass 2, where they would look like wire cages.

`e` turns the edges off, which shows why they are needed. `w` switches to **wireframe**: lines only, in each part's own colour.

### 6.4 Bulb halos — [room.h:681](../src/room.h#L681)

A lit bulb gets a soft disc that always faces the camera (a **billboard**), drawn last.

**Derivation of the camera axes.** When only the camera transform is on the modelview matrix M, its upper 3×3 is a pure rotation from world to eye space. The rows of a rotation matrix are the eye axes written in world coordinates. So row 0 is the camera's right vector and row 1 its up vector:

```
right = (m[0], m[4], m[8])      up = (m[1], m[5], m[9])        (OpenGL stores columns)
vertex_k = C + R_g·(cos a_k · right + sin a_k · up),   a_k = 2πk/32,   R_g = 0.45
```

The centre has alpha 0.6 and the rim alpha 0, blended additively with `GL_SRC_ALPHA, GL_ONE`. Depth is tested, so the machine can hide a bulb, but not written, so the halo cuts no holes in what is drawn later.

### 6.5 The 2D overlay (HUD) — [main.cpp:403](../src/main.cpp#L403)

After the 3D scene:

1. Disable the depth test, so the text is never behind the machine.
2. Push both matrices, and load `gluOrtho2D(0, width, 0, height)` so coordinates are pixels. `glRasterPos` goes through the matrices, so text would otherwise land off screen.
3. Draw translucent backing rectangles and bitmap text (`glutBitmapCharacter`), then pop both matrices.

The panel shows the switches, speed and parts made. `h` opens the technical readout: θ, phase, punch clearance, Geneva angle, belt travel, motor rpm, fps, the gear-freeze speed and the interlock status.

---

## 7. Camera and projection

### 7.1 Projection — [main.cpp:165](../src/main.cpp#L165)

```
gluPerspective(45°, width/height, near, far)
preset views:  near = 4.0,  far = 40      (ZNEAR, ZFAR)
free camera:   near = 0.2,  far = 40      (FREE_ZNEAR)
```

**Why near = 4 and not the usual 0.1.** A depth buffer's precision at distance z is roughly proportional to z²/near. The default eye is 11.10 from its target:

```
|(7.0, 6.2, 9.0) − (1.4, 2.9, 0.0)| = √(5.6² + 3.3² + 9²) = √123.25 = 11.10
```

Nothing visible is closer than about 9 units, so near = 4 is safe and gives about **40×** better depth precision than 0.1. That is how z-fighting is prevented. The far plane of 40 covers the farthest room corner (about 30 in the overview). The free camera flies close to parts, so it needs near = 0.2. To keep that safe it is held within 22 units of the room centre and below a height of 20.

### 7.2 Preset views — [main.cpp:115](../src/main.cpp#L115)

`gluLookAt(eye, target, up)`:

| Key | View | Eye | Target | Up |
|---|---|---|---|---|
| `1` | three-quarter, front right | (7.0, 6.2, 9.0) | (1.4, 2.9, 0) | +y |
| `2` | front elevation | (1.8, 3.2, 12.0) | (1.8, 3.2, 0) | +y |
| `3` | top plan | (1.4, 12.0, 0) | (1.4, 2.9, 0) | −z |
| `4` | whole room | (14.5, 7.0, 17.0) | (1.5, 3.6, 1.0) | +y |

The top view needs "up" = −z, because looking straight down makes +y parallel to the view direction.

### 7.3 Orbit (`←` `→`)

The eye is rotated about the vertical line through the target. With d = eye − target and orbit angle o (±3° per key press):

```
d'x =  dx·cos o + dz·sin o
d'z = −dx·sin o + dz·cos o
eye' = target + (d'x, dy, d'z)
```

This is the standard rotation about the y axis applied to the eye's offset. Its height does not change.

### 7.4 Free camera (`c`) — [main.cpp:145](../src/main.cpp#L145), [main.cpp:174](../src/main.cpp#L174), [main.cpp:194](../src/main.cpp#L194)

The view direction comes from two angles: **yaw** (turn left or right, 0 = looking down −z) and **pitch** (look up or down).

**Forward vector.** Start from (0, 0, −1). Pitch up: the vector's height becomes sin(pitch) and its horizontal length cos(pitch). Then yaw turns the horizontal part:

```
f = ( sin(yaw)·cos(pitch),   sin(pitch),   −cos(yaw)·cos(pitch) )
right r = ( cos(yaw), 0, sin(yaw) )                  kept level, so strafing never climbs
target = eye + f
```

**Movement** uses held keys and dt, like everything else:

```
eye ← eye + 3.0·dt·( fwd·f + side·r )          fwd, side ∈ {−1, 0, 1}
eye.y ← clamp(eye.y + 3.0·dt·rise, 0.3, 20)
if the horizontal distance from the room centre > 22: pull the eye back onto that circle
```

**Looking.** A mouse drag adds 0.25° per pixel to yaw, and subtracts it from pitch. Pitch is clamped to ±89°, because at 90° the view is parallel to "up" and `gluLookAt` breaks.

**Entering from a preset.** The inverse of the forward formula, with d = target − eye:

```
pitch = asin(dy / |d|)          yaw = atan2(dx, −dz)
```

### 7.5 Cutaway walls — [room.h:67](../src/room.h#L67)

A wall, and everything mounted on it, is drawn only if the eye is **on the room side** of that wall's plane:

```
back  wall (z = −2.5):  eye.z > −2.5        left  wall (x = −6):  eye.x > −6
front wall (z =  5.5):  eye.z <  5.5        right wall (x =  9):  eye.x <  9
ceiling (y = 9.5):      eye.y <  9.5
```

The walls between the camera and the machine vanish at every orbit angle. Culling alone would hide a bare wall seen from outside, but not the boxes fixed to it, such as window frames and the door.

---

## 8. The machine, object by object

Each object lists **what it is**, **how it is modelled**, **where the code is**, and **how it moves**.

### 8.1 Floor

- **Model:** a 15 × 8 grid of 1.0 tiles covering the room, x −6→9, z −2.5→5.5, at y = 0. Concrete grey.
- **Code:** list `L_FLOOR` ([scene.h:133](../src/scene.h#L133)) using `grid_xz`.
- **Motion:** none. The tile outlines give the flat-coloured scene its sense of depth.

### 8.2 Drive panel, guide rails and rail brackets

- **Model:**
  - **Panel:** a 5.2 × 5.5 × 0.10 box centred at (3.2, 2.75, −1.0), so its front face is at z = −0.95.
  - **Guide rails:** two boxes, 0.06 × 0.90 × 0.10, at x = 2.115 ± 0.24, y 3.20→4.10. They hold the ram over its whole travel (3.10→4.00).
  - **Brackets:** two arms joining the rails to the panel at y 3.60→3.76, under G2's lowest tooth tip at 3.80, and either side of the ram.
- **Code:** list `L_PANEL` ([scene.h:139](../src/scene.h#L139)).
- **Motion:** none. The panel is the parent of every gear (§11).

### 8.3 Motor

- **Model:** a black cylinder, r 0.35, from z −1.75 to −0.95 on G1's axis (2.115, 5.95). It sits above the panel's top edge, so it is visible over the panel. A silver stub shaft reaches forward into the pinion G1.
- **Code:** list `L_MOTOR` ([scene.h:158](../src/scene.h#L158)).
- **Motion:** the body is static. The motor's turning is shown by pinion G1 at **3× crank speed** (§8.4), so `motor rpm = 3 × ppm` (90 rpm at 30 ppm).

### 8.4 The gear train, G1–G5

#### Sizes

The gears use **module** m = 0.05 (`MODULE`). A gear with N teeth has **pitch radius** r = m·N/2. Two gears mesh when their centres are exactly r_i + r_j apart. The module is the standard way to size gears so that any two with the same module mesh.

| Gear | Role | Teeth N | r = mN/2 | Root r − 1.25m | Tip r + m | Centre |
|---|---|---|---|---|---|---|
| G1 | motor pinion | 12 | 0.30 | 0.2375 | 0.35 | (2.115, 5.950) |
| G2 | crank gear | 36 | 0.90 | 0.8375 | 0.95 | (2.115, 4.750) |
| G3 | idler | 20 | 0.50 | 0.4375 | 0.55 | (3.342, 4.076) |
| G4 | idler | 20 | 0.50 | 0.4375 | 0.55 | (4.010, 3.332) |
| G5 | Geneva drive gear | 36 | 0.90 | 0.8375 | 0.95 | (4.550, 2.040) |

Check: |G1 − G2| = 5.95 − 4.75 = 1.20 = 0.30 + 0.90.

#### Where G2 and G5 come from

- **G2** is on the crank axis, fixed by the press stack (§8.6): x = 2.115 (station 7), y = 4.75.
- **G5** is fixed by the Geneva geometry (§8.7): its centre is 0.7778 from the head roller axis, along the 135° line.
- **G1** sits straight above G2 at 1.20.

#### Placing the idlers G3 and G4 — [kinematics.h:397](../src/kinematics.h#L397)

The chain G2→G3→G4→G5 needs centre distances d₁ = 0.90+0.50 = **1.40**, d₂ = 0.50+0.50 = **1.00**, d₃ = 0.50+0.90 = **1.40**. The idlers are *computed*, not placed by eye.

```
D     = |G5 − G2| = √(2.435² + 2.710²) = 3.6433
û     = (G5 − G2)/D                   unit vector along the line G2→G5
n̂     = (−û_y, û_x)                   perpendicular, pointing upward
along = (D − d₂)/2 = 1.3216
off   = √(d₁² − along²) = 0.4618       Pythagoras: G2→G3 is the hypotenuse d₁
G3    = G2 + along·û + off·n̂
G4    = G3 + d₂·û
```

**Why this works.** The two outer links are equal (1.40), so the shape is a symmetric trapezoid. The middle link G3→G4 is parallel to G2→G5 and has length 1.00. Each outer link covers `along` in the direction of the line and `off` sideways. The chain bends **atan(0.4618 / 1.3216) = 19.26°** upwards, which keeps both idlers above belt level and in view.

**Why two idlers, not one.** Every mesh reverses the direction of rotation. The crank G2 turns clockwise. With two idlers there are three meshes to G5, so G5 turns anticlockwise. A Geneva wheel turns opposite to its driver, so the wheel turns clockwise, and a clockwise head roller moves the belt's top surface **+x**, the right way. With one idler the belt would run backwards. One idler would also need at least 37 teeth to span 3.64.

#### Speed ratio: derivation

At the contact point (the pitch point) the two pitch circles roll without slipping, so they move the same arc length in opposite senses:

```
r_i · Δφ_i = − r_j · Δφ_j        ⇒   Δφ_j = −(r_i / r_j)·Δφ_i = −(N_i / N_j)·Δφ_i
```

The radii are proportional to the tooth counts (r = mN/2), so the **tooth counts set the speeds**. Starting from G2 = −θ (clockwise):

| Gear | Speed × θ̇ | Sense (from front) | How |
|---|---|---|---|
| G2 | −1 | clockwise | root |
| G1 | −(36/12)·(−1) = **+3** | anticlockwise | from G2 |
| G3 | −(36/20)·(−1) = **+1.8** | anticlockwise | from G2 |
| G4 | −(20/20)·(+1.8) = **−1.8** | clockwise | from G3 |
| G5 | −(20/36)·(−1.8) = **+1** | anticlockwise | from G4 |

G5 turns exactly once per crank revolution, so the Geneva driver indexes once per part.

#### Tooth phase: derivation — [kinematics.h:52](../src/kinematics.h#L52)

The speeds alone are not enough: the teeth must **interleave**, not pass through each other. Tooth k of a gear sits at angle φ + 2πk/N, with tooth 0 along the gear's local +x axis. Let ψ be the direction from gear i's centre to gear j's centre.

- Measure i relative to the line of centres: its tooth points at j when φ_i − ψ = 0.
- Measure j relative to the direction back to i, ψ + π.
- **When i has a tooth pointing at j, j must have a gap pointing at i.** A gap is half a tooth spacing away from a tooth: π/N_j.
- Rolling couples the relative angles with the speed ratio.

Putting these together:

```
φ_j − (ψ + π + π/N_j) = −(N_i/N_j)·(φ_i − ψ)

φ_j = −(N_i/N_j)·(φ_i − ψ) + ψ + π + π/N_j
```

Evaluated in dependency order G2 (φ₂ = −θ), G1, G3, G4, G5. Check: if i advances one tooth (2π/N_i), then j moves by −2π/N_j, one tooth, and a gap is again opposite a tooth. A mesh that is right at one angle is right at every angle. Without the π/N_j term, every pair of teeth would sit tip-to-tip *through* each other forever.

#### Drawing a gear — [scene.h:312](../src/scene.h#L312)

```
glPushMatrix
  glTranslatef(cx, cy, −0.85)                 gear plane GEAR_Z
  glRotatef(φ in degrees, 0, 0, 1)
  glCallList(body)                            cylinder at root radius (28 slices) + hub
  for k = 0 … N−1:
    glPushMatrix
      glRotatef(360·k/N, 0, 0, 1)
      glTranslatef(root radius, 0, 0)
      glCallList(L_TOOTH)
    glPopMatrix
glPopMatrix
```

**The tooth** ([scene.h:247](../src/scene.h#L247)) is a box 2.25m = 0.1125 long radially, from the root r − 1.25m out to the tip r + m. It is **0.45 × πm = 0.0707** wide and 0.08 deep. The circular pitch (distance between teeth along the pitch circle) is πm. A width under half of it leaves room for rectangular teeth, which lack a real involute tooth shape, to pass without visibly crossing.

#### Why the gears seem to freeze at high speed (temporal aliasing)

All gears share the same pitch-line speed and module, so **every gear passes teeth at the same rate**:

```
N₂ · (crank revolutions per second) = 36 · ppm/60 = 0.6·ppm   teeth per second
```

A gear looks identical after turning one tooth. If exactly one tooth passes per displayed frame (F frames per second), every frame looks the same:

```
0.6·ppm = F   ⇒   ppm = F / 0.6            (100 ppm on a 60 Hz display)
```

All five gears freeze **at once** while the crank, ram and belt keep moving. The `h` readout prints this speed for the current frame rate. It is the sampling problem (aliasing) made visible.

### 8.5 Crankshaft, crank disc and crank pin — [scene.h:333](../src/scene.h#L333)

- **Model (silver):** shaft r 0.10 from the panel face (z −0.95) to z −0.11. Disc r 0.35, 0.06 thick. Pin r 0.07 at radius **r = 0.25** (the throw, `CRANK_R`).
- **Motion:** rotates with θ, clockwise. The pin is at the top when θ = 0:

```
pin = (x_P + r·sin θ,  Y_C + r·cos θ)          x_P = 2.115, Y_C = 4.75
```

- **Drawing:** `glTranslatef(2.115, 4.75, 0)`, then `glRotatef(90° − θ, 0, 0, 1)`, then the pin at local (r, 0). Check that the rotation puts the pin where the formula says:

```
local (r, 0) rotated by (90° − θ):  (r·cos(90° − θ),  r·sin(90° − θ)) = (r·sin θ,  r·cos θ)
```

### 8.6 Connecting rod and ram: the crank-slider

#### The press formula: derivation — [kinematics.h:73](../src/kinematics.h#L73)

The ram can only slide vertically on the line x = x_P. Its top (the wrist pin) is at W = (x_P, s). A rigid rod of length **L = 1.00** joins the crank pin P to W, so |P − W| = L:

```
(r·sin θ)² + (Y_C + r·cos θ − s)² = L²
Y_C + r·cos θ − s = ±√(L² − r²·sin²θ)
```

The ram hangs **below** the pin, so take the positive root:

```
s(θ) = Y_C + r·cos θ − √(L² − r²·sin²θ)
```

This is the standard crank-slider solution from mechanism kinematics, derived here from the rod constraint.

**Extremes.** At θ = 0: s = 4.75 + 0.25 − 1.00 = **4.00** (top dead centre). At θ = π: s = 4.75 − 0.25 − 1.00 = **3.50** (bottom dead centre). The **stroke is exactly 2r = 0.50**: sin θ = 0 at both ends, so the square root equals L at both and cancels out.

**Why the extremes are at 0 and π.** Differentiate:

```
ds/dθ = −r·sin θ · [ 1 − r·cos θ / √(L² − r²·sin²θ) ]
```

The bracket is always positive: `r²cos²θ < L² − r²sin²θ` simplifies to `r² < L²`, which is true. So s falls steadily from 0 to π and rises steadily back.

**Rod angle limit.** The rod's largest tilt from vertical is arcsin(r/L) = arcsin(0.25) = **14.48°**. The program checks L ≥ 2.5r at startup. A shorter rod would swing wildly and stop looking like a press.

#### The vertical stack at the press (how Y_C was derived)

Built from the bottom up, so the numbers close exactly:

| Level | y | From |
|---|---|---|
| belt top surface | 3.00 | roller axis 2.59 + roller r 0.39 + belt 0.02 |
| unstamped blank top | 3.20 | 3.00 + blank height 0.20 |
| stamped blank top = punch face at bottom | 3.10 | 3.00 + flattened height 0.10 |
| ram top at bottom, s_min | 3.50 | punch face 3.10 + ram height 0.40 |
| **crank axis Y_C** | **4.75** | s_min + r + L = 3.50 + 0.25 + 1.00 |
| ram top at top, s_max | 4.00 | Y_C + r − L |
| punch face at top | 3.60 | 4.00 − 0.40, so it clears an unstamped blank by 0.40 |
| G2 lowest tooth tip | 3.80 | Y_C − tip radius 0.95 |

#### Drawing the rod: a composite transformation — [scene.h:358](../src/scene.h#L358)

The rod is placed **from both of its endpoints**:

```
glTranslatef(pin.x, pin.y, 0)                                 move to the crank pin
glRotatef(atan2(s − pin.y, x_P − pin.x) in degrees, 0, 0, 1)  point at the wrist pin
glTranslatef(L/2, 0, 0)                                       box is centred, so shift half its length
box(L, 0.08, 0.08)
```

Its length is exactly L because that is what s(θ) solved for.

- **Wrist pin:** a small silver cylinder at (x_P, s).
- **Ram and punch:** a dark-steel box, 0.36 × 0.40 × 0.36, translated to (x_P, s − 0.20). This is a pure **translation** that follows s(θ).

### 8.7 The Geneva mechanism

A Geneva drive turns continuous rotation into **intermittent rotation with a positive lock**. The driver has a pin that enters one of the wheel's n slots, turns the wheel by 2π/n, then leaves. While the pin is out, the wheel cannot turn. This is a standard mechanism from the theory of machines, and its design equations are below.

#### Geometry: derivation — [kinematics.h:90](../src/kinematics.h#L90)

Chosen: **n = 4 slots**, driver pin radius **a = 0.55**.

For the pin to enter a slot **without impact**, it must be moving straight along the slot at the moment of entry. The pin moves tangentially, perpendicular to the driver's radius, so at entry the driver radius and the slot are **at right angles**. Triangle *driver centre O – pin P – wheel centre W* has a right angle at P.

The wheel turns 2π/n across the engagement, symmetric about the line of centres, so at entry the slot makes angle **π/n** with the line of centres at W. In the right triangle:

```
sin(π/n) = OP / OW = a / c     ⇒   c = a / sin(π/n) = 0.55 / sin 45° = 0.7778    (centre distance)
R = WP = √(c² − a²) = 0.55                                                 (wheel radius)
engagement: the driver is in the slot for |α| ≤ π/2 − π/n = 45°            (angle at O)
```

So per driver revolution the wheel turns **90°** during 90° of driver motion, and is **locked for the other 270°**.

#### Wheel angle while engaged: derivation — [kinematics.h:99](../src/kinematics.h#L99)

Put O at the origin with the line of centres along +x, so W = (c, 0). The driver angle from that line is α, so the pin is P = (a·cos α, a·sin α). The pin sits in the slot, so the slot points from W to P:

```
W→P = (a·cos α − c,  a·sin α)
```

Measure the wheel's angle β from the direction W→O. Dividing both components by c, with λ = a/c = sin(π/n) = 0.7071:

```
β(α) = atan2( λ·sin α,  1 − λ·cos α )          for |α| ≤ 45°
```

Check at the ends: at α = 45°, λ·sin α = 0.5 and 1 − λ·cos α = 0.5, so β = **45°**. Across −45°→+45° the wheel turns exactly **90°**. At α = 0 the pin is deepest, at c − a = 0.2278 from the wheel centre.

**It starts and stops smoothly.** Differentiating:

```
dβ/dα = λ(cos α − λ) / (1 − 2λ·cos α + λ²)
```

At α = ±45°, cos α = λ, so the numerator is **0**: the wheel, and with it the belt, starts and stops with zero speed and no jerk.

#### The driver's angle — [kinematics.h:439](../src/kinematics.h#L439), [scene.h:384](../src/scene.h#L384)

The driver arm is fixed to G5's shaft, so `arm angle − φ₅` is a constant. It is **computed once at startup** at θ = 0, so that the arm points along the line of centres (135°) when θ = 0:

```
arm_offset = 135° − φ₅(θ=0) = 46.119°
arm angle  = φ₅ + arm_offset = 135° + θ          (G5 turns at exactly +θ̇)
```

The arm's angle from the line of centres is therefore **α = θ, wrapped to (−180°, 180°]**, and the index is centred on the press's top dead centre.

- **Model (silver):** shaft on G5's axis from z −0.95 to 0.88. Arm box from −0.10 to a + 0.10, at z 0.80–0.88. Pin r 0.045 at radius a, z 0.66–0.80, reaching into the wheel's plane.
- **Drawing:** translate to G5's centre, rotate by the arm angle, draw the arm, translate to (a, 0), draw the pin.

#### Modelling the wheel — [scene.h:74](../src/scene.h#L74)

- **Hub:** a cylinder of radius 0.175, the slot bottom.
- **Arms:** four arms, each an `extrude_strip` between an inner and an outer boundary. The outer boundary is the rim R = 0.55. The inner boundary is:

```
r_in(u) = max( hub,  hw / sin u,  hw / cos u )      hw = slot half-width 0.07
```

**Why `hw / sin u`.** A slot wall is the straight line y = hw. In polar coordinates y = r·sin u, so r = hw / sin u. The slot on the other side of the arm is centred at 90°, and its wall x = hw gives r = hw / cos u. So one formula traces the hub arc in the middle of the arm and the two straight slot walls near its ends, and the four arms tile the disc with four clean slots.

- **Sampling:** the samples start where the wall meets the rim, u₀ = asin(hw/R) = 7.31°, and include the corner where the wall meets the hub, u_c = asin(hw/hub) = 23.58°, so the corners stay sharp.
- **Slot depth:** the pin's centre reaches 0.2278, and with its radius 0.045 it reaches 0.183. The slot is cut to 0.175, which leaves **0.008** clearance.
- **Placement:** the wheel sits at z 0.66–0.74 on the head roller's axis (4.00, 2.59), outside the conveyor frame rail, and is drawn with the roller's angle (§8.10).

### 8.8 Phases and the press interlock

#### The blank zone: derivation — [kinematics.h:110](../src/kinematics.h#L110)

The belt must **not** move while the punch is low enough to hit a blank. The punch face is s − 0.40. It is below an unstamped blank's top (3.20) when s < 3.60. Solve s(θ) = T for T = 3.60, writing c = cos θ and A = Y_C − T = 1.15:

```
Y_C + r·c − √(L² − r²(1 − c²)) = T
A + r·c = √(L² − r² + r²c²)
A² + 2Arc + r²c² = L² − r² + r²c²           square both sides; r²c² cancels
c = (L² − r² − A²) / (2·A·r)
  = (1 − 0.0625 − 1.3225) / (2 · 1.15 · 0.25)
  = −0.385 / 0.575 = −0.66957
θ = acos(−0.66957) = 132.03°,   and by symmetry 360° − 132.03° = 227.97°
```

So the punch is inside the blank zone for **θ ∈ (132.03°, 227.97°)**. The code computes this from the constants, so changing the throw, the rod, the ram or the belt height moves the window with them.

#### The four phases — [kinematics.h:123](../src/kinematics.h#L123)

| Phase | θ range | Geneva | Ram | Belt | Stack light |
|---|---|---|---|---|---|
| **INDEX** | 315°→45° (wraps through 0°) | pin in slot, wheel turns 90° | near the top | moves one pitch | amber |
| **APPROACH** | 45°→132.03° | locked | coming down | stopped | green |
| **STAMP** | 132.03°→227.97° | locked | below blank top, flattens it | stopped | red |
| **RETREAT** | 227.97°→315° | locked | going up | stopped | green |

INDEX is the Geneva engagement |α| ≤ 45°, centred on θ = 0 (top dead centre). The smallest punch clearance during INDEX is **0.34**, at its edges. At θ = 45°: s = 3.943, the punch face is 3.543, and 3.543 − 3.20 = 0.343. There is **87°** of margin before the blank zone on each side.

#### Live check — [main.cpp:97](../src/main.cpp#L97)

Every frame, if the punch face is below 3.20 **and** B changed since the last frame, a fault is counted and shown as "INTERLOCK FAULT" on the HUD. It should always read "interlock ok".

### 8.9 Belt travel B — [kinematics.h:142](../src/kinematics.h#L142), [kinematics.h:153](../src/kinematics.h#L153)

Belt travel is measured in **pitches** (stations): B = I + g.

```
g = (β(α) + 45°) / 90°     if |α| < 45°    (progress through the current index, 0→1)
g = 0                      otherwise

I = cycles − 1   if θ < 45°
I = cycles       otherwise                  (indices completed)
```

**Why I is not simply `cycles`.** INDEX runs from 315° to 45°, straight through θ = 0 where `cycles` goes up. With I = cycles, B would jump a whole pitch at 0°, halfway through the belt's visible movement. Counting an index as complete at its **end** (45°) makes B continuous and always increasing. Check at θ just below 45°: I = cycles − 1 and g → 1, so B → cycles. Just above: I = cycles and g = 0, so B = cycles.

**Wheel and roller angle** ([kinematics.h:161](../src/kinematics.h#L161)):

```
wheel angle = −(360° / 4)·B = −90°·B        negative = clockwise
```

### 8.10 Rollers and belt

#### The pitch: derivation — [kinematics.h:167](../src/kinematics.h#L167)

One station is defined as **one quarter turn** of the head roller. The belt's centreline wraps the roller at radius R_c = roller 0.39 + half the belt thickness 0.01 = **0.40**. The arc length moved in a quarter turn is:

```
p = R_c · π/2 = 0.62832        pitch = distance between stations
```

#### Roller spacing and loop length

The belt loop's length is `2D + 2πR_c`, where D is the distance between the roller axes. Because p = R_c·π/2, the two half-circles together are always `2πR_c = 4p`, whatever the roller size. Choosing **D = 10p** makes:

```
loop = 20p + 4p = 24p = 15.080          ⇒ exactly 24 cleats, one per pitch
tail roller x = 4.000 − 10p = −2.2832   (head roller at x = 4.000)
```

Every piece of the belt path is then a whole number of pitches, so the cleats line up where the loop closes.

#### Rollers — list `L_ROLLER` ([scene.h:287](../src/scene.h#L287)), drawn in [scene.h:404](../src/scene.h#L404)

- **Model:** a silver cylinder, r 0.39, length 1.04, with a small key bar across its front end cap. On a rotating cylinder of one colour you cannot see rotation, so the key and the wall's outline lines make it visible.
- **Motion:** both rollers rotate by −90°·B. The tail roller has the same radius and is driven by the same belt, so it turns by the same angle. A stub shaft joins the head roller to the Geneva wheel and turns with them.

#### Belt — list `L_BELT` ([scene.h:192](../src/scene.h#L192))

- **Model:** a thin top strip (y 2.98–3.00), a bottom strip, and two half-shells round the rollers (`extrude_strip` between radii 0.39 and 0.41, head −90°→90°, tail 90°→270°).
- **Motion:** the strips are **static**. A plain strip that moves looks identical to one that stands still, so all the belt's visible motion is carried by the cleats.

### 8.11 Cleats — list `L_CLEAT`, drawn in [scene.h:404](../src/scene.h#L404)

- **Model:** a yellow box, 0.03 × 0.03 × 0.96, raised by half its height so its base sits on the belt.
- **Position along the loop:** cleat k (k = 0…23) is at arc length

```
s_k = ((B + k + ½) · p)  mod 24p
```

  The **+½** puts cleats halfway between stations, so a blank never overlaps one. Check with the widest (flattened) blank: its radius is 0.2546, and the half-gap between cleats is (p − 0.03)/2 = 0.2992, which leaves 0.045 clear.

- **Path function** ([kinematics.h:178](../src/kinematics.h#L178)): arc length s is measured from the top of the tail roller, moving +x. Positions are on the outer surface, radius R_o = 0.41.

| s | Piece | Position | Cleat rotation |
|---|---|---|---|
| 0 → 10p | top run | (x_T + s, 3.00) | 0° |
| 10p → 12p | round the head roller | φ = 90° − (s − 10p)/R_c; (4.00 + R_o·cos φ, 2.59 + R_o·sin φ) | φ − 90° |
| 12p → 22p | bottom run | (4.00 − (s − 12p), 2.18) | 180° |
| 22p → 24p | round the tail roller | φ = 270° − (s − 22p)/R_c; (x_T + R_o·cos φ, 2.59 + R_o·sin φ) | φ − 90° |

  **How the arc formula works.** Arc length along a circle is radius × angle, so the angle swept is (s − 10p)/R_c. Over 2p that is 2p/R_c = π, a half circle from 90° down to −90°, clockwise. The cleat is rotated by φ − 90° so that its local "up" (+y, at 90°) turns to point along the outward normal at angle φ.

- **Drawing:** `glTranslatef(path point)`, then `glRotatef(rotation)`, then the cleat list. A composite of translation and rotation.

### 8.12 Blanks on the belt — [scene.h:462](../src/scene.h#L462)

- **Model:** a silver cylinder, r 0.18, height 0.20, 14 slices, with a 0.03 chamfer (list `L_BLANK`).
- **Stations:** station j is at x_j = x_T + j·p. Station 1 (x −1.655) is under the magazine, **station 7 (x 2.115) is under the press**, and station 10 (x 4.000) is on top of the head roller.
- **Motion:** blanks carry labels j = 1…9, and blank j is drawn at

```
x = x_T + (j + g)·p,   y = 3.00
```

  **The relabelling trick.** A blank's position uses only g, not I. During an index every blank slides forward one pitch as g goes 0→1. When the index ends, g snaps back to 0, and label j now means the blank that was at label j−1. Visually nothing jumps: the blank at station j+1 is exactly where label j+1 is drawn from then on. No array of blank positions is ever stored.

- **The press is at station 7** because it is the furthest upstream station the two-idler gear chain can reach (station 6 would need G2 4.09 from G5, beyond the 3.8 reach). That leaves two stations after the press where finished parts are on show.

### 8.13 A fresh blank from the magazine — [kinematics.h:235](../src/kinematics.h#L235)

During INDEX, a new blank appears inside the magazine tube once the departing blank has cleared station 1:

```
visible when g ≥ 0.605     because 0.605 · p = 0.380 = 2 × 0.18 (two radii) + 0.02 gap
y = 3.24 − 0.24 · clamp((g − 0.605)/0.10, 0, 1)
```

It starts at 3.24, the tube's lower edge (hidden inside the tube), and drops to the belt (3.00) over the next tenth of the index, a **translation**. At g = 1 it is exactly where label 1 is drawn after the index, so the handover never shows.

### 8.14 Stamping: the blank squash — [kinematics.h:216](../src/kinematics.h#L216), [scene.h:442](../src/scene.h#L442)

#### Height of the blank under the press

Only label 7 changes height:

```
h₇(θ) = clamp( punch_face(θ) − 3.00,  0.10,  0.20 )     for 45° ≤ θ < 180°
h₇(θ) = 0.10                                            otherwise
labels < 7: 0.20 (not yet stamped)       labels > 7: 0.10 (stamped)
```

- **Before 45°** label 7 is the finished part leaving the press, which is flat.
- **From 45°** it is the new arrival. Its height stays 0.20 until the punch face reaches it at 132.03°, then follows the punch face exactly down to 0.10 at 180°.
- **After 180°** it stays flat.

Every boundary is continuous. For example, h₇(135°) = 0.189.

#### Volume-preserving squash: derivation

A real pressed part gets wider as it gets thinner, because metal keeps its volume. A cylinder's volume is V = π·r²·h. Scale the height by q = h/0.20 and both horizontal axes by k:

```
V' = π·(k·r)²·(q·h) = V   ⇒   k²·q = 1   ⇒   k = 1/√q

glScalef(1/√q,  q,  1/√q)
```

At full squash q = 0.5, so k = 1.414 and the radius becomes 0.18 × 1.414 = **0.2546**. Check: 0.18² × 0.20 = 0.2546² × 0.10 = 0.00648.

#### Scaling about the base: a composite transformation

```
glTranslatef(x, 3.00, z)      move to the belt surface first
glScalef(1/√q, q, 1/√q)       then scale
glCallList(L_BLANK)           the cylinder rises from y = 0, so its base stays on the belt
```

`glScalef` scales about the current origin. Because the cylinder's base is at the origin, the base stays put and only the top comes down. Scaling about the centre would lift the flattened part off the belt. **This is the only `glScalef` in the whole scene.**

### 8.15 Finished parts leave the line — [kinematics.h:241](../src/kinematics.h#L241)–[kinematics.h:384](../src/kinematics.h#L384)

A flattened part rides to station 10, over the head roller, is tossed onto the chute, slides down and drops into the bin. The whole path is still a closed-form function of θ and cycles.

#### The stroke clock — [kinematics.h:251](../src/kinematics.h#L251)

```
n = I                              (indices completed, as in §8.9)
f = ((θ° − 45°) mod 360°) / 360°    fraction of a revolution since the last index ended
clock = n + f                       continuous, +1 per part, whole at θ = 45°
```

INDEX fills the last 90° of each clock cycle, so it starts at **f = 1 − 45°/180° = 0.75**.

Part e reaches station 10 when the clock reads e. Its **local time** is t = (n − e) + f:

| t | What the part does |
|---|---|
| 0 → 0.75 | rests on top of the head roller (belt locked) |
| 0.75 → 1 | the next INDEX: rides round the roller to 40°, then is tossed onto the chute |
| 1 → 1.30 | slides down the chute (`EXIT_SLIDE` = 0.30) |
| 1.30 → 1.52 | drops off the lip into its bin slot (`EXIT_DROP` = 0.22) |

#### On the belt

The part is w pitches past station 10, at `belt_path((10 + w)·p)`. It is pushed outwards by half its thickness (0.05) along the belt's normal, and tilted with the belt. **A quarter turn of roller is exactly one pitch**, so a wrap of 40° is w = 40/90 = 0.444 pitch.

#### The toss: a quadratic Bézier curve

The toss runs once g passes 0.444. With s = (g − 0.444)/(1 − 0.444) and u = 1 − s:

```
P0 = the part where it leaves the roller           P1 = its landing spot on the chute
C  = P0 + 0.35·(cos a, sin a)                       a = belt direction at P0 (the tangent)
P(s) = u²·P0 + 2·s·u·C + s²·P1
z(s) = 0.22 · (3s² − 2s³)                           eases out sideways onto the chute
tilt(s) = tilt₀ + (−45° − tilt₀)·s
```

**Why the curve joins smoothly.** A quadratic Bézier's derivative at s = 0 is 2(C − P0). C lies along the belt's tangent, so the part leaves in the direction it was already moving. The toss is keyed to g, so it lands as the belt slows to a stop.

#### On the chute — [kinematics.h:276](../src/kinematics.h#L276)

The chute runs from (4.55, 2.35) to (5.45, 1.45): length √(0.9² + 0.9²) = **1.2728**, at −45°. A part fraction u of the way down is:

```
position = start + u·(end − start) + 0.08·n̂      n̂ = (−dy, dx)/length, the chute's upward normal
                                                 0.08 = half the chute thickness + half the part thickness
tilt = atan2(−n̂x, n̂y) = −45°
```

The part is on the chute from u = flat_r / length = 0.2546/1.2728 = **0.2**, when its upper rim clears the top end, to **u = 0.8**, when its lower rim reaches the bottom end.

#### The slide: constant acceleration from rest

```
u = 0.2 + 0.6 · (t₁ / 0.30)²          t₁ = t − 1
```

Distance growing with the square of time is the form of uniform acceleration from rest (x = ½at²), like something sliding down a ramp.

#### The drop: matching the slide's speed

```
P(s) = P0 + V·s + (P1 − P0 − V)·s²          s = (t₁ − 0.30)/0.22
```

**Derivation.** A quadratic with P(0) = P0, P(1) = P1 and starting velocity P'(0) = V has exactly this form. V must equal the slide's final velocity, so there is no kink. From the slide formula, with chute vector d = end − start:

```
dP/dt₁ = 2 · 0.6 · t₁ / 0.30² · d           at t₁ = 0.30:  2 · 0.6 / 0.30 · d
dt₁/ds = 0.22
V = dP/ds = (2 · 0.6 · 0.22 / 0.30) · d      (the code's k = 2·run·DROP/SLIDE)
```

The z coordinate uses `z0 + (z1 − z0)·s²`, which starts with no sideways speed, like the slide. The tilt goes from −45° to 0°, so the part lands flat.

#### Bin slots — [kinematics.h:292](../src/kinematics.h#L292)

Parts pile in a 2 × 2 grid of columns, 9 levels, **36 parts**. Slot i:

```
c = i mod 4,    level = i div 4
x = 5.6 + (c mod 2 − 0.5)·0.54 + 0.01·cos(i·137.5°)
z =       (c div 2 − 0.5)·0.54 + 0.01·sin(i·137.5°)
y = 0.06 + (level + 0.5)·0.10                   bin floor + half a part + whole parts below
```

137.5° (2.39996 rad = π(3 − √5)) is the **golden angle**. Stepping by it never repeats a pattern, so the tiny nudges make the columns look hand-stacked rather than ruled.

**Parts in the bin** ([kinematics.h:306](../src/kinematics.h#L306)): part e has landed when its local time t = n − e + f ≥ 1.52, that is e ≤ n + f − 1.52. So:

```
bin count = max(0,  n + floor(f − 1.52))
```

Once the bin is full, each new part aims at the top slot, which is already drawn, so it lands on the pile without a second copy appearing.

### 8.16 Magazine, exit hood, chute and bin — list `L_FIXTURES` ([scene.h:206](../src/scene.h#L206))

- **Feed magazine (black):** a square tube of 4 boxes over station 1, 0.50 wide, y 3.24→4.60. Its lower edge is above an unstamped blank's top (3.20), so fresh blanks appear hidden inside it.
- **Exit hood (black):** a top and two sides over x 3.67→4.45, open at the far end so finished parts can leave over the roller.
- **Chute (green):** one box, translated to the chute's midpoint and rotated by atan2(−0.9, 0.9) = −45°, length 1.2728.
- **Bin (green):** 5 boxes (floor and 4 walls), x 5.0→6.2, 1.0 tall.
- **Motion:** none. The parts moving through them are described in §8.13 and §8.15.

### 8.17 Stack light — [scene.h:481](../src/scene.h#L481)

- **Model:** a green post (r 0.05, 5.5 tall) at (5.5, −0.8), a black housing, then three lens cylinders (r 0.10, 0.25 tall each, list `L_STACK_SEG`): green at the bottom, amber, red at the top.
- **Motion (colour):** the lit lens follows the phase (§8.8): **red** during STAMP, **amber** during INDEX, **green** during APPROACH and RETREAT. The lit lens gets its full colour and the others `mat::lens` dim colour. Colours are set outside the list every frame.

### 8.18 Parts counter — [room.h:585](../src/room.h#L585)

- **Model:** a black housing over the drive panel (on the back wall) with four seven-segment digits. Each segment is a small box.
- **Count** ([kinematics.h:136](../src/kinematics.h#L136)):

```
parts made = cycles + (1 if θ ≥ 180° else 0)       a part counts at bottom dead centre, when it is flattened
shown value = parts mod 10000,   digit k = (value / 10^(3−k)) mod 10
```

- **Segments:** each digit is a 7-bit mask (bits a…g). For example, 0 = `0x3F` = segments a–f. Segment s is lit if `(mask >> s) & 1`. Lit segments are drawn bright orange and unlit ones dim, in two passes, with two colour changes per frame. The HUD reads the same function, so the two can never disagree.

---

## 9. The room, object by object

The room is 15 × 8 × 9.5 units: x −6→9, z −2.5→5.5, ceiling at y = 9.5. It lives in [room.h](../src/room.h). The machine does not know it exists.

### 9.1 Wall frames — [room.h:52](../src/room.h#L52)

Each wall is built in its own local frame: **u** runs along the wall (left to right seen from inside), **y** is up, and **+z points into the room**. `enter_wall(w)` sets up that frame. A rotation of angle a about y sends local +x to (cos a, 0, −sin a) and local +z to (sin a, 0, cos a):

| Wall | Translate to | Rotate about y | local +x (u) → | local +z (inward) → |
|---|---|---|---|---|
| back (z = −2.5) | (−6, 0, −2.5) | 0° | +x | +z |
| left (x = −6) | (−6, 0, 5.5) | +90° | −z | +x |
| right (x = 9) | (9, 0, −2.5) | −90° | +z | −x |
| front (z = 5.5) | (9, 0, 5.5) | 180° | −x | −z |

So a routine written once, like `window(u)`, puts a window on any wall. `back_u(x) = x + 6` and similar helpers convert world positions into u.

### 9.2 Wall surface — [room.h:90](../src/room.h#L90)

- **Concrete plinth**, 1.20 tall: a `grid` 3 cells high.
- **Painted wall** above it, up to the ceiling, in 1.0 cells, which read as wall panels.
- **Yellow trim line**, a box 0.06 tall, where they meet.
- **Motion:** none. Each wall is one display list, drawn only when the cutaway rule allows (§7.5).

### 9.3 Windows — [room.h:103](../src/room.h#L103)

- **Model:** a pale-blue pane (a 1 × 1 grid 0.01 off the wall) in a galvanized frame: sill, head, two jambs, a vertical bar and two half horizontal bars. The horizontal bar is split so no two boxes share a face.
- **Placement:** two on the back wall, two on the left, two on the right, three on the front (`BWIN_X`, `LWIN_Z`, `RWIN_Z`, `FWIN_X`).
- **Motion:** none.

### 9.4 Back wall — [room.h:140](../src/room.h#L140)

- **Exhaust fan:**
  - **Model:** the housing is a black backing, a motor cylinder and a four-box frame. The rotor ([room.h:516](../src/room.h#L516)) has a silver hub and **6 blades**. Blade k is rotated by 360°·k/6, moved 0.24 out and pitched 25° about its length.
  - **Motion:** the rotor turns by −fan_deg about the wall's normal (§4.4). Switch `f` stops or starts it with the first-order lag.
- **Parts counter housing:** see §8.18.
- **Pipe run:** two pipes (r 0.09) at y 8.30 and 8.65 along the whole wall. `cyl_u` rotates −90° about z, which maps the cylinder's +y onto the wall's +u. Hangers every 2.5 units.
- **Riser:** a vertical pipe at the right end, with a flange box and a red valve wheel.
- **Conduit and breaker box:** a thin pipe from the cabinet top up to the pipe run, and a box with a lever.
- **Motion:** none, apart from the fan rotor.

### 9.5 Left wall — [room.h:209](../src/room.h#L209)

- **Roll-up door:**
  - **Model:** 3.0 wide, 3.7 tall. A dark backing, then **18 slats**, each (3.70 − 0.10)/18 = 0.20 tall with 0.012 gaps. Two side guides, a coil box on top, a yellow bottom bar and a handle.
  - **Motion:** none.
- **Fire extinguisher:** a red chamfered cylinder on a bracket, a black valve cap, and a red sign box above.
- **Tool board:** a wooden board with a spanner (5 boxes), a hammer (handle and head boxes), and two screwdrivers (a thin silver cylinder plus a red handle each).
- **Shelf of spare gears:** a wooden shelf on two brackets holding a G3-size gear and a G1-size pinion. They reuse the train's own body and tooth lists, so they are exactly the parts they would replace.
  - **Standing on a tooth tip:** each is translated up by its tip radius, so it stands on its tooth at 270°. Tooth k is at 360°·k/N, and 270° needs 270N/360 = 0.75N to be a whole number. That holds for N = 12 (tooth 9) and N = 20 (tooth 15), which is why these two sizes were used.
- **Motion:** none.

### 9.6 Ceiling and beams — [room.h:296](../src/room.h#L296)

- **Model:** the ceiling is a `grid` facing down at y = 9.5. **5 I-beams** run front to back at x = −3.90 + 3.0·k. Each is 3 boxes: bottom flange (0.30 wide), web (0.05 wide) and top flange, 0.45 deep.
- **Visibility:** the ceiling is not drawn when the eye is above it, as in the top view.
- **Motion:** none.

### 9.7 The two hanging bulbs

- **Pendants** ([room.h:314](../src/room.h#L314)): for each bulb at x −0.57 and 4.80 (z 1.0), a ceiling rose (cylinder), a flex (thin black cylinder from the rose down to the holder), a lampholder and a collar.
- **Glass globe** ([room.h:340](../src/room.h#L340)): `lathe` of a profile that follows a circle of radius 0.12 from −90° to 55°, then narrows into a neck: (0.042, 0.135), (0.040, 0.170), (0, 0.170).
- **Motion (colour):** the switches `[` and `]` choose the globe's colour (warm white when on, grey when off) and draw the halo (§6.4). There is no lighting in this build, so a bulb lights nothing.
- **Code:** globes and colours are drawn per frame in [room.h:643](../src/room.h#L643).

### 9.8 Floor items — list `R_FLOOR_ITEMS`

- **Hazard markings** ([room.h:364](../src/room.h#L364)): a dashed yellow and black border round the machine, plus a strip at the door. Each strip of length ℓ is split into dashes:

```
n = max(1, round(ℓ / 0.40)),   dash length d = ℓ / n
even dashes yellow, odd dashes black, each 0.004 above the floor
```

  They sit 0.004 above the floor, which is thousands of depth-buffer steps at this near plane, so they never flicker against it. The back and front strips run the full width and the sides stop short, so no two overlap.

- **Pallet of blanks** ([room.h:390](../src/room.h#L390)): 3 bearers and 5 deck boards (wood), then two layers of 3 × 3 blanks at 0.38 spacing. The blanks reuse `L_BLANK`, and there is a slip sheet between layers. The top layer has 2 blanks missing (as if some were used).
- **Workbench** ([room.h:422](../src/room.h#L422)):
  - A wooden top, a lower shelf and 4 galvanized legs.
  - A green vice with a silver screw and bar, and a red toolbox with a black handle.
  - **A blank and a stamped part side by side.** The stamped one is built flat at radius 0.18·√(0.20/0.10) = 0.2546 rather than scaled, so the belt's squash stays the only `glScalef`.
- **Electrical cabinet** ([room.h:473](../src/room.h#L473)): a black plinth, a green body, a door seam, 3 louvres, 2 handles, and 4 switch plates.
- **Switches** ([room.h:619](../src/room.h#L619)):
  - **Model:** four rockers (machine, left bulb, right bulb, fan), each with a lamp above.
  - **Motion:** a rocker is rotated **−16° about x when on, +16° when off**, so its top leans in or out. Its lamp is lit in the switch's colour, the same colour as the dot in the HUD.
- **Drums** ([room.h:494](../src/room.h#L494)): one red and one galvanized, each a chamfered cylinder with two slightly wider hoop rings and a black bung on top.

---

## 10. Transformations used

| Type | Where it does real work |
|---|---|
| **Translation** | ram following s(θ); blanks along the belt; cleats on the straight runs; the fresh blank dropping; parts on the chute and dropping into the bin; placing every object |
| **Rotation** | five gears; crankshaft and disc; rod swing; Geneva driver and wheel; both rollers; cleats round the roller arcs; tilting parts; fan rotor; switch rockers; walls into their frames; camera orbit |
| **Scaling** | the volume-preserving blank squash, the only `glScalef` |
| **Composite** | the crank-slider (rotation becomes translation); the rod placed from both endpoints (translate → rotate → translate); cleats (translate along the path → rotate to the normal); scale about the blank's base (translate → scale); each gear tooth (rotate → translate); wall frames (translate → rotate) |
| **Hierarchical** | the chains below, built with `glPushMatrix`/`glPopMatrix` |

---

## 11. Hierarchy and the two closed loops

```
A: floor → drive panel → crankshaft → crank disc → crank pin → connecting rod → ram → punch
B: floor → drive panel → guide rails → ram
C: floor → drive panel → G5 shaft → driver arm → pin
D: floor → conveyor frame → head roller → Geneva wheel
            ├─ belt path → cleat k
            └─ station j → blank → squash
```

- **Two closed loops.** Chains A and B meet at the ram, and C and D meet where the pin sits in the slot. The scene is therefore two closed kinematic loops, not a simple tree. Both are closed **by formula**, A+B by s(θ) and C+D by β(α), so no iterative solving is needed.
- **Meshing is not parenting.** Making G3 a child of G2 looks natural but is wrong. A child inherits its parent's rotation, while a meshing gear turns the *other* way at a *different* rate, so G3 would first have to undo G2's rotation. Instead every gear is a child of the drive panel, and the mesh law (§8.4) is a constraint between siblings.

---

## 12. Controls

| Key | Action |
|---|---|
| `Space` | machine on / off |
| `.` | machine off: step θ forward 5° |
| `↑` `↓` (or `+` `−`) | speed ±6 ppm, range 6–120 |
| `←` `→` | orbit the camera |
| `1` `2` `3` `4` | three-quarter / front / top / whole room |
| `c` | free camera on / off |
| free camera: `↑` `↓` `←` `→`, `PgUp` `PgDn`, left-drag | fly forward/back, strafe, rise/sink, look |
| `[` `]` | left / right bulb on / off |
| `f` | exhaust fan on / off |
| `e` | edge lines on / off |
| `w` | wireframe |
| `h` | technical readout |
| `r` | reset (θ = 90°, cycles = 0) |
| `Esc` | quit |

---

## 13. Verification

1. **Startup checks** ([main.cpp:597](../src/main.cpp#L597)), run in every build and printed to the console:
   - L ≥ 2.5r
   - s_max = 4.00 and s_min = 3.50
   - tail roller at −2.2832
   - Geneva c = 0.7778 and R = 0.55
   - every meshing pair exactly r_i + r_j apart

   The console also prints the blank zone (132.03°–227.97°), the index window, the pitch and the loop length.
2. **Live interlock check** (§8.8): shown on the HUD every frame.
3. **Headless test** `.\build.ps1 -Check` ([tests/mathcheck.cpp](../tests/mathcheck.cpp)). It includes the *same* [kinematics.h](../src/kinematics.h) the renderer uses and checks:
   - belt pitch and loop closure
   - the press stack
   - the analytic blank-zone angle against a 0.01° numerical scan
   - the Geneva geometry, and that the pin stays in its slot through the index
   - that belt travel never goes backwards, across 45° and the wrap
   - all gear meshes and tooth-to-gap alignment at every angle
   - volume preservation under the squash
   - the clearances
   - that exiting parts move continuously, land on their slots, stay inside the bin and fit the pile

---

## 14. Motion cheat sheet

| Object | Moves how | Driven by | Formula | Code |
|---|---|---|---|---|
| G2 crank gear | rotates clockwise | θ | φ₂ = −θ | [kinematics.h:52](../src/kinematics.h#L52) |
| G1 pinion (motor) | rotates, 3× | θ | mesh law, rate +3 | same |
| G3, G4 idlers | rotate, 1.8× | θ | mesh law, rates +1.8, −1.8 | same |
| G5 | rotates, 1× | θ | mesh law, rate +1 | same |
| crank disc and pin | rotates | θ | rotate(90° − θ) | [scene.h:333](../src/scene.h#L333) |
| connecting rod | swings and moves | θ | ends at the pin and at (x_P, s) | [scene.h:358](../src/scene.h#L358) |
| ram and punch | slides up and down | θ | s(θ) = Y_C + r cos θ − √(L² − r² sin² θ) | [kinematics.h:73](../src/kinematics.h#L73) |
| Geneva driver arm | rotates | θ | 135° + θ | [kinematics.h:439](../src/kinematics.h#L439) |
| Geneva wheel | quarter turn, then locked | θ, cycles | −90°·B, β = atan2(λ sin α, 1 − λ cos α) | [kinematics.h:99](../src/kinematics.h#L99) |
| head and tail rollers | quarter turn, then locked | B | −90°·B | [scene.h:404](../src/scene.h#L404) |
| cleats | travel round the loop | B | s_k = (B + k + ½)p mod 24p | [kinematics.h:178](../src/kinematics.h#L178) |
| blanks | step one station per index | θ | x = x_T + (j + g)p | [scene.h:462](../src/scene.h#L462) |
| fresh blank | drops out of the magazine | θ | y = 3.24 − 0.24·clamp(…) | [kinematics.h:236](../src/kinematics.h#L236) |
| blank at press | flattens and widens | θ | h₇, glScalef(1/√q, q, 1/√q) | [kinematics.h:216](../src/kinematics.h#L216) |
| finished part | over roller → toss → slide → drop | θ, cycles | stroke clock, Bézier, u², quadratic | [kinematics.h:324](../src/kinematics.h#L324) |
| bin pile | grows | θ, cycles | n + floor(f − 1.52), golden-angle slots | [kinematics.h:292](../src/kinematics.h#L292) |
| stack light | changes colour | θ | phase ranges | [scene.h:481](../src/scene.h#L481) |
| parts counter | counts | θ, cycles | cycles + [θ ≥ 180°] | [room.h:585](../src/room.h#L585) |
| exhaust fan | spins up and down | own state | first-order lag | [main.cpp:81](../src/main.cpp#L81) |
| switch rockers, lamps | tilt, light | key presses | ±16° about x | [room.h:619](../src/room.h#L619) |
| bulbs | glow on or off | key presses | colour + halo | [room.h:643](../src/room.h#L643) |
| camera | orbit, fly, look | key presses, dt | §7.3, §7.4 | [main.cpp:115](../src/main.cpp#L115) |

---

## 15. Where the numbers came from

Every number is in one of three groups.

**A. Standard formulas from geometry and mechanism theory** (textbook kinematics, derived in this document):

| Formula | Source |
|---|---|
| pitch radius r = mN/2, centre distance r_i + r_j, speed ratio −N_i/N_j | standard spur-gear relations; rolling without slipping |
| tooth phase φ_j = −(N_i/N_j)(φ_i − ψ) + ψ + π + π/N_j | derived from rolling + "gap opposite tooth" (§8.4) |
| crank-slider s(θ), stroke 2r, obliquity arcsin(r/L) | standard slider-crank kinematics; rod-length constraint (§8.6) |
| Geneva c = a/sin(π/n), R = √(c² − a²), β(α), dβ/dα | standard Geneva drive design; right triangle at entry (§8.7) |
| volume-preserving scale 1/√q | cylinder volume πr²h (§8.14) |
| quadratic Bézier, constant-acceleration slide, quadratic drop | standard curve and motion forms (§8.15) |
| exact first-order lag e^(−dt/τ) | solution of dω/dt = (ω* − ω)/τ (§4.4) |
| orbit rotation, yaw/pitch forward vector, billboard axes | rotation matrices (§6.4, §7) |
| golden angle π(3 − √5) | standard even-spread angle (§8.15) |

**B. Derived from other numbers** (computed in code, not typed in):

| Value | From |
|---|---|
| crank axis Y_C = 4.75 | press stack: s_min + r + L (§8.6) |
| G3, G4 centres, 19.26° bend | symmetric chain between G2 and G5 (§8.4) |
| blank zone 132.03° / 227.97° | solving s(θ) − 0.40 = 3.20 (§8.8) |
| Geneva c = 0.7778, R = 0.55, engagement 45° | n = 4, a = 0.55 (§8.7) |
| driver arm offset 46.119° | arm along the line of centres at θ = 0 (§8.7) |
| pitch p = 0.62832, tail roller x = −2.2832, loop 24p | quarter turn of R_c = 0.40, D = 10p (§8.10) |
| fresh-blank threshold 0.605 | two radii + 0.02 gap, divided by p (§8.13) |
| chute landing fractions 0.2 / 0.8, index start 0.75 | part radius / chute length; 45°/180° (§8.15) |
| belt top 3.00, bottom run 2.18 | roller axis ± radii (§8.6, §8.11) |
| freeze speed F/0.6 | tooth-passing rate 0.6·ppm (§8.4) |

**C. Chosen design values** (picked for size, visibility and look, then tuned on screen):

| Value | Why |
|---|---|
| module 0.05, tooth counts 12/36/20/20/36 | 36/12 gives a 3× motor; 36/36 gives the Geneva gear one turn per part |
| throw r = 0.25, rod L = 1.00 | stroke 0.50 flattens a 0.20 blank to 0.10; L/r = 4 keeps the rod nearly vertical |
| 4 Geneva slots, pin radius 0.55 | a quarter turn per part; wheel sized to the roller |
| roller radius 0.39, belt 0.02 thick | belt top at a 75 cm working height (3.00) |
| blank r 0.18, h 0.20, flattened 0.10 | a halved height, still clear of the cleats (0.045 margin) |
| tooth width 0.45 of the circular pitch | rectangular teeth pass without visibly crossing |
| speeds 6–120 ppm, default 30 | slow enough to follow the pin; 100 ppm shows the freeze at 60 fps |
| camera positions, near 4 / far 40 | the whole machine in view; near and far set from measured distances (§7.1) |
| colours, edge darkness 0.40, polygon offset (1, 1) | readable flat-colour look |
| room sizes and furniture positions | composition around the machine |
| exit timings 0.35 toss, 0.30 slide, 0.22 drop, tip at 40° | chosen so the parts land in time with the index |
