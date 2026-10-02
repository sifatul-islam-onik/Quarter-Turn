| `src/prim.h` | the box and cylinder routines, and `new_list()` for display lists |
| `src/materials.h` | colour, specular and exponent per material; `glow()` for emission |
| `src/common.h` | the includes every machine file shares |
| `src/gears.h` | the five-gear train (FR-3) |
| `src/press.h` | the drive panel and the crank-slider press (FR-4) |
| `src/geneva.h` | the driver arm and the four-slot wheel (FR-5) |
| `src/conveyor.h` | frame, rollers, belt, cleats (FR-6) |
| `src/blanks.h` | the workpieces and the squash (FR-7) |
| `src/fixtures.h` | floor, motor, stack light |
| `src/scene.h` | assembles those six into the machine: build order, draw order |
| `src/room.h` | the cutaway workshop: walls, windows, beams, lamps, pallets, drums, cabinet switches, fan |
| `src/camera.h` | two preset views, orbit, the level free camera |
| `src/hud.h` | the switch panel and the key hint |
| `src/shading.h` | flat, Gouraud and the per-pixel Phong program (GLSL 1.10) |
| `src/main.cpp` | GLUT glue, GLEW, the clock, the spotlights, input, the fill and edge passes |
# Quarter Turn

An automated stamping line in OpenGL — CSE 4207 Computer Graphics, KUET.

> **Branch `static-objects`: the minimal build.** The machine runs and the room
> is lit, both in the smallest form that is still correct. The whole machine is
> a function of one angle; the lighting is the slides' Phong reflection model
> (ambient, diffuse, specular, emission) from two attenuated spotlights, shaded
> flat, Gouraud or per-pixel Phong (a small GLSL program).
>
> The branch name is now a misnomer — it started as the objects on their own.

A motor drives a train of five meshing brass gears. The largest carries a crank,
and a crank-slider drives a press ram over a conveyor. The last gear turns a
Geneva mechanism on the conveyor's head roller, converting continuous rotation
into exactly one quarter turn of the roller followed by a locked pause. Each
quarter turn advances the belt one station: blanks enter at the tail end, ride
the belt, stop under the press, are flattened while the belt is locked, and
leave at the head roller. The line stands in a cutaway
workshop (walls, windows, ceiling
beams), whose near walls drop away as the camera orbits. Two bulbs hang over
the line, and the bulbs, the exhaust fan and the machine each have their own
switch.

**How it moves.** The whole machine is a direct function of one angle,
`theta`, the crank angle, plus a count of the whole revolutions it has
completed. Nothing else holds a position: no part keeps an angle of its own, so
no part can drift out of step with another however long it runs. `src/layout.h`
holds every formula, and `main.cpp` does nothing per frame but add
`CRANK_DPS * dt` to `theta`.

- **the gear train** — each gear has a fixed rate against the crank, because at
  a mesh the next gear reverses and scales by the inverse tooth ratio;
- **the press** — the slider law, so the stroke is exactly twice the throw;
- **the Geneva drive** — the slot-follows-pin law, which gives the quarter turn
  the project is named after, and locks the belt for the rest of the
  revolution;
- **the belt and the blanks** — belt travel falls straight out of the wheel
  angle, because a quarter turn of the wheel is one belt pitch by construction;
  a blank's height is read off where it stands and where the punch is.

One crank revolution is one part: the belt indexes one station while the ram is
at the top, then the ram comes down on a blank the Geneva lock is holding still.

**How it is lit.** Each hanging bulb is one positional `GL_LIGHT`, switched by
the same flag that draws its glass lit or dark, over an ambient floor that
keeps the far corners off black. Each is a spotlight pointing straight down,
cut off at 90° by its flat shade and fading as `cos^0.5` towards that edge, so
nothing above a bulb is lit by it and the upper walls and the ceiling get the
ambient light alone. Its
light is also divided by `a0 + a1·d + a2·d²` with distance, so the walls far
from a bulb get less of it than the machine under it. A material
(`materials.h`) is a colour plus the
specular colour `ks` and exponent `ns`; brass, polished silver, copper and
black plastic take theirs from the slides' coefficient table.
`GL_COLOR_MATERIAL` turns the colour into ambient and diffuse, and
`glMaterial` sets the specular term. Lit bulbs, lamp lenses and window
daylight use the emission term instead (`mat::glow`), so they shine whatever
the lamps do.

