# Quarter Turn — how the project was made, start to finish

**Course:** CSE 4207 Computer Graphics, KUET — Prof. Dr. Sk. Md. Masudul Ahsan
**Student:** Md. Sifatul Islam
**Project:** an automated stamping line in OpenGL (C++17, fixed-function OpenGL plus one GLSL 1.10 program, freeglut, GLEW)

This document walks through the project step by step: how the scene was
chosen, how the layout was plotted, how every motion was derived as a formula,
how the code was organised and written, and how it was checked.

---

## Contents

1. [Step 1 — reading the brief](#step-1--reading-the-brief)
2. [Step 2 — choosing the scene](#step-2--choosing-the-scene)
3. [Step 3 — the key design decision: one angle drives everything](#step-3--the-key-design-decision-one-angle-drives-everything)
4. [Step 4 — plotting the layout](#step-4--plotting-the-layout)
5. [Step 5 — deriving the mechanisms](#step-5--deriving-the-mechanisms)
6. [Step 6 — setting up the build](#step-6--setting-up-the-build)
7. [Step 7 — coding the primitives](#step-7--coding-the-primitives)
8. [Step 8 — coding the machine, part by part](#step-8--coding-the-machine-part-by-part)
9. [Step 9 — the render loop and the clock](#step-9--the-render-loop-and-the-clock)
10. [Step 10 — lighting](#step-10--lighting)
11. [Step 11 — the room, the camera and the switches](#step-11--the-room-the-camera-and-the-switches)
12. [Step 12 — checking it](#step-12--checking-it)
13. [Step 13 — documentation](#step-13--documentation)
14. [What I learned](#what-i-learned)

---

## Step 1 — reading the brief

The brief asked for a simple 3D industrial factory scene in OpenGL — gears
moving one another and a conveyor belt — that:

- runs continuously in the render loop, with visible motion;
- uses the model transformations taught — translation, rotation, scaling,
  composite and hierarchical — each doing real work;
- is lit;
- has **no pre-computed animation**;
- is optimised, and interactive or clearly explained on screen.

I turned that list into a checklist and kept it next to me for the whole
project, so every feature I added could be matched to a line of the brief.

## Step 2 — choosing the scene

I wanted a factory where every moving part has a *reason* to move, so a viewer
can follow power from the motor to the finished product. I chose a **stamping
line**:

```
motor ─► 5-gear train ─┬─► crank ─► connecting rod ─► press ram   (stamps the part)
                       └─► Geneva drive ─► conveyor head roller   (moves the part)
```

- A **motor** turns a train of **five meshing gears**.
- The big gear carries a **crank**; a **crank-slider** turns its rotation into
  the up-and-down stroke of a **press ram**.
- The last gear turns a **Geneva mechanism**, which turns continuous rotation
  into *exactly a quarter turn, then a locked pause*. That quarter turn moves
  the **conveyor belt** one station.
- Round steel **blanks** ride the belt, stop under the press, are flattened
  while the belt is locked, and leave at the head roller.

The name *Quarter Turn* comes from that unit: one quarter turn of the Geneva
wheel = one station of belt = one press stroke = one finished part.

How the scene covers the brief:

| Brief item | Where it is in the scene |
|---|---|
| Gears moving one another | five meshing gears, G1–G5 |
| Conveyor belt | belt, rollers and 24 cleats, indexed by the Geneva drive |
| Translation | ram going up and down, blanks and cleats moving along the belt |
| Rotation | gears, crank, Geneva driver and wheel, rollers, fan |
| Scaling | the blank being flattened by the press |
| Composite | crank-slider, placing the rod between two moving points, cleats following the belt path, scaling about the blank's base |
| Hierarchical | panel → crank → pin → rod → ram, and others (Step 8) |
| Lighting | two hanging bulbs as positional lights (Step 10) |
| No pre-computed animation | everything is a formula of one angle (Step 3) |
| Interactive | switches, camera presets, orbit, free camera, edge lines |

I also decided the level of detail up front: **simple shapes are enough**.
Gear teeth are rectangular blocks, the Geneva wheel is a hub with four arms,
and every object is a box or a cylinder. The effort goes into the mechanisms
and the graphics concepts, not into realistic shapes.

## Step 3 — the key design decision: one angle drives everything

Before drawing anything I decided how motion would work, because that shapes
the whole program:

> **The entire animation state of the machine is one float, `theta` (the crank
> angle), and one integer, `turns` (whole revolutions so far). Every moving
> part is computed from those two.**

No gear stores its own angle, no blank stores its own position, and there are
no keyframes. Each frame the program only does

```cpp
theta += CRANK_DPS * dt * RAD;   // then wrap to [0, 2π) and count turns
```

and each part asks "where am I for this `theta`?".

Why this is worth it:

- **Nothing can drift.** Five gears that each add up their own speed slowly
  slip out of mesh through rounding error. Five gears computed from one angle
  cannot.
- **It satisfies "no pre-computed animation" by construction** — every pose is
  calculated live.
- **Everything stays in step**: the belt moves exactly one station per press
  stroke because belt travel comes from the angle, not from elapsed time.
- **It is easy to explain**: here is the angle; here is each part's formula.

I chose the *crank* angle rather than the motor angle because one crank
revolution is exactly one part, so the important moments (top of stroke,
bottom of stroke) are simple angles.

The one exception is the exhaust fan in the room. It is not connected to the
machine, so it has its own angle, `fan_deg`.

## Step 4 — plotting the layout

I fixed a coordinate system first:

- right-handed, **Y up**, floor is `y = 0`;
- 1 unit ≈ 25 cm, so the belt top at `y = 3.0` is a 75 cm working height;
- parts flow in **+x** (left to right), the viewer stands at **+z**;
- **every rotating shaft is parallel to z**, so every mechanism turns in a plane
  facing the camera, and every rotation in the machine is
  `glRotatef(angle, 0, 0, 1)`.

Then I placed things by *constraints* rather than by eye:

1. **The gears go on a drive panel *behind* the line**, at `z = −0.85`. The
   crank, rod and ram must be on the belt's centre line (`z = 0`) to hit the
   blanks. A large gear in front would hide the crank; behind, it becomes a
   backdrop the crank is seen against.
2. **The Geneva wheel goes on the *front* end of the head roller**, because
   behind the belt it would be hidden. A shaft from the last gear reaches
   forward to its driver arm.
3. **Two idler gears, not one.** Every mesh reverses direction. With one idler
   the belt would run backwards; with two it runs `+x`. The distance between
   the crank gear and the Geneva gear (3.64) also needs two 20-tooth idlers to
   bridge it.
4. **The press stands over station 7**, the furthest upstream station the gear
   train can reach, which leaves two stations after it to show finished parts.
5. **The rollers are exactly 10 belt pitches apart**, so the belt loop is a
   whole number of pitches (24) and the 24 cleats line up where the loop
   closes.

Every number lives in [src/config.h](../src/config.h), one section per object.
Each section is tagged **`[free]`** (chosen, safe to change) or **`[coupled]`**
(derived — other geometry depends on it). That split came straight from this
step: I wrote down which numbers I had *chosen* and which I had *worked out*.

## Step 5 — deriving the mechanisms

Each mechanism was worked out on paper as a formula of `theta` and checked with
a calculator before it was coded. All the finished formulas are in
[src/layout.h](../src/layout.h).

### 5.1 Gears

- Pitch radius from the tooth count: `r = m·N/2`, with module `m = 0.05`.
- Two meshing gears sit exactly `r_i + r_j` apart.
- Speed at a mesh: `ω_j = −(N_i / N_j)·ω_i` (the minus sign is the reversal).

Teeth `12 : 36 : 20 : 20 : 36` give speeds, relative to the crank, of
`+3, −1, +1.8, −1.8, +1` — that is `GEAR_RATE` in `layout.h`.

The crank gear G2 is fixed by the press and the Geneva gear G5 by the Geneva
geometry, so the two idlers G3 and G4 had to be **solved**:

```
d1 = r2 + r3 = 1.40,   d2 = r3 + r4 = 1.00
u  = unit vector G2 → G5,   n = its left normal,   D = |G5 − G2| = 3.6432
along = (D − d2)/2 = 1.3216,   off = sqrt(d1² − along²) = 0.4620
G3 = G2 + along·u + off·n
G4 = G3 + d2·u
```

That is why `G3_X`, `G3_Y`, `G4_X`, `G4_Y` in `config.h` have six decimals and
are marked "solved, not chosen". Check: G1–G2 = 1.20, G2–G3 = 1.40,
G3–G4 = 1.00, G4–G5 = 1.40 — each exactly its pitch-radius sum.

**Tooth phase.** The right distance is not enough: a tooth on one gear must
face a *gap* on the other. The mesh law turns the driven gear by half a tooth,
`π/N_j`; without it the teeth would sit inside each other. I solved each gear's
starting angle once, at the pose `theta = 90°`, and stored them as `PHI_DEG`.
Each gear's angle is then

```cpp
gear_deg(i, th) = PHI_DEG[i] + GEAR_RATE[i] * (th·DEG − 90°)
```

### 5.2 Crank-slider press

The crank pin is at `(x_P + r·sin θ, Y_C + r·cos θ)`. The ram is held on the
vertical line `x = x_P` and joined to the pin by a rod of length `L`. Solving
the rod constraint gives the height of the ram's top:

```
s(θ) = Y_C + r·cos θ − sqrt(L² − r²·sin²θ)
```

With `r = 0.25` and `L = 1.00`, the stroke is exactly `2r = 0.50`: at the top
and bottom `sin θ = 0`, so the square root is just `L` both times and cancels.
I kept `L/r = 4`, so the rod leans at most `arcsin(r/L) = 14.5°` and it still
looks like a press.

**The vertical stack was built from the floor up**, which is how the crank
height was derived:

| Level | y | From |
|---|---|---|
| belt top | 3.00 | roller axis 2.59 + roller 0.39 + belt 0.02 |
| fresh blank top | 3.20 | + blank 0.20 |
| flattened blank top = punch at the bottom | 3.10 | + flat blank 0.10 |
| ram top, lowest | 3.50 | 3.10 + ram height 0.40 |
| crank axis `Y_C` | 4.75 | 3.50 + r + L |

### 5.3 Geneva drive

A 4-slot wheel with the driver pin at radius `a = 0.55`. For the pin to enter a
slot smoothly, the centre distance must be `c = a / sin(π/4) = 0.7778` and the
wheel radius `sqrt(c² − a²) = 0.55`. The pin is engaged for ±45° of the
driver's turn, and during that time the wheel angle is

```
β(α) = atan2(λ·sin α, 1 − λ·cos α),   λ = sin(π/4)
```

At `α = ±45°` this gives exactly `β = ±45°`, so **one driver revolution = one
quarter turn of the wheel**, and the wheel is locked for the other 270°.

### 5.4 The interlock — why the belt stops while the press is down

This is *derived*, not chosen. The punch is inside a blank when
`s(θ) − 0.40 < 3.20`. Solving with `c = cos θ`:

```
0.25c + 1.15 = sqrt(0.9375 + 0.0625c²)   →   c = −0.66957
θ = 132.03°  to  227.97°
```

The belt must stand still for that whole window. I placed the Geneva engagement
at `θ ∈ (315°, 45°)`, centred on the top of the stroke, which leaves 87° of
margin on each side. On a real press, moving the belt during the stroke would
shear the part — this is the "why" behind the motion.

### 5.5 Belt travel

The belt pitch is defined as a quarter turn of the roller:
`p = R_c·π/2 = 0.6283`. Belt travel in stations is

```
B = (indexes completed) + (fraction of the current index)
```

One trap: the index straddles `θ = 0`, exactly where `turns` goes up. Counting
indexes straight from `turns` would make the belt jump a whole station once per
part. `belt_travel()` counts an index as finished only when it *ends* (at 45°),
so `B` is smooth across the wrap.

### 5.6 Belt path, cleats and blanks

- The belt loop is four pieces — top run, around the head roller, bottom run,
  around the tail roller — each a whole number of pitches. `belt_path(s)`
  returns a position and a tilt for any distance `s` around the loop, and cleat
  *k* is drawn at `cleat_s(k) + B·p`.
- Blanks sit at stations `x_j = tail_x + j·p`, shifted forward by the fractional
  part of `B`. When `B` passes a whole number every blank has moved up one
  station, so each slot simply takes over the place of the one ahead.
- A blank's height depends only on where it is (`blank_h_at()`): 0.20 before
  the press, 0.10 after it, and *at* the press it follows the punch face down.

## Step 6 — setting up the build

- **Toolchain:** MSYS2 MinGW64 `g++` with `freeglut` and `glew`. Windows'
  `opengl32.dll` stops at OpenGL 1.1, so GLEW (`glewInit()`, right after the
  window is made) finds the OpenGL 2.0 shader functions for Phong shading.
- **One translation unit:** `main.cpp` includes the headers, so the build is
  one `g++` command.
- [build.ps1](../build.ps1) for PowerShell (`.\build.ps1 -Run`) and a
  [Makefile](../Makefile) for the MSYS2 shell (`make run`).
- Flags: `-std=c++17 -Wall -Wextra`, with `-O0 -g` for debugging and
  `-O2 -DNDEBUG` for release.

## Step 7 — coding the primitives

I wrote a small set of shape routines before any object, in
[src/prim.h](../src/prim.h):

| Routine | What it draws |
|---|---|
| `box(sx, sy, sz)` | a box centred on the origin, one normal per face |
| `box_span(x0,y0,z0, x1,y1,z1)` | a box between two corners — the most used call |
| `cyl(r, h)` | a cylinder rising along +y: a quad-strip wall plus two caps |
| `cyl_z(r, h)` | the same cylinder turned onto +z, for shafts and gears |
| `tiles_y`, `tiles_z` | a flat surface split into tiles (floor, walls) |
| `new_list()` | starts recording a display list |

Two rules I followed from the start:

1. **Consistent winding.** With back-face culling on, a cylinder whose strip
   winds the wrong way loses half its surface. Each routine notes which way it
   winds.
2. **Every part is generated at its true size.** No `glScalef` is used to size a
   part — the only scale in the machine is the blank being flattened.

Materials are in [src/materials.h](../src/materials.h): for each one a colour,
a specular colour `ks` and an exponent `ns` (brass, copper, silver, dark steel,
machine paint, rubber, safety yellow, …).

## Step 8 — coding the machine, part by part

Each part has its own header with the same two functions: `build_*()` records
the parts that never change into **display lists** once at startup, and
`draw_*(theta)` places the moving parts every frame.

| File | Part | Key idea in the code |
|---|---|---|
| [gears.h](../src/gears.h) | five gears | **one tooth display list drawn 124 times**; each gear is translate → rotate by `gear_deg` → body and teeth |
| [press.h](../src/press.h) | drive panel, crank, rod, ram | the crank disc and pin rotate together; the rod is placed at the crank pin and rotated by `rod_deg` to point at the ram |
| [geneva.h](../src/geneva.h) | driver arm and wheel | the wheel is a hub plus four box arms; the gaps between them are the slots |
| [conveyor.h](../src/conveyor.h) | frame, rollers, belt, cleats | the belt strips are static; the **24 cleats** carry all the visible belt motion |
| [blanks.h](../src/blanks.h) | blanks | one blank list; flattening is translate to the belt → `glScalef(1, h/0.20, 1)` → draw, so it scales *about its base* and stays on the belt |
| [fixtures.h](../src/fixtures.h) | floor, motor, stack light | static display lists |
| [scene.h](../src/scene.h) | the whole machine | the build order and the draw order in one place |

**Order of work.** I first drew every part at one fixed pose and checked each
position against the plotted layout. Then I switched on the `layout.h`
formulas one mechanism at a time — gears, press, Geneva, belt, blanks — and
checked each at a slow speed before starting the next.

**The hierarchy decision.** It is tempting to make each gear a child of the
gear that drives it. That is wrong: a child inherits its parent's rotation, but
a meshing gear turns the *other* way at a *different* rate. So every gear is a
child of the drive panel, and the mesh law links them as siblings. The real
parent–child chains are:

- panel → crankshaft → crank disc → crank pin → rod → ram
- panel → guide rails → ram
- panel → Geneva shaft → driver arm → pin
- conveyor frame → head roller → Geneva wheel; frame → belt path → cleat *k*;
  frame → station *j* → blank → squash

The first two meet at the ram, and the last two meet where the pin sits in the
slot, so the machine contains **two closed loops**. Both are closed by a
formula (`s(θ)` and `β(α)`), not by iteration.

## Step 9 — the render loop and the clock

[src/main.cpp](../src/main.cpp) connects everything to GLUT.

**Frame-rate-independent timing** (`idle()`):

```cpp
float dt = (now - prev) * 0.001f;     // seconds since the last frame
if (dt > DT_CLAMP) dt = DT_CLAMP;     // a window drag must not jump the line
theta += CRANK_DPS * dt * RAD;
while (theta >= 2π) { theta -= 2π; ++turns; }   // while, not if
```

The machine therefore runs at the same speed on a slow laptop and a fast PC.
At `CRANK_DPS = 72` one part takes 5 seconds.

**Each frame** (`display()`): clear → camera (`gluLookAt`) → place the lights →
draw the scene → draw the HUD → swap buffers.

**Hidden surfaces:** `GLUT_DEPTH` with `GL_DEPTH_TEST`, plus back-face culling.
Where two surfaces touch exactly (a blank's bottom on the belt, a cleat on the
belt) they face opposite ways, so culling removes one and there is no
z-fighting. The near plane is 4.0, measured from the nearest visible geometry
rather than a habitual 0.1, which gives much better depth precision.

**Edge lines** (key `e`): the scene is drawn a second time as lines
(`glPolygonMode(GL_LINE)`). `glPolygonOffset` pushes the filled faces back so
the lines do not fight them, and a multiply blend (`GL_ZERO, GL_SRC_COLOR`)
makes each edge a darker shade of its own face. This keeps two parts that meet
at a shallow angle readable as separate shapes.

## Step 10 — lighting

The scene is lit by **two hanging bulbs**, drawn as real fixtures, each a
positional `GL_LIGHT` with its own switch. It uses the Phong reflection model
from the slides: ambient + diffuse + specular, plus emission for the parts
that give out light.

Things that had to be right:

- `glLightfv(GL_POSITION, …)` is called **every frame, after the camera**
  (`place_lights()`), otherwise the lights move with the camera.
- The position's `w` is `1.0`, so they are positional lights, not directional
  ones.
- **Spotlights:** each points straight down (`GL_SPOT_DIRECTION`, set after the
  camera like the position), cut off at 90° because the flat shade blocks
  everything above the bulb, with a `cos^0.5` falloff (`GL_SPOT_EXPONENT`). I
  first tried an 80° cone with the bulbs at 5.4, but the pinion and motor stand
  above that height and went dark, and an 80° cone at 6.6 cut a line across the
  pinion. A 90° cone with the bulbs at 6.6 keeps the whole machine inside.
- **Attenuation:** each bulb's light is divided by `a0 + a1·d + a2·d²`. Without
  it, both bulbs hit the pale left wall at full strength and it burnt out to
  white; with it, the light falls off away from the machine.
- **Specular:** each material has `ks` and `ns`. Brass, polished silver, copper
  and black plastic use the values in the slides' coefficient table.
  `GL_LIGHT_MODEL_LOCAL_VIEWER` makes the highlight use the true direction to
  the eye.
- `GL_COLOR_MATERIAL` turns the `glColor` already recorded in each display list
  into that part's ambient and diffuse colour; `glMaterial` sets the rest.
- **Emission:** lit bulbs, lit lamp lenses and window daylight are drawn with
  `mat::glow()`, which sets the emission term, so they shine whatever the
  lights do.
- `GL_NORMALIZE`, because flattening a blank is a non-uniform scale and would
  otherwise stretch its normals.
- **Flat, Gouraud or Phong** (key `s`, [src/shading.h](../src/shading.h)):
  `GL_FLAT` shows the facets; `GL_SMOOTH` (Gouraud) lights each vertex and
  blends, with radial normals on cylinder walls so they look round; Phong is a
  small GLSL program that passes the normal to each pixel and lights it there.
  It reads the same `gl_LightSource` and `gl_FrontMaterial` state, so only
  *where* the lighting is worked out changes. Under Gouraud the drive panel
  (one big quad) needs a low `ks`, or a highlight on one corner spreads across
  the whole panel.
- A switched-off bulb sets its light's diffuse and specular to black, and its
  glass turns to dark glass by the same flag.

## Step 11 — the room, the camera and the switches

With the machine working, I placed it in a **cutaway workshop**
([src/room.h](../src/room.h)): walls with windows, ceiling beams, two pendant
lamps, a hazard border on the floor, a pallet of raw blanks and one of stamped
parts, an electrical cabinet, drums, a shelf of spare gears and an exhaust fan.

- **Cutaway walls:** each wall is drawn only while the eye is on the room side
  of it (`wall_shown()`), so whichever wall stands between the camera and the
  machine disappears as the camera orbits.
- **Reuse:** anything that appears more than once is one display list placed
  with a `glTranslatef` per copy — a window (9), an I-beam (5), a lamp (2), a
  pallet (2), a drum (2). The spare gears on the shelf and the blanks on both
  pallets call the machine's own lists and `draw_blank()`, so they are exactly
  the same parts; the stamped pallet's blanks get the belt's own squash.
- **Switches:** four switches on the cabinet — machine (`Space`), left bulb
  (`[`), right bulb (`]`), fan (`f`) — each with a status lamp matching its dot
  on the small HUD ([src/hud.h](../src/hud.h)).

The camera ([src/camera.h](../src/camera.h)):

- `1` / `2` — two `gluLookAt` presets: the line, and the whole room;
- `←` `→` — orbit the eye around the look-at point;
- `c` — a free camera that flies level (one yaw angle): arrows to fly and turn,
  PgUp/PgDn to rise and sink, all at `speed × dt`. Its near plane drops to 0.2
  so it can get close to a mechanism, and it is kept inside a cylinder around
  the room so the far plane (40) still covers everything;
- `r` — reset the crank and the camera.

## Step 12 — checking it

Because every motion is a formula, most of it could be checked with numbers
instead of by eye.

- **Derived numbers**, checked by hand calculation: every gear pair at its
  pitch-radius sum, the Geneva centre distance and wheel radius, the tail
  roller position, the rod angle, and the punch clearance.
- **Motion, over ten crank revolutions:** belt travel never runs backwards; ten
  revolutions give exactly ten indexes; the wheel turns 90° per index; the belt
  advances exactly one pitch per index; the stroke is exactly twice the throw;
  the punch bottoms out at exactly the flat-blank height; and **the punch never
  comes down while the belt is moving**.
- **By eye:** with edge lines on, the teeth never pass through each other and
  the pin slides into a slot without hitting an arm.

Mistakes that are easy to make here, and how the code avoids them:

| Mistake | What it looks like | How the code avoids it |
|---|---|---|
| counting an index when `theta` wraps | belt jumps one station per part | `belt_travel()` counts at the end of the index, 45° |
| no half-tooth `π/N` | teeth permanently inside each other | built into `PHI_DEG` |
| wrong winding in a cylinder | half the cylinder missing under culling | every routine in `prim.h` winds the same way |
| lights set before the camera | lights swim with the view | `place_lights()` runs after `cam::apply()` |
| `dt` spike from a window drag | the line lurches forward | `dt` clamped to `DT_CLAMP` |

## Step 13 — documentation

- [README.md](../README.md) — what the program is, how to build and run it, the
  controls, and what to demonstrate.
- [docs/OBJECTS.md](OBJECTS.md) — every object and whether each number was
  typed, derived or solved; all the formulas on one page; which numbers are
  safe to change.
- [docs/DEMO-CHANGES.md](DEMO-CHANGES.md) — how to change things live during
  the demo: move an object, recolour it, change the speed, add a new object.

## What I learned

1. **Do the maths on paper first.** With each derivation written down, any
   mismatch between the code and the drawing was quick to find.
2. **One source of truth for motion.** Driving everything from one angle made
   the animation impossible to knock out of step and easy to explain.
3. **Separate chosen numbers from derived ones.** Tagging `[free]` and
   `[coupled]` in `config.h` shows exactly what is safe to change.
4. **Meshing is not parenting.** The hierarchy has to describe the real
   structure, not the path the power takes.
5. **Constraints give reasons.** The interlock window, the near plane, the
   second idler — each came from a calculation, so each has an answer to "why
   is it like that?".
6. **Simple shapes are enough.** Boxes and cylinders were all the scene needed;
   the marks are in the mechanisms.
