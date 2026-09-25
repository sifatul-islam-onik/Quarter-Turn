# Quarter Turn — every object, and where its numbers come from

Branch `static-objects`. This is the reference for **what each object is made
of** and **where every number in it came from** — whether it was typed in by
hand or worked out from something else.

A link like [layout.h:59](../src/layout.h#L59) opens the code at that line.
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
startup ([main.cpp:162](../src/main.cpp#L162)) and is replayed each frame.
Anything that changes with the clock is built fresh every frame instead. Each
mechanism file has a `build_*` for the first kind and a `draw_*` for the
second.

---

## 2. The primitives everything is built from

Six routines in [prim.h](../src/prim.h). There is no mesh loading and no
modelling tool anywhere in the project — every object is these six.

| Routine | Line | Shape | Notes |
|---|---|---|---|
| `box(sx, sy, sz)` | [14](../src/prim.h#L14) | cuboid centred on the origin | six faces, one `glNormal3f` each |
| `box_span(x0,y0,z0, x1,y1,z1)` | [31](../src/prim.h#L31) | cuboid between two corners | the form most of the machine uses |
| `cyl(r, h)` | [39](../src/prim.h#L39) | cylinder, base at `y=0`, axis `+y` | wall + two caps |
| `cyl_z(r, h)` | [67](../src/prim.h#L67) | the same, axis `+z` | one `glRotatef(90,1,0,0)` around `cyl` |
| `tiles_y(x0,z0,x1,z1, y, nx,nz, up)` | [74](../src/prim.h#L74) | horizontal rectangle in `nx × nz` cells | `up` picks the `+Y` or `−Y` face |
| `tiles_z(x0,y0,x1,y1, z, nu,nv)` | [90](../src/prim.h#L90) | vertical rectangle in `nu × nv` cells | faces `+Z` in its own frame |

A cylinder is `CYL_SLICES = 10` sided ([config.h:10](../src/config.h#L10)).
That is deliberately low: the edge pass outlines every facet, so a rounder
cylinder means a busier drawing.

The cell counts on `tiles_*` are **only** about looks. Every cell edge becomes
an outline, so the floor keeps 1.0-unit tiles — they are what shows the floor
receding in perspective — while a window pane is one cell.

---

## 3. The one clock

The entire machine is a function of two values held in
[main.cpp:17](../src/main.cpp#L17):

- `theta` — the crank angle in radians;
- `turns` — how many whole revolutions of it have finished.

`idle()` ([main.cpp:93](../src/main.cpp#L93)) does nothing but

```
theta += CRANK_DPS * dt * RAD        // CRANK_DPS = 72 deg/s, so 5 s a cycle
```

and rolls `turns` over at 360°. No part keeps an angle of its own, so no part
can drift out of step with another however long it runs. One crank revolution
is one finished part.

`theta` starts at `START_DEG = 45°` ([layout.h:106](../src/layout.h#L106)),
just after an index, so the belt begins on a whole station.

---

## 4. The machine, object by object

### 4.1 Drive panel — `build_press` [press.h:9](../src/press.h#L9)

One `box`. The flat green slab everything else is mounted on.

| Number | Value | Kind |
|---|---|---|
| `PANEL_CX, PANEL_CY, PANEL_CZ` | 3.2, 2.75, −1.0 | typed |
| `PANEL_W, PANEL_H, PANEL_D` | 5.2, 5.5, 0.10 | typed |

### 4.2 Motor — `build_fixtures` [fixtures.h:9](../src/fixtures.h#L9)

A `cyl_z` body, a `box_span` mount bracket, and a short `cyl_z` stub shaft
running into the first gear. It is centred on `G1` so it reads as driving the
pinion, though nothing is transmitted — the gear angles come from the clock.

| Number | Value | Kind |
|---|---|---|
| `MOTOR_R` | 0.35 | typed |
| `MOTOR_Z0, MOTOR_Z1` | −1.75, −0.95 | typed |
| position | `G1_X, G1_Y` | derived (follows the pinion) |

### 4.3 Gear train — `build_gears` / `draw_gears` [gears.h:13](../src/gears.h#L13), [gears.h:41](../src/gears.h#L41)

Five gears, each a root cylinder plus a hub cylinder, with rectangular teeth
instanced around it. **One tooth display list is drawn 124 times** (12+36+20+20+36).

Teeth are blocks, not involutes. A block tooth is honest at this scale and is
the reason `TOOTH_W_FRAC` is under ½ — two block teeth would jam at the mesh
otherwise.

| Quantity | Formula / value | Where |
|---|---|---|
| tooth counts `N` | `{12, 36, 20, 20, 36}` typed | [config.h:34](../src/config.h#L34) |
| module `m` | 0.05 typed | [config.h:31](../src/config.h#L31) |
| pitch radius | `r = m·N/2` → 0.30, 0.90, 0.50, 0.50, 0.90 — **derived** | [layout.h:33](../src/layout.h#L33) |
| root radius | `r − 1.25m` — derived | [gears.h:10](../src/gears.h#L10) |
| tip radius | `r + 1.00m` — derived | [gears.h:11](../src/gears.h#L11) |
| tooth width | `TOOTH_W_FRAC · π · m` — derived | [gears.h:19](../src/gears.h#L19) |
| centres G1, G2, G5 | typed | [config.h:41](../src/config.h#L41) |
| centres G3, G4 | **solved** — see §7 | [config.h:45](../src/config.h#L45) |
| pose at `theta = 90°` | `PHI_DEG[5]` solved | [layout.h:19](../src/layout.h#L19) |
| rotation | `gear_deg(i, th) = PHI_DEG[i] + GEAR_RATE[i]·(th − 90°)` | [layout.h:55](../src/layout.h#L55) |

**The rate law.** At a mesh the next gear reverses and scales by the inverse
tooth ratio:

```
rate[j] = -rate[i] * N[i] / N[j]
```

With G2 as the reference at −1 this gives `GEAR_RATE = {3, −1, 1.8, −1.8, 1}`
([layout.h:53](../src/layout.h#L53)). G2 → G3 → G4 → G5 is three meshes, so G5
runs backwards against G2 — which is why the crank and the Geneva driver turn
opposite ways. **The sign of this table is not free**; see §7.

Brass and copper alternate along the train so the two gears at every mesh are
different colours and each pair visibly turns opposite ways.

### 4.4 Crank-slider press — `draw_press` [press.h:26](../src/press.h#L26)

Built fresh each frame: crank shaft (`cyl_z`), crank disc (`cyl_z`), crank pin
(`cyl_z`), connecting rod (`box`), wrist pin (`cyl_z`), ram (`box`). The guide
rails and brackets are static and live in the panel list.

| Quantity | Formula | Where |
|---|---|---|
| ram height | `s(t) = Y_C + r·cos t − √(L² − r²·sin²t)` | [layout.h:59](../src/layout.h#L59) |
| punch face | `ram_top(t) − RAM_H` | [layout.h:64](../src/layout.h#L64) |
| crank disc angle | `90° − t` | [layout.h:66](../src/layout.h#L66) |
| crank pin | `(PRESS_X + r·sin t, CRANK_Y + r·cos t)` | [layout.h:67](../src/layout.h#L67) |
| rod angle | `atan2(ram_top − pin_y, PRESS_X − pin_x)` | [layout.h:70](../src/layout.h#L70) |

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

### 4.5 Geneva drive — `build_geneva` / `draw_geneva_driver` [geneva.h:8](../src/geneva.h#L8), [geneva.h:27](../src/geneva.h#L27)

The mechanism the project is named after. A driver arm on G5's shaft carries a
pin; the pin enters one of four slots in a wheel keyed to the conveyor's head
roller, turns it a quarter turn, and leaves.

The wheel is drawn as a hub disc with four arms at 45°, 135°, 225°, 315°. **The
gaps between the arms are the slots**, so the slots face 0°, 90°, 180°, 270°.

| Quantity | Formula / value | Where |
|---|---|---|
| slots `n` | 4 typed | [config.h:95](../src/config.h#L95) |
| pin orbit `a` | `GEN_A` = 0.55 typed | [config.h:96](../src/config.h#L96) |
| line of centres | `GEN_LOC_DEG` = 135° typed | [config.h:97](../src/config.h#L97) |
| `λ` | `sin(π/n)` = 0.7071 — derived | [layout.h:74](../src/layout.h#L74) |
| centre distance `c` | `a / λ` = 0.7778 — derived | [layout.h:75](../src/layout.h#L75) |
| wheel radius | `√(c² − a²)` = 0.5500 — derived | [layout.h:76](../src/layout.h#L76) |
| driver angle `α` | `wrap180(t)` | [layout.h:81](../src/layout.h#L81) |
| arm angle | `GEN_LOC_DEG + α` | [layout.h:82](../src/layout.h#L82) |
| engagement half-angle | `90° − 180°/n` = 45° — derived | [layout.h:84](../src/layout.h#L84) |
| wheel angle `β` | `atan( λ·sin α / (1 − λ·cos α) )` | [layout.h:86](../src/layout.h#L86) |

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

### 4.6 Conveyor — `build_conveyor` / `draw_conveyor` [conveyor.h:9](../src/conveyor.h#L9), [conveyor.h:61](../src/conveyor.h#L61)

Static: side frame rails and four legs (`box_span`), the belt's two straight
runs (`box_span`) and two half-shell wraps (`cyl_z`). Per frame: two rollers,
the Geneva wheel, a stub shaft, and 24 cleats.

| Quantity | Formula / value | Where |
|---|---|---|
| belt centreline radius | `BELT_R_C` = 0.40 typed | [config.h:77](../src/config.h#L77) |
| **station pitch** | `p = R_c · π/2` = 0.628319 — derived | [layout.h:112](../src/layout.h#L112) |
| roller span | `10p` — derived | [layout.h:113](../src/layout.h#L113) |
| head roller | `HEAD_X` = 4.000 typed | [config.h:75](../src/config.h#L75) |
| tail roller | `HEAD_X − 10p` = −2.283185 — derived | [layout.h:114](../src/layout.h#L114) |
| loop length | `24p` — derived | [layout.h:115](../src/layout.h#L115) |
| station *j* | `tail_x + j·p` — derived | [layout.h:116](../src/layout.h#L116) |
| cleat *k* | at `(k + ½)·p` along the loop | [layout.h:146](../src/layout.h#L146) |
| roller / wheel angle | `−90°·B` | [layout.h:109](../src/layout.h#L109) |

**Why the pitch is what it is.** `p = R_c·π/2` is exactly the arc a quarter
turn of the roller drags. That single choice is what makes one Geneva index
advance the belt exactly one station — it is not tuned, it is construction.
`CLEAT_N = 24` follows the same way: the loop is `2·(10p) + 2πR_c = 24p`.

Cleats sit half a pitch off the stations so a cleat never overlaps a blank.

**`belt_path(s)`** ([layout.h:121](../src/layout.h#L121)) maps a distance along
the loop to a position and a rotation, in four pieces: top run, head wrap,
bottom run, tail wrap. It is what puts cleats correctly round the roller ends.

### 4.7 Belt travel — [layout.h:91](../src/layout.h#L91), [layout.h:97](../src/layout.h#L97)

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

### 4.8 Blanks — `build_blanks` / `draw_blanks` [blanks.h:10](../src/blanks.h#L10), [blanks.h:29](../src/blanks.h#L29)

One `cyl` list, drawn nine times. `BLANK_R` = 0.18, `BLANK_H` = 0.20,
`BLANK_H_FLAT` = 0.10, all typed.

Position: slot *j* sits at `station_x(j) + frac(B)·p`. When `B` passes a whole
number every blank has moved up one station, so the blank drawn at slot *j*
takes over the place slot *j−1* just left — one leaves at the head roller, one
arrives from the magazine, and nothing in between appears to move.

Height is read straight off where the blank stands and where the punch is,
with no per-blank state at all ([layout.h:148](../src/layout.h#L148)):

```
x < PRESS_X  →  BLANK_H           not yet stamped
x > PRESS_X  →  BLANK_H_FLAT      already stamped
otherwise    →  clamp(punch_face(th) − BELT_TOP_Y, BLANK_H_FLAT, BLANK_H)
```

The squash is `glScalef(1, h/BLANK_H, 1)` about the blank's base. It is the
only `glScalef` in the machine, and it is a non-uniform scale, which is why
`GL_NORMALIZE` is on.

### 4.9 Feed magazine and exit hood — `build_fixtures` [fixtures.h:9](../src/fixtures.h#L9)

Magazine: two `box_span` plates either side of the belt at station 1, blanks
notionally dropping between them. `MAG_W` = 0.50, `MAG_WALL` = 0.06, typed.

Exit hood: one `box_span` plate over the belt, open all round underneath.
`HOOD_X0/X1` = 3.67 / 4.45, typed — 0.30 past station 9.

> The discharge chute and collection bin were removed: the belt recycles its
> blanks along the top run rather than carrying them over the roller, so both
> stood permanently empty.

### 4.10 Stack light — `draw_stack_light` [fixtures.h:55](../src/fixtures.h#L55)

A post (`cyl`), a housing (`cyl`), and three lens segments from one list. Green,
amber, red upward. `STACK_LIT = 0` ([layout.h:154](../src/layout.h#L154)) picks
the lit one — green, fixed.

The lenses are drawn with **lighting disabled**: a lamp makes light, it does
not catch it. `mat::lens()` ([materials.h](../src/materials.h)) gives the full
colour when lit and a dim version when not.

---

## 5. The room, object by object

All in [room.h](../src/room.h). The room is not in the PRD; it is a deviation,
recorded in the README.

**The cutaway.** A wall, and everything mounted on it, is drawn only while the
eye is on the room side of that wall's plane
(`wall_shown`, [room.h:41](../src/room.h#L41)). So the walls between the camera
and the machine vanish as it orbits and the far ones stay. Back-face culling
alone would hide the bare wall surfaces, but not the boxes fixed to them — a
window frame would float in front of the machine.

**Wall frames.** Each wall is built in its own frame: `u` runs along the wall
left to right as seen from inside, `y` is up, `z` points out of the wall into
the room (`enter_wall`, [room.h:26](../src/room.h#L26)). One `window()` routine
then serves all four walls. `back_u`/`left_u`/`right_u`/`front_u`
([room.h:36](../src/room.h#L36)) convert a world coordinate into that wall's `u`.

| Object | Built from | Line | Key numbers |
|---|---|---|---|
| Floor | `tiles_y`, 15 × 8 cells of 1.0 | [fixtures.h:13](../src/fixtures.h#L13) | `FLOOR_X0/X1/Z0/Z1` typed |
| Wall surface | two `tiles_z` bands + a trim `box_span` | [room.h:50](../src/room.h#L50) | `DADO_H` 1.20, `CEIL_Y` 9.5 typed |
| Window ×11 | one `tiles_z` pane + four `box_span` frame bars | [room.h:61](../src/room.h#L61) | `WIN_W` 1.80, `WIN_H` 1.80 typed |
| Ceiling | `tiles_y` facing **down** | [room.h:126](../src/room.h#L126) | — |
| I-beams ×5 | three `box_span` each (flange, web, flange) | [room.h:134](../src/room.h#L134) | `BEAM_X0` −3.90, `BEAM_PITCH` 3.00 |
| Pendant flex ×2 | thin `cyl` from ceiling to bulb | [room.h:142](../src/room.h#L142) | `BULB_X[2]`, `BULB_Y` 5.40 |
| Bulb globe ×2 | `cyl` radius `BULB_R` | [room.h:153](../src/room.h#L153) | drawn unlit; follows its switch |
| Hazard border | four `tiles_y` strips, 0.004 proud | [room.h:166](../src/room.h#L166) | `HAZ_*` typed |
| Pallet | 3 bearers + 5 deck boards + 9 blanks | [room.h:175](../src/room.h#L175) | `PAL_X/Z` typed; reuses `L_BLANK` |
| Cabinet | one `box_span` + four switch plates | [room.h:197](../src/room.h#L197) | `CAB_X0/X1/H/D` typed |
| Drums ×2 | `cyl` | [room.h:207](../src/room.h#L207) | `DRUM_R` 0.30, `DRUM_H` 0.88 |
| Spare-gear shelf | `box_span` shelf + 2 brackets + 2 gears | [room.h:110](../src/room.h#L110) | reuses `L_GEAR0`, `L_TOOTH` |
| Exhaust fan | housing (4 bars + grille) + 6-blade rotor | [room.h:89](../src/room.h#L89), [room.h:217](../src/room.h#L217) | `FAN_BLADES` 6 |
| Switch lamps ×4 | `cyl_z`, drawn unlit | [room.h:276](../src/room.h#L276) | colours shared with the HUD |

Two objects deliberately **reuse the machine's own display lists**, so they are
exactly the parts they represent: the spare gears on the shelf are built from
`L_GEAR0` and `L_TOOTH`, and the pallet is stacked with `L_BLANK`. That is why
`room::build_lists()` has to run *after* `scene::build_lists()`
([main.cpp:162](../src/main.cpp#L162)).

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
colours, `BLANK_R`, panel and motor sizes, magazine and hood.

**Coupled — read this first.**

| Number | What depends on it |
|---|---|
| `TEETH`, `MODULE` | every pitch radius, and therefore **every gear centre**. Two meshing gears must sit exactly `r_i + r_j` apart. |
| `G3_X/Y`, `G4_X/Y` ([config.h:45](../src/config.h#L45)) | **solved**, not typed by choice — see the construction below. All four meshes currently sit at their exact pitch-radius sums. |
| `GEAR_RATE` signs | not free. The belt must feed towards the head roller → fixes the wheel's direction → fixes the driver's → fixes G5's → and G2 is three meshes back. |
| `BELT_R_C` | the station pitch, and so every station position, the tail roller, the loop length and `CLEAT_N`. |
| `GEN_A`, `GEN_SLOTS` | the centre distance and wheel radius, and therefore where the head roller must sit relative to G5. They are currently 0.7778 apart, exactly `c`. |
| `CRANK_R`, `ROD_L`, `RAM_H` | the stroke, and the punch gap at the bottom — which is what makes a stamped blank exactly `BLANK_H_FLAT` tall. |
| `POSE_DEG` (90°) | `PHI_DEG` was solved at this angle. Changing it rotates the whole train out of its solved pose. |
| `PRESS_STATION` / `PRESS_X` | station 7 is at exactly `PRESS_X`. The blank height rule compares against `PRESS_X`. |

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