**How it is shaded** (`s`, `shading.h`). Flat and Gouraud are the fixed
pipeline, which lights each vertex. Phong, the default, is a GLSL 1.10 program
that passes the normal to every pixel and lights it there. It works out the
same equation from the same lights and materials (`gl_LightSource`,
`gl_FrontMaterial`), so switching modes changes only *where* the lighting is
worked out. The difference shows where light changes inside one face: the
spotlight's edge crossing a wall tile, or a highlight on the drive panel.

## Build and run

Requires MSYS2 MinGW64 with `freeglut` and `glew`. Windows' `opengl32.dll`
stops at OpenGL 1.1; GLEW finds the OpenGL 2.0 shader functions in the driver.
Without OpenGL 2.0 the program is skipped and `s` cycles flat and Gouraud only.

```powershell
.\build.ps1          # build
.\build.ps1 -Run     # build and run
.\build.ps1 -Release # -O2, assertions off
```

or, from the MSYS2 shell: `make`, `make run`, `make release`.

## Documentation

| Document | What it is for |
|---|---|
| [docs/OBJECTS.md](docs/OBJECTS.md) | every object: what it is built from, and whether each number was **typed**, **derived** or **solved**. Has every formula in one page, and a list of which numbers are safe to change and which are coupled. |
| [docs/DEMO-CHANGES.md](docs/DEMO-CHANGES.md) | changing things live: moving an object, reshaping one, colours, speed, lighting, adding a new object, and the five things that bite. |
| [docs/HOW-IT-WORKS.md](docs/HOW-IT-WORKS.md) | the long-form original. **Describes `unlit-demo`** — its derivations still hold, its file and line references do not. Its header lists the differences. |
| [docs/PROPOSAL.md](docs/PROPOSAL.md) | the original project proposal. |

## Controls


| Key | Action |
|---|---|
| `←` `→` | orbit the camera |
| `1` `2` | three-quarter view of the line / the whole room |
| `c` | free camera on / off, starting from the current view |
| free camera: `↑` `↓` | fly forward / back |
| free camera: `←` `→` | turn left / right |
| free camera: `PgUp` `PgDn` | rise / sink |
| `r` | reset: crank back to the start, camera back on view 1 |
| `e` | edge lines on / off |
| `s` | shading: flat → Gouraud → Phong (per pixel) |
| `Space` | machine switch — starts and stops the line |
| `[` `]` | left / right bulb on / off |
| `f` | exhaust fan switch |
| `Esc` | quit |

Not on this branch: `.` (step the crank one frame) and `↑` `↓` `+` `-` (line
speed). The line runs at one fixed speed, `CRANK_DPS` in `config.h`.

## What to demonstrate

**1. The objects, view by view.** `1` is the three-quarter view of the whole
line: the drive panel with its five gears on the left, the press above the
conveyor, the conveyor running to the right, and the head roller with the
Geneva wheel. `2` pulls back to the workshop — walls, windows,
the ceiling beams, the two hanging bulbs, the exhaust fan and the switch
cabinet.

**2. What the pose is showing.** The ram is part way down and clear of the
blanks, the belt is parked between indexes, and the Geneva driver's pin sits
outside the slot, which is what locks the wheel. Every blank on the belt is drawn at the height its
own station gives it, so the stamped ones downstream of the press are visibly
flatter than the ones still waiting: station 7 is where the press works, and the
squash is a volume-preserving scale, not a different model.

**3. The gear train, standing still.** Each of the five gears sits exactly at
the sum of its and its neighbour's pitch radius, and at every mesh a tooth on
one gear faces a gap on the other — that half-tooth offset is `pi/N` in
`lay::mesh_angles()`, and without it the teeth would interpenetrate where they
stand. The teeth counts are 12:36:20:20:36 (`TEETH` in `config.h`). Brass and
copper
alternate along the train, so each mesh is between two colours.

**4. The hierarchy.** The crank disc sits on its shaft, the
rod runs from the crank pin to the wrist pin, and the ram sits between its
rails — a chain of `glPushMatrix` / `glTranslatef` / `glRotatef`, one child per
level. Every gear is a child of the drive panel, not of the gear that drives
it: a child would inherit its parent's rotation, but a meshing gear turns the
other way at a different rate, so each gear's angle is computed and applied on
its own.

**5. Why the edges are there (`e`).** Shading separates most surfaces, but two
painted parts meeting at a shallow angle still read as one shape. Press `e`:
the drive panel, the conveyor frame and its legs flatten into each other. The
edge pass draws the whole scene a second time as lines
(`glPolygonMode(GL_LINE)`).
Three details make it work:

