# Quarter Turn — every object, and where its numbers come from

Branch `static-objects`. This is the reference for **what each object is made
of** and **where every number in it came from** — whether it was typed in by
hand or worked out from something else.

A link like [layout.h:32](../src/layout.h#L32) opens the code at that line.
Named constants such as `CRANK_R` all live in [config.h](../src/config.h).

---

## Contents

1. [How to read this](#1-how-to-read-this)
2. [The primitives everything is built from](#2-the-primitives-everything-is-built-from)
3. [The one clock](#3-the-one-clock)
4. [The machine, object by object](#4-the-machine-object-by-object)
5. [The room, object by object](#5-the-room-object-by-object)
6. [Every formula in one page](#6-every-formula-in-one-page)
7. [Which numbers are free and which are not](#7-which-numbers-are-free-and-which-are-not)

---

## 1. How to read this

**Units and axes.** One unit is roughly one metre. `+x` runs along the
conveyor towards the discharge end, `+y` is up, `+z` is towards the viewer.
Every rotating shaft in the machine is parallel to `z`, so nearly every
rotation in the scene is `glRotatef(angle, 0, 0, 1)`.

**Angles.** Degrees everywhere in the drawing code, because that is what
`glRotatef` takes. Radians only inside the trigonometry in
[layout.h](../src/layout.h), where `th` (theta, the crank angle) is always
radians. `DEG` and `RAD` convert.

**Two kinds of number.** Each object below says which it uses:

| | meaning |
|---|---|
| **typed** | a literal in `config.h`. Change it freely; nothing else depends on it. |
| **derived** | computed in `layout.h` from typed numbers. Do not type it in — change its inputs. |
| **solved** | a literal in `config.h` that was *calculated once* and written down, and that other geometry depends on. Section 7 lists these; they are the dangerous ones. |

**Build once, draw many.** Fixed geometry goes into an OpenGL display list at
startup ([main.cpp:170](../src/main.cpp#L170)) and is replayed each frame.
Anything that changes with the clock is built fresh every frame instead. Each
mechanism file has a `build_*` for the first kind and a `draw_*` for the
second. Each list is a named `GLuint` at the top of its file, such as
`tooth_list`: `new_list()` ([prim.h](../src/prim.h)) starts recording it,
`glEndList()` stops, and `glCallList(tooth_list)` replays it.

---

## 2. The primitives everything is built from

Six routines in [prim.h](../src/prim.h). There is no mesh loading and no
modelling tool anywhere in the project — every object is these six.

| Routine | Line | Shape | Notes |
|---|---|---|---|
| `box(sx, sy, sz)` | [14](../src/prim.h#L22) | cuboid centred on the origin | six faces, one `glNormal3f` each |
| `box_span(x0,y0,z0, x1,y1,z1)` | [31](../src/prim.h#L39) | cuboid between two corners | the form most of the machine uses |
| `cyl(r, h)` | [39](../src/prim.h#L47) | cylinder, base at `y=0`, axis `+y` | wall + two caps |
| `cyl_z(r, h)` | [67](../src/prim.h#L75) | the same, axis `+z` | one `glRotatef(90,1,0,0)` around `cyl` |
| `tiles_y(x0,z0,x1,z1, y, nx,nz, up)` | [74](../src/prim.h#L82) | horizontal rectangle in `nx × nz` cells | `up` picks the `+Y` or `−Y` face |
| `tiles_z(x0,y0,x1,y1, z, nu,nv)` | [90](../src/prim.h#L98) | vertical rectangle in `nu × nv` cells | faces `+Z` in its own frame |

A cylinder is `CYL_SLICES = 10` sided ([config.h:14](../src/config.h#L14)).
That is deliberately low: the edge pass outlines every facet, so a rounder
cylinder means a busier drawing.

The cell counts on `tiles_*` are **only** about looks. Every cell edge becomes
an outline, so the floor keeps 1.0-unit tiles — they are what shows the floor
receding in perspective — while a window pane is one cell.

---

## 3. The one clock

The entire machine is a function of two values held in
[main.cpp:20](../src/main.cpp#L20):

- `theta` — the crank angle in radians;
- `turns` — how many whole revolutions of it have finished.

`idle()` ([main.cpp:100](../src/main.cpp#L100)) does nothing but

```
theta += CRANK_DPS * dt * RAD        // CRANK_DPS = 72 deg/s, so 5 s a cycle
```

and rolls `turns` over at 360°. No part keeps an angle of its own, so no part
can drift out of step with another however long it runs. One crank revolution
is one finished part.

`theta` starts at `ENGAGE_DEG = 45°` ([layout.h:57](../src/layout.h#L57)),
just after an index, so the belt begins on a whole station.

---

## 4. The machine, object by object

### 4.1 Drive panel — `build_press` [press.h:11](../src/press.h#L11)

One `box`. The flat green slab everything else is mounted on.

| Number | Value | Kind |
|---|---|---|
| `PANEL_CX, PANEL_CY, PANEL_CZ` | 3.2, 2.75, −1.0 | typed |
| `PANEL_W, PANEL_H, PANEL_D` | 5.2, 5.5, 0.10 | typed |

### 4.2 Motor — `build_fixtures` [fixtures.h:13](../src/fixtures.h#L13)

A `cyl_z` body, a `box_span` saddle on the panel's top edge, and a short `cyl_z` stub shaft
running into the first gear. It is centred on `G1` so it reads as driving the
pinion, though nothing is transmitted — the gear angles come from the clock.

| Number | Value | Kind |
|---|---|---|
| `MOTOR_R` | 0.35 | typed |
| `MOTOR_Z0, MOTOR_Z1` | −1.75, −0.95 | typed |
| position | `G1_X, G1_Y` | derived (follows the pinion) |

### 4.3 Gear train — `build_gears` / `draw_gears` [gears.h:15](../src/gears.h#L15), [gears.h:56](../src/gears.h#L56)

Five gears, each a root cylinder plus a hub cylinder, with rectangular teeth
instanced around it. **One tooth display list is drawn 124 times** (12+36+20+20+36).

Teeth are blocks, not involutes. A block tooth is honest at this scale and is
the reason `TOOTH_W_FRAC` is under ½ — two block teeth would jam at the mesh
otherwise.

| Quantity | Formula / value | Where |
|---|---|---|
| tooth counts `N` | `{12, 36, 20, 20, 36}` typed | [config.h:43](../src/config.h#L43) |
| module `m` | 0.05 typed | [config.h:42](../src/config.h#L42) |
| pitch radius | `r = m·N/2` → 0.30, 0.90, 0.50, 0.50, 0.90 — **derived** | [layout.h:16](../src/layout.h#L16) |
| root radius | `r − 1.25m` — derived | [gears.h:12](../src/gears.h#L12) |
| tip radius | `r + 1.00m` — derived | [gears.h:13](../src/gears.h#L13) |
| tooth width | `TOOTH_W_FRAC · π · m` — derived | [gears.h:19](../src/gears.h#L19) |
| centres G1, G2, G5 | typed | [config.h:44](../src/config.h#L44) |
| centres G3, G4 | **solved** — see §7 | [config.h:46](../src/config.h#L46) |
| pose at `theta = 90°` | `PHI_DEG[5]` solved | [layout.h:13](../src/layout.h#L13) |
| rotation | `gear_deg(i, th) = PHI_DEG[i] + GEAR_RATE[i]·(th − 90°)` | [layout.h:28](../src/layout.h#L28) |

**The rate law.** At a mesh the next gear reverses and scales by the inverse
tooth ratio:

```
rate[j] = -rate[i] * N[i] / N[j]
```

With G2 as the reference at −1 this gives `GEAR_RATE = {3, −1, 1.8, −1.8, 1}`
([layout.h:26](../src/layout.h#L26)). G2 → G3 → G4 → G5 is three meshes, so G5
runs backwards against G2 — which is why the crank and the Geneva driver turn
opposite ways. **The sign of this table is not free**; see §7.

Brass and copper alternate along the train so the two gears at every mesh are
different colours and each pair visibly turns opposite ways.

### 4.4 Crank-slider press — `draw_press` [press.h:28](../src/press.h#L28)

Built fresh each frame: crank shaft (`cyl_z`), crank disc (`cyl_z`), crank pin
(`cyl_z`), connecting rod (`box`), wrist pin (`cyl_z`), ram (`box`). The guide
rails and brackets are static and live in the panel list.

| Quantity | Formula | Where |
|---|---|---|
| ram height | `s(t) = Y_C + r·cos t − √(L² − r²·sin²t)` | [layout.h:32](../src/layout.h#L32) |
| punch face | `ram_top(t) − RAM_H` | [layout.h:37](../src/layout.h#L37) |
| crank disc angle | `90° − t` | [layout.h:39](../src/layout.h#L39) |
| crank pin | `(PRESS_X + r·sin t, CRANK_Y + r·cos t)` | [layout.h:40](../src/layout.h#L40) |
| rod angle | `atan2(ram_top − pin_y, PRESS_X − pin_x)` | [layout.h:43](../src/layout.h#L43) |

`r = CRANK_R = 0.25`, `L = ROD_L = 1.00`, `Y_C = CRANK_Y = 4.75`, all typed.

Consequences that fall out of the law, not typed anywhere:

- **stroke = 2r = 0.500** exactly;
- `t = 0` is the top of the stroke, `t = 180°` the bottom;
- `L/r = 4`, so the worst rod lean is `asin(r/L) = 14.48°`;
- at the bottom the punch leaves a gap of exactly `BLANK_H_FLAT = 0.10`, which
  is what makes a stamped blank come out at exactly the flat height.

**The disc angle and the pin formula must agree.** `(cos(90−t), sin(90−t)) =
(sin t, cos t)` — that identity is the whole reason `crank_deg` is `90 − t`. If
one is changed without the other, the rod visibly misses the knob on the disc.

### 4.5 Geneva drive — `build_geneva` / `draw_geneva_driver` [geneva.h:10](../src/geneva.h#L10), [geneva.h:29](../src/geneva.h#L29)

The mechanism the project is named after. A driver arm on G5's shaft carries a
pin; the pin enters one of four slots in a wheel keyed to the conveyor's head
roller, turns it a quarter turn, and leaves.

The wheel is drawn as a hub disc with four arms at 45°, 135°, 225°, 315°. **The
gaps between the arms are the slots**, so the slots face 0°, 90°, 180°, 270°.

| Quantity | Formula / value | Where |
|---|---|---|
| slots `n` | 4 typed | [config.h:78](../src/config.h#L78) |
| pin orbit `a` | `GEN_A` = 0.55 typed | [config.h:79](../src/config.h#L79) |
| line of centres | `GEN_LOC_DEG` = 135° typed | [config.h:80](../src/config.h#L80) |
| `λ` | `sin(π/n)` = 0.7071 — derived | [layout.h:47](../src/layout.h#L47) |
| centre distance `c` | `a / λ` = 0.7778 — derived | [layout.h:48](../src/layout.h#L48) |
| wheel radius | `√(c² − a²)` = 0.5500 — derived | [layout.h:49](../src/layout.h#L49) |
| driver angle `α` | `wrap180(t)` | [layout.h:54](../src/layout.h#L54) |
| arm angle | `GEN_LOC_DEG + α` | [layout.h:55](../src/layout.h#L55) |
| engagement half-angle | `90° − 180°/n` = 45° — derived | [layout.h:57](../src/layout.h#L57) |
| wheel angle `β` | `atan( λ·sin α / (1 − λ·cos α) )` | [layout.h:59](../src/layout.h#L59) |

**Where β comes from.** Put the driver at the origin with the wheel at distance
`c` along `+x`. The pin sits at `(a·cos α, a·sin α)`, so seen from the wheel's
centre it lies at angle

```
β = atan( λ·sin α / (1 − λ·cos α) ),    λ = a/c = sin(π/n)
```

away from the direction back towards the driver. The slot the pin is in must
point along that line, so β **is** the wheel's angle. It sweeps −45° to +45°
across one index — a 90° quarter turn, once per crank revolution.

The wheel turns *against* the driver, so the wheel's own rotation is `−β`, and
that sign matters: getting it wrong leaves the ends of each index looking
correct while the pin cuts through the wheel arms in the middle.

Deliberate simplification: there is **no lock disc**. A real Geneva has a
circular blank on the driver that traps the wheel between indexes. Here the
wheel is held still by the arithmetic instead, and the driver sweeps through
the space where the lock would be.

### 4.6 Conveyor — `build_conveyor` / `draw_conveyor` [conveyor.h:14](../src/conveyor.h#L14), [conveyor.h:66](../src/conveyor.h#L66)

Static: side frame rails and four legs (`box_span`), the belt's two straight
runs (`box_span`) and two half-shell wraps (`cyl_z`). Per frame: two rollers,
the Geneva wheel, a stub shaft, and 24 cleats.

| Quantity | Formula / value | Where |
|---|---|---|
| belt centreline radius | `BELT_R_C` = 0.40 typed | [config.h:98](../src/config.h#L98) |
| **station pitch** | `p = R_c · π/2` = 0.628319 — derived | [layout.h:83](../src/layout.h#L83) |
| roller span | `10p` — derived | [layout.h:84](../src/layout.h#L84) |
| head roller | `HEAD_X` = 4.000 typed | [config.h:95](../src/config.h#L95) |
| tail roller | `HEAD_X − 10p` = −2.283185 — derived | [layout.h:85](../src/layout.h#L85) |
| loop length | `24p` — derived | [layout.h:86](../src/layout.h#L86) |
| station *j* | `tail_x + j·p` — derived | [layout.h:87](../src/layout.h#L87) |
| cleat *k* | at `(k + ½)·p` along the loop | [layout.h:117](../src/layout.h#L117) |
| roller / wheel angle | `−90°·B` | [layout.h:80](../src/layout.h#L80) |

**Why the pitch is what it is.** `p = R_c·π/2` is exactly the arc a quarter
turn of the roller drags. That single choice is what makes one Geneva index
advance the belt exactly one station — it is not tuned, it is construction.
`CLEAT_N = 24` follows the same way: the loop is `2·(10p) + 2πR_c = 24p`.

Cleats sit half a pitch off the stations so a cleat never overlaps a blank.

**`belt_path(s)`** ([layout.h:92](../src/layout.h#L92)) maps a distance along
the loop to a position and a rotation, in four pieces: top run, head wrap,
bottom run, tail wrap. It is what puts cleats correctly round the roller ends.

### 4.7 Belt travel — [layout.h:64](../src/layout.h#L64), [layout.h:70](../src/layout.h#L70)

`B` is belt travel **in stations**, and everything that rides the belt uses it.

```
index_progress(th) = (45° + β(α)) / 90°     while |α| ≤ 45°, else 0
```

One index straddles `theta = 0`, so `turns` has already ticked over while the
index is still half done. `belt_travel` therefore reads the revolution in three
pieces:

| theta | meaning | `B` |
|---|---|---|
| `< 45°` | second half of index `turns−1` | `(turns−1) + progress` |
| `< 315°` | no index; the last one is finished and counted | `turns` |
| `≥ 315°` | first half of index `turns` | `turns + progress` |

which is continuous across both joins.

### 4.8 Blanks — `build_blanks` / `draw_blanks` [blanks.h:12](../src/blanks.h#L12), [blanks.h:31](../src/blanks.h#L31)

One `cyl` list, drawn for every blank: nine on the belt, nine on each pallet,
and the ones in the bin or on their way to it. `BLANK_R` = 0.18, `BLANK_H` = 0.20,
`BLANK_H_FLAT` = 0.10, all typed.

Position: slot *j* sits at `station_x(j) + frac(B)·p`. When `B` passes a whole
number every blank has moved up one station, so the blank drawn at slot *j*
takes over the place slot *j−1* just left — one reaches the head roller, where
`draw_bin` takes it over, one arrives at station 1, and nothing in between
appears to move.

Height is read straight off where the blank stands and where the punch is,
with no per-blank state at all ([layout.h:119](../src/layout.h#L119)):

```
x < PRESS_X  →  BLANK_H           not yet stamped
x > PRESS_X  →  BLANK_H_FLAT      already stamped
otherwise    →  clamp(punch_face(th) − BELT_TOP_Y, BLANK_H_FLAT, BLANK_H)
```

The squash is `glScalef(1, h/BLANK_H, 1)` about the blank's base. It is the
only `glScalef` in the machine, and it is a non-uniform scale, which is why
`GL_NORMALIZE` is on.

### 4.9 Bin — `build_bin` / `draw_in_bin` / `draw_bin` [blanks.h:43](../src/blanks.h#L43), [blanks.h:60](../src/blanks.h#L60), [blanks.h:76](../src/blanks.h#L76)

An open box just past the head roller: a floor and four walls, five
`box_span`, in one list. `BIN_X` 5.40, `BIN_Z` 0.00, `BIN_S` 1.30, `BIN_H` 0.50,
`BIN_T` 0.05, all typed.

Every index carries one blank off the head roller, so after `B` indexes the bin
holds `floor(B)` blanks. There is no counter and no per-blank state, and the
count stops at 36 (`BIN_LAYERS` × 9). They lie 3 × 3 a layer at `PAL_PITCH`, as
on the pallets, each `BLANK_H_FLAT` tall.

One routine, `draw_in_bin(i, u)`, draws blank *i* of the pile. At `u` = 1 it
sits in its slot, which is how every blank already in the bin is drawn. At
`u` = 0 it sits on top of the head roller, at station 10. The next blank, number
`floor(B)`, waits there while the belt is locked and falls during the next
index, with `u` the time through that index:

```
u    = (α + 45°) / 90°                  0 → 1 while the belt indexes
y    = top + (slot − top)·u²            a fall: slow, then fast
x, z = top + (slot − top)·(1 − (1 − u)³)  the push off the belt: fast, then slow
```

It lands just as the index ends, which is the moment `floor(B)` counts it in.
When the bin is full, each new blank lands on the top slot, which is already
drawn.

The blank stays flat as it falls. x is pushed out fast so the blank is clear of
the roller before it has dropped far; with `(1 − u)²` its back edge sank 0.064
into the belt. Checked numerically for all 36 slots, it clears the Geneva shaft
by 0.16, the bin walls by 0.04 and the blanks already in the pile by 0.013. A
flat disc that starts lying on the roller cannot leave it without a small dip:
for the column nearest the roller, its back edge dips up to 0.018 into the
belt for the first 0.13 s.

### 4.10 Stack light — `draw_stack_light` [fixtures.h:44](../src/fixtures.h#L44)

A post (`cyl`), a housing (`cyl`), and three lens segments from one list. Green,
amber, red upward. `i == 0` ([fixtures.h:61](../src/fixtures.h#L61)) picks
the lit one — green, fixed.

A lit lens uses the **emission** term: a lamp makes light, it does not need to
catch it. `mat::lens()` ([materials.h](../src/materials.h)) makes the lit one
glow in its full colour, and leaves the others as dim plastic that only
reflects.

---

## 5. The room, object by object

All in [room.h](../src/room.h). The room is not in the PRD; it is a deviation,
recorded in the README.

**No cutaway.** Every camera is locked inside the room (`keep_inside`,
[camera.h:25](../src/camera.h#L25)), so all four walls and the ceiling are drawn
every frame. The room is 23 × 16 × 9.5: x −10 → 13, z −2.5 → 13.5. The back
wall is still just behind the drive panel; the room grew to the left, the right
and the front.

**Wall frames.** Each wall is built in its own frame: `u` runs along the wall
left to right as seen from inside, `y` is up, `z` points out of the wall into
the room (`enter_wall`, [room.h:35](../src/room.h#L35)). One window list
then serves all four walls. `back_u`/`left_u`/`right_u`/`front_u`
([room.h:45](../src/room.h#L45)) convert a world coordinate into that wall's `u`.

**Build once, draw many.** Every room part that appears more than once is one
display list, recorded once in `build_lists()` ([room.h:271](../src/room.h#L271))
and placed with a `glTranslatef` per copy. The parts are built first, because
a list that calls another records its id when it is compiled.

| List | Drawn | Where the copies go |
|---|---|---|
| `window_list` | 9 | `window_at(u)` on each wall, inside that wall's list |
| `beam_list` | 5 | `ceiling()`, one per `BEAM_PITCH` |
| `lamp_list` | 2 | `draw()`, at each bulb |
| `globe_list` | 2 | `draw()`, coloured per frame by its switch |
| `pallet_list` | 2 | `loaded_pallet()`, at `PAL_X[p], PAL_Z[p]` |
| `drum_list` | 2 | `drums()`, coloured before each call |

| Object | Built from | Line | Key numbers |
|---|---|---|---|
| Floor | `tiles_y`, 23 × 16 cells of 1.0 | [fixtures.h:17](../src/fixtures.h#L17) | `ROOM_X0/X1/Z0/Z1` typed — the floor is the room's footprint |
| Wall surface | two `tiles_z` bands + a trim `box_span` | [room.h:50](../src/room.h#L50) | `DADO_H` 1.20, `CEIL_Y` 9.5 typed |
| Frame | four `box_span` bars round an opening | [room.h:63](../src/room.h#L63) | shared by the windows and the fan housing |
| Window ×17 | one glowing `tiles_z` pane + `frame()` + a cross bar | [room.h:72](../src/room.h#L72) | `WIN_W` 1.80, `WIN_H` 1.80, `WIN_BAR` 0.05 typed |
| Ceiling | `tiles_y` facing **down** + the beams | [room.h:138](../src/room.h#L138) | — |
| I-beam ×7 | three `box_span` (flange, web, flange) | [room.h:130](../src/room.h#L130) | `BEAM_X0` −7.00, `BEAM_PITCH` 3.00 |
| Pendant lamp ×2 | thin `cyl` flex + two `cyl` for the shade | [room.h:154](../src/room.h#L154) | `BULB_X[2]`, `BULB_Y` 6.60, `SHADE_R` 0.30 |
| Bulb globe ×2 | `cyl` radius `BULB_R` | [room.h:165](../src/room.h#L165) | glows when on, dark glass when off |
| Hazard border | four `tiles_y` strips, 0.004 proud | [room.h:178](../src/room.h#L178) | `HAZ_*` typed |
| Pallet ×2 | 3 bearers + 5 deck boards | [room.h:188](../src/room.h#L188) | `PAL_S` 1.20, `PAL_TOP` 0.13 typed |
| Load of blanks ×2 | 9 × `scene::draw_blank()` | [room.h:203](../src/room.h#L203) | raw (`BLANK_H`) by the tail, stamped (`BLANK_H_FLAT`) by the head |
| Cabinet | one `box_span` + door seam + four switch plates | [room.h:219](../src/room.h#L219) | `CAB_X0/X1/H/D` typed |
| Drum ×2 | `cyl` body + three `cyl` hoops | [room.h:233](../src/room.h#L233) | `DRUM_R` 0.30, `DRUM_H` 0.88 |
| Spare-gear shelf | `box_span` shelf + 2 brackets + 2 gears | [room.h:114](../src/room.h#L114) | calls `scene::gear_shape()` |
| Exhaust fan | housing (`frame()` + grille) + 6-blade rotor | [room.h:97](../src/room.h#L97), [room.h:254](../src/room.h#L254) | `FAN_BLADES` 6 |
| Switch lamps ×4 | `cyl_z`, glowing when on | [room.h:329](../src/room.h#L329) | colours shared with the HUD |

Three room objects deliberately **reuse the machine's own parts**, so they are
exactly the parts they represent: the spare gears on the shelf are drawn by
`scene::gear_shape()` from `gear_list` and `tooth_list`, and both pallets are
loaded by `scene::draw_blank()` from `blank_list` — the stamped load with the
same squash the belt uses. That is why
`room::build_lists()` has to run *after* `scene::build_lists()`
([main.cpp:170](../src/main.cpp#L170)).

**The fan** is the one moving part not on the machine's clock: it keeps its own
`fan_deg`, advanced in `idle()` while its switch is on.

---

## 6. Every formula in one page

```
gear i angle        phi_i(t)  = PHI_DEG[i] + GEAR_RATE[i]·(t − 90°)
gear rate law                   rate[j]   = −rate[i]·N[i]/N[j]
pitch radius        r_i       = m·N_i/2

ram height          s(t)      = Y_C + r·cos t − √(L² − r²·sin²t)
punch face                      s(t) − RAM_H
crank disc angle                90° − t
crank pin                       (PRESS_X + r·sin t, CRANK_Y + r·cos t)
rod angle                       atan2(s − pin_y, PRESS_X − pin_x)

Geneva λ                        sin(π/n)
centre distance     c         = a/λ
wheel radius                    √(c² − a²)
driver angle        α(t)      = wrap180(t)
wheel angle         β(α)      = atan( λ·sin α / (1 − λ·cos α) )
engagement half                 90° − 180°/n
index progress                  (45° + β)/90°     while |α| ≤ 45°

station pitch       p         = R_c·π/2
station j                       tail_x + j·p
tail roller                     HEAD_X − 10p
loop length                     24p
roller / wheel angle            −90°·B
blank height                    clamp(punch_face − BELT_TOP_Y, FLAT, FULL)
blanks in the bin               min(floor(B), 36)
falling blank       u         = (α + 45°)/90°, y ∝ u², x and z ∝ 1 − (1 − u)³
```

**Checked numerically** over ten crank revolutions: belt travel never runs
backwards, ten revolutions give exactly ten indexes, the wheel turns 90° per
index, the belt advances exactly one pitch per index, the ram stroke is exactly
`2r`, the punch bottoms out at exactly the flat blank height, the pin tracks a
slot centreline to within 0.0001° through the whole engagement, and the punch
never comes down while the belt is indexing.

---

## 7. Which numbers are free and which are not

Change anything in the first list freely. Anything in the second has geometry
depending on it.

**Free — change and rebuild.** Colours ([materials.h](../src/materials.h)),
`CRANK_DPS` and `FAN_DPS`, `CYL_SLICES`, all the room dimensions, the cabinet,
drums, pallet, shelf, beams, windows, hazard border, camera presets, light
colours, `BLANK_R`, panel and motor sizes.

**Coupled — read this first.**

| Number | What depends on it |
|---|---|
| `TEETH`, `MODULE` | every pitch radius, and therefore **every gear centre**. Two meshing gears must sit exactly `r_i + r_j` apart. |
| `G3_X/Y`, `G4_X/Y` ([config.h:46](../src/config.h#L46)) | **solved**, not typed by choice — see the construction below. All four meshes currently sit at their exact pitch-radius sums. |
| `GEAR_RATE` signs | not free. The belt must feed towards the head roller → fixes the wheel's direction → fixes the driver's → fixes G5's → and G2 is three meshes back. |
| `BELT_R_C` | the station pitch, and so every station position, the tail roller, the loop length and `CLEAT_N`. |
| `GEN_A`, `GEN_SLOTS` | the centre distance and wheel radius, and therefore where the head roller must sit relative to G5. They are currently 0.7778 apart, exactly `c`. |
| `CRANK_R`, `ROD_L`, `RAM_H` | the stroke, and the punch gap at the bottom — which is what makes a stamped blank exactly `BLANK_H_FLAT` tall. |
| `POSE_DEG` (90°) | `PHI_DEG` was solved at this angle. Changing it rotates the whole train out of its solved pose. |
| `PRESS_X` | station 7 is at exactly `PRESS_X`. The blank height rule compares against `PRESS_X`. |
| `BIN_X`, `BIN_Z` | the falling blank aims at the bin wherever it is, but at x 5.4 it clears the Geneva shaft by 0.16. Much closer and it clips the shaft and dips further into the belt as it leaves the roller. |
| `BIN_S`, `BIN_H` | the bin has to hold a 3 × 3 layer, so `BIN_S` ≥ 2·`PAL_PITCH` + 2·`BLANK_R` + 2·`BIN_T` = 1.22, and the full pile, so `BIN_H` ≥ `BIN_T` + `BIN_LAYERS`·`BLANK_H_FLAT` = 0.45. |

### How G3 and G4 were placed

The two idlers bridge G2 to G5. They were solved once from the meshing
condition and the results written into `config.h` as literals:

```
d1    = r2 + r3 = 1.40                  G2-G3 must be this far apart
d2    = r3 + r4 = 1.00                  G3-G4 must be this far apart
u     = unit vector G2 -> G5            n = its left normal
D     = |G5 - G2| = 3.6432
along = (D - d2) / 2   = 1.3216
off   = sqrt(d1^2 - along^2) = 0.4620
G3    = G2 + along*u + off*n
G4    = G3 + d2*u
```

Verified: G1–G2 = 1.200 (= 0.30+0.90), G2–G3 = 1.400 (= 0.90+0.50),
G3–G4 = 1.000 (= 0.50+0.50), G4–G5 = 1.400 (= 0.50+0.90). Every pair sits at
exactly its pitch-radius sum, which is what "meshing" means.

> This construction, and the camera's near/far reasoning, used to be written as
> comments in `config.h`. Those comments are no longer in the file, so this
> document and the git history are now where they live.

**If you change a coupled number**, the quickest sanity check is to watch one
index at low speed: the pin should slide down a slot without touching an arm,
and the teeth should stay meshed at every pair.