- `glPolygonOffset` pushes the faces back, so a line does not fight its own face in the depth buffer.
- A blend of `GL_ZERO, GL_SRC_COLOR` multiplies the pixel already in the framebuffer by the line's own colour. The display lists set their own colours, so the line's colour *is* the face's colour, and every edge comes out as that face's colour squared — a darker shade of the same colour.
- Back-face culling and the depth test still apply to lines, so hidden edges stay hidden.

**6. Viewing.** `1` and `2` are two `gluLookAt` presets, and `←` `→` orbit the
eye about the look-at point. The walls between the camera and the machine
disappear as it orbits. `c` hands the same eye to a free camera, which flies
level: its view direction is one yaw angle, the arrows fly it and turn it, and
that is the way to get in close to one mechanism while talking about it.

**7. The numbers behind the pose.** They are all in `layout.h`, worked out at
startup, and nothing changes them afterwards: the five gear angles, the derived
centres of G3 and G4, the ram and punch heights against the top of a blank, the
belt pitch and its first station, and the Geneva wheel radius and centre
distance. Read them there rather than on screen - this branch prints nothing
and has no readout panel.

## Verification

`tests/mathcheck.cpp` and the startup `verify_layout()` checks are not on this
branch; they are on `unlit-demo` and on `main`, unchanged. Between them they
re-prove the rod obliquity, the tail roller position, the Geneva centre
distance and wheel radius, the four meshes at their pitch-radius sums and the
punch clearance.

The motion here was checked separately over ten crank revolutions: belt travel
never runs backwards, ten revolutions give exactly ten indexes, the wheel turns
90 degrees per index, the belt advances exactly one pitch per index, the ram
stroke is exactly twice the throw, the punch bottoms out at exactly the flat
blank's height, and the punch never comes down while the belt is indexing.

## Deviations from the PRD

Each is in the source next to the number it changes.

**Coarse geometry, made for the edge pass.** A material is a colour set with
`glColor`, plus `ks` and `ns` set with `glMaterial` (`materials.h`), recorded
into the display lists like any other call. The geometry is kept coarse
because the edge pass outlines every face. That suits Phong, which lights per
pixel, but not Gouraud, which lights per vertex. The consequences:

- The drive panel and the belt strip are plain boxes, so their faces have only four vertices. Under Gouraud a highlight that lands on one corner smears across the whole face, which is why the machine paint is semi-gloss (`ks` 0.10). The floor, walls and ceiling are 1.0 tiles, so even under Gouraud the light falls off across them, though the fade at the spotlight's edge is smeared over a whole row of tiles (`config.h`).
- Cylinder caps are one `GL_POLYGON` instead of a triangle fan, so outlined they show a rim, not spokes (`prim.h`).
- Every object is a box or a cylinder (`prim.h` has `box`, `cyl` and the two tile grids and nothing else), and every cylinder has the same `CYL_SLICES` sides. The Geneva wheel is a hub with four box arms, and the gaps between them are its four slots. Gear teeth are sunk `TOOTH_SINK` into their body so a coarse body still carries them.
- The rod and ram are darker steel, the gears alternate brass and copper, and the cleats are yellow, so parts that meet do not share a colour.
- Brass, copper, polished silver and black plastic take `ks` and `ns` from the slides' table, but keep this branch's brighter colours for ambient and diffuse. The slide's silver diffuse is 0.28, which would make the blanks almost black away from a highlight.

**Cylinder ends are square** (`prim.h`, `cyl()`). Earlier builds broke them with
a 45° chamfer, as real stamped parts have; unlit and outlined it cost two extra
quad strips per part and read as one more line, so it went.

**The Geneva wheel sits at z 0.66–0.74** rather than the PRD's 0.62 centre
(`config.h`). The near conveyor frame rail also has to fit between the roller
end cap and the wheel, and it occupies 0.51–0.59. The wheel moves outboard of
it and the roller reaches it through a stub shaft; it stays 0.07 clear of the
nearest same-facing surface.

**The Geneva wheel is four box arms on a hub** (`geneva.h`), so its slots are
the gaps between the arms rather than channels cut into a disc. They are
wedge-shaped rather than parallel-sided, and the hub at 0.175 is what closes
their bottoms. The pin sits outside a slot in this pose, so nothing depends on
the slot walls being exact.

**The rail bracket is two arms**, one per guide rail (`press.h`), so the ram
passes between them instead of through them.

**Two hanging bulbs** (`config.h`, `room.h`). Each is a cylinder of glass under
a metal shade on a flex, glowing (emission) or dark glass with its own switch.
They hang either side of the press, 1.0 in front of the belt and 3.6 above it.
Each is a spotlight whose cone matches its shade, so the upper walls and the
ceiling get the ambient light alone. They hang that high because a spot cone
cannot be wider than 90°: lower, the cone would miss G1 and the motor, which
stand above the old bulb height of 5.4.

**A switch panel and a minimal HUD** (`main.cpp`, `room.h`). The machine, each
bulb and the fan have their own switch, each a status lamp on the electrical
cabinet lit in the same colour as its HUD dot. The HUD is one
small panel listing the four switches, and a key hint. Nothing else: speed and
a parts count would both be meaningless here.

**There is a room** (`room.h`). The PRD says "no factory building". The line
now stands in a 15 × 8 × 9.5 workshop. Each wall, and everything fixed to it, is
drawn only while the eye is on the room side of that wall, so the walls nearest
the camera vanish at every orbit angle, and the ceiling goes whenever the eye
is above it. Back-face culling alone hides a bare inward-facing wall but not the
boxes mounted on it. Consequences elsewhere:

- The **far plane is 40**, not 25: in the overview preset the furthest room corner
  is at a depth of 29, and 30 at worst while orbiting. far/near is 10.

**A free camera** (`c`, `camera.h`). It flies right up to the parts, so it
switches the near plane from 4.0 to 0.2 while it is on, and far/near becomes
200. To keep that safe it stays within 22 of the room's centre and below a
height of 20. Every room corner then stays within a depth of 36.5, and a 24-bit
depth step there is 0.0004, so the hazard marks, 0.004 above the floor, stay
about ten steps clear. It moves at speed × `dt` from held keys, not on key
repeat, so it is frame-rate independent, like the machine. Walls hide the same way as in the presets: fly out
through a wall and it disappears.
- The **floor grows** to the room, in 1.0 tiles (15 × 8).
- **Eight room colours** are added (wall paint, safety yellow, wood, signal
  red, galvanized steel, window glass, and a bulb lit and unlit).
- The **fan** keeps its own angle, off the machine's clock, and spins while its
  switch is on.
- **Build once, draw many.** A window, an I-beam, a pendant lamp, a pallet and
  a drum are each one display list, drawn nine, five, two, two and two times.
  The drum list sets no colour, so each copy is coloured just before its call.
- The spare gears on the shelf and the blanks on both pallets use the
  machine's own lists and its `draw_blank()`, so they are exactly the parts
  they stand in for. The stamped pallet's blanks get the same squash as those
  on the belt.

**No discharge chute and no collection bin.** On the animated branches a
finished part rides over the head roller, is tossed onto a chute, slides down
and drops into a bin that piles 36. The belt here recycles its blanks along the
top run rather than carrying them over the roller, so both stood empty for the
whole run and have been taken out. The code for the exit is on `unlit-demo`
and `main`.

**No feed magazine and no exit hood.** Both were plain black plates over the
belt that did nothing to the blanks, so they have been taken out.

## Files

| File | Contents |
|---|---|
| `src/config.h` | every number, one section per object, each tagged `[free]` or `[coupled]` |
| `src/layout.h` | where every part stands — no OpenGL, no time, evaluated once |
| `src/prim.h` | the box and cylinder routines, and `new_list()` for display lists |
| `src/materials.h` | colour, specular and exponent per material; `glow()` for emission |
| `src/common.h` | the includes every machine file shares |
| `src/gears.h` | the five-gear train (FR-3) |
| `src/press.h` | the drive panel and the crank-slider press (FR-4) |
| `src/geneva.h` | the driver arm and the four-slot wheel (FR-5) |
| `src/conveyor.h` | frame, rollers, belt, cleats (FR-6) |
| `src/blanks.h` | the workpieces and the squash (FR-7) |
| `src/fixtures.h` | floor, motor, stack light |
| `src/scene.h` | assembles those six into the machine: build order, draw order |
| `src/room.h` | the cutaway workshop: walls, windows, beams, lamps, pallets, drums, cabinet switches, fan |
| `src/camera.h` | two preset views, orbit, the level free camera |
| `src/hud.h` | the switch panel and the key hint |
| `src/shading.h` | flat, Gouraud and the per-pixel Phong program (GLSL 1.10) |
| `src/main.cpp` | GLUT glue, GLEW, the clock, the spotlights, input, the fill and edge passes |
| `PRD.md` | the requirements document this implements |
| `docs/OBJECTS.md` | every object and where its numbers came from |
| `docs/DEMO-CHANGES.md` | how to change things during a demonstration |
