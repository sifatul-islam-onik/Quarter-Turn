# Quarter Turn

An automated stamping line in OpenGL — CSE 4207 Computer Graphics, KUET.

> **Branch `static-objects`: the objects only, and nothing moves.** The machine
> is held at one crank angle (`theta` = 90°) and drawn there; the clock that
> advanced it is gone, so each mechanism can be pointed at and named while it
> stands still. This branch is `unlit-demo` with the motion removed, so there is
> no lighting or shading either — flat colours plus an edge pass. The running
> machine is on `unlit-demo`; lighting, materials and the three shading modes
> are on `main`.

A motor drives a train of five meshing brass gears. The largest carries a crank,
and a crank-slider drives a press ram over a conveyor. The last gear turns a
Geneva mechanism on the conveyor's head roller, converting continuous rotation
into exactly one quarter turn of the roller followed by a locked pause. Each
quarter turn advances the belt one station: blanks drop from a magazine, ride the
belt, stop under the press, are flattened while the belt is locked, pass under
an exit hood, then tip off the head roller, slide down a chute and pile up in a
bin. The line stands in a cutaway workshop (walls, windows, a
roll-up door, a workbench, pipework, ceiling beams), whose near walls drop away
as the camera orbits. Two bulbs hang over the line, and the bulbs, the exhaust
fan and the machine each have their own switch.

**The technical claim, and what this branch shows about it:** the machine's
entire animation state is one accumulating float (`theta`, the crankshaft
angle) and one integer (`cycles`). Every motion it makes — five meshing gears,
the press, the intermittent belt and the parts it carries — is a closed-form
function of those two. No keyframes, no stored poses, no interpolation.

Because of that, taking the animation out is not a rewrite: `theta` and
`cycles` become two constants and the render loop stops advancing them. Every
part is still placed by the same functions, so the line is caught mid-cycle
rather than collapsed into a neutral pose. `src/config.h`, `src/kinematics.h`,
`src/prim.h`, `src/materials.h`, `src/scene.h` and `src/room.h` are byte for
byte the ones on `unlit-demo`; only `src/main.cpp` and this file differ.

The pose is `theta` = 90°: the belt is locked between indexes, the ram is half
way down its stroke and clear of the blanks, and the Geneva driver's pin is
outside its slot.

## Build and run

Requires MSYS2 MinGW64 with `glew` and `freeglut`.

```powershell
.\build.ps1          # build
.\build.ps1 -Run     # build and run
.\build.ps1 -Check   # headless verification of every derived number
.\build.ps1 -Release # -O2, assertions off
```

or, from the MSYS2 shell: `make`, `make run`, `make check`, `make release`.

## Controls

The camera is the only thing in this build that moves.

| Key | Action |
|---|---|
| `←` `→` | orbit the camera |
| `1` `2` `3` `4` | three-quarter / front elevation / top plan / whole room |
| `c` | free camera on / off, starting from the current view |
| free camera: `↑` `↓` `←` `→` | fly forward / back along the view, strafe left / right |
| free camera: `PgUp` `PgDn` | rise / sink |
| free camera: left-drag | look around |
| `r` | put the camera back on view 1 |
| `h` | show / hide the technical readout of the pose |
| `e` | edge lines on / off |
| `w` | wireframe |
| `Space` | machine switch — throws its lever and lights its lens; starts nothing |
| `[` `]` | left / right bulb on / off |
| `f` | exhaust fan switch — its lens lights, but the blades stay put |
| `Esc` | quit |

Gone with the animation: `.` (step the crank), `↑` `↓` and `+` `-` (line speed).

## What to demonstrate

**1. The objects, view by view.** `1` is the three-quarter view of the whole
line: the drive panel with its five gears on the left, the press above the
conveyor, the conveyor running to the right, the head roller with the Geneva
wheel, the chute and the bin. `2` puts the gear train and the press stack
square to the camera. `3` looks down on the belt, the nine blanks along it and
the stations they sit on. `4` pulls back to the workshop — walls, windows, the
roll-up door, the bench, the pipework, the ceiling beams, the two hanging
bulbs, the exhaust fan and the switch cabinet.

**2. What the pose is showing.** At `theta` = 90° the ram is half way down and
clear of the blanks, the belt is locked between indexes, and the Geneva driver's
pin sits outside the slot. Every blank on the belt is drawn at the height its
own station gives it, so the stamped ones downstream of the press are visibly
flatter than the ones still waiting: station 7 is where the press works, and the
squash is a volume-preserving scale, not a different model.

**3. The gear train, standing still.** Each of the five gears sits exactly at
the sum of its and its neighbour's pitch radius, and at every mesh a tooth on
one gear faces a gap on the other — that half-tooth offset is `pi/N` in
`kin::gear_angles()`, and without it the teeth would interpenetrate at rest.
The teeth counts are 12:36:20:20:36 (`TEETH` in `config.h`); the readout (`h`)
prints them. Brass and copper alternate along the train, so each mesh is
between two colours.

**4. The hierarchy, in wireframe (`w`).** The crank disc sits on its shaft, the
rod runs from the crank pin to the wrist pin, and the ram sits between its
rails — a chain of `glPushMatrix` / `glTranslatef` / `glRotatef`, one child per
level. Every gear is a child of the drive panel, not of the gear that drives
it: a child would inherit its parent's rotation, but a meshing gear turns the
other way at a different rate, so each gear's angle is computed and applied on
its own.

**5. Why the edges are there (`e`).** Without lighting, every face of a part is
the same colour. Press `e`: the drive panel, the conveyor frame and the bin
merge into one flat green shape, and with nothing moving there is no other cue
left to separate them. The edge pass draws the whole scene a second time as
lines (`glPolygonMode(GL_LINE)`).
Three details make it work:

- `glPolygonOffset` pushes the faces back, so a line does not fight its own face in the depth buffer.
- A blend of `GL_ZERO, GL_CONSTANT_COLOR` darkens the colour already in the framebuffer, because the display lists set their own colours.
- Back-face culling and the depth test still apply to lines, so hidden edges stay hidden.

**6. Viewing.** `1`–`4` are four `gluLookAt` presets, and `←` `→` orbit the
eye about the look-at point. The walls between the camera and the machine
disappear as it orbits. `c` hands the same eye to a free camera, whose view
direction comes from a yaw and a pitch, which is the way to get in close to one
mechanism while talking about it.

**7. The numbers behind the pose (`h`).** The readout is the pose's own
figures, not a running log: the crank angle and its phase, the punch face
height against the top of a blank and the clearance between them, the Geneva
wheel's angle, belt travel in stations and how far through an index the line
is. They are what `kinematics.h` returns for `theta` = 90°, and
`build\mathcheck.exe` checks the same functions across the whole cycle.

*For the line actually running — the gears turning, the press stroking, the belt
indexing one station per stroke and the parts piling into the bin — check out
`unlit-demo`.*

## Verification

`build\mathcheck.exe` includes the same `src/kinematics.h` the renderer does, so
it checks the shipping code rather than a transcription. 62 checks covering the
belt pitch and loop closure, the press stack, the analytic blank-zone root
against a 0.01° scan, the Geneva geometry and the pin's position in a slot
throughout the index, belt-travel monotonicity across both 45° and the wrap, all
four gear meshes and tooth-to-gap alignment at every angle, volume preservation
under the squash, and every clearance. For the finished parts it runs eight
revolutions in 0.02° steps and checks that the stroke clock and a part's whole
path are continuous and that the part arrives where the belt's last blank stops,
clears the blank behind it, lands exactly on its slot, and lands one per
revolution. It also checks that every drop to every one of the 36 slots stays
inside the bin's walls below the rim, and that the full pile fits.

## Deviations from the PRD

Each is in the source next to the number it changes.

**No lighting on this branch.** A material is one flat colour set with
`glColor` (`materials.h`), recorded into the display lists like any other call.
The consequences:

- The surfaces that were split into cells for per-vertex lighting are one cell now. The exception is the floor, which is kept as 1.0 tiles because the tiles show the floor receding in perspective (`config.h`).
- Cylinder caps are one `GL_POLYGON` instead of a triangle fan, so outlined they show a rim, not spokes (`prim.h`).
- The Geneva wheel and the belt's wrap are outlined with `glEdgeFlag`, so they show their shape rather than every sample (`prim.h`).
- The rod and ram are darker steel, the gears alternate brass and copper, and the cleats are yellow, so parts that meet do not share a colour.
- The bulbs glow but light nothing, and the window glass is a pale daylight colour.

**The polished parts have a 45° chamfer** (`prim.h`, `cyl()`), as real stamped
parts have.

**The Geneva wheel sits at z 0.66–0.74** rather than the PRD's 0.62 centre
(`config.h`). The near conveyor frame rail also has to fit between the roller
end cap and the wheel, and it occupies 0.51–0.59. The wheel moves outboard of
it and the roller reaches it through a stub shaft; it stays 0.07 clear of the
nearest same-facing surface.

**The Geneva slot bottom is 0.175, not 0.20** (`scene.h`). The PRD's figure
assumes a point pin, whose centre reaches 0.2278. A pin of radius 0.045 reaches
0.183, so the slot is cut deeper and the pin clears the bottom by 0.008.

**The rail bracket is two arms**, one per guide rail (`scene.h`), so the ram
passes between them instead of through them.

**Two hanging bulbs** (`config.h`, `room.h`). Each is a glass bulb on a flex,
drawn with an additive halo while it is on, and each has its own switch. They
hang either side of the press, 1.0 in front of the belt and 2.4 above it.

**A switch panel and a minimal HUD** (`main.cpp`, `room.h`). The machine, each
bulb and the fan have their own switch, drawn as rockers with status lamps on
the electrical cabinet, in the same colours as the HUD's dots. The HUD is one
small panel (switches, speed, parts) and a key hint; the full technical
readout the demonstrations above refer to is behind `h`.

**There is a room** (`room.h`). The PRD says "no factory building". The line
now stands in a 15 × 8 × 9.5 workshop. Each wall, and everything fixed to it, is
drawn only while the eye is on the room side of that wall, so the walls nearest
the camera vanish at every orbit angle; from above (`3`) the ceiling and beams
go too. Back-face culling alone hides a bare inward-facing wall but not the
boxes mounted on it. Consequences elsewhere:

- The **far plane is 40**, not 25: in the overview preset the furthest room corner
  is at a depth of 29, and 30 at worst while orbiting. far/near is 10.

**A free camera** (`c`, `main.cpp`). It flies right up to the parts, so it
switches the near plane from 4.0 to 0.2 while it is on, and far/near becomes
200. To keep that safe it stays within 22 of the room's centre and below a
height of 20. Every room corner then stays within a depth of 36.5, and a 24-bit
depth step there is 0.0004, so the hazard marks, 0.004 above the floor, stay
about ten steps clear. It moves at speed × `dt` from held keys, not on key
repeat, so it stays frame-rate independent like the machine. Walls hide the
same way as in the presets: fly out through a wall and it disappears.
- The **floor grows** to the room, in 1.0 tiles (15 × 8).
- **Eight room colours** are added (wall paint, safety yellow, wood, signal
  red, galvanized steel, window glass, and a bulb lit and unlit).
- The **parts counter** is closed-form in `theta` and `cycles` and reads
  `kin::parts_made()`, the same function the HUD uses. The **fan** is not: with
  its own switch it keeps its own speed and angle, and closes on its target
  speed with a 0.8 s first-order lag.
- The spare gears on the shelf and the pallet's blanks call the machine's own
  display lists. The stamped part on the bench is built flat at the squashed
  radius, so the squash is still the only `glScalef`.

**Finished parts leave the line instead of vanishing** (`kinematics.h`,
`exit_pose()`). The PRD's hood had a closed far end, and a part was removed
out of sight under it at the end of each index. The far end is now open. A
part rests on top of the head roller while the belt is locked, rides the next
index over the roller to 40° of wrap, and is tossed onto the chute. It lands as
the index ends, slides down, and drops into the bin. The exit runs on a
*stroke clock*, the count of finished indices plus the fraction of a revolution
since the last one, so it is closed-form in `theta` and `cycles` like the belt
and scales with the speed. The toss follows the roller's tangent and the drop
leaves the chute at the slide's speed, so neither kinks. The bin piles parts in
2 × 2 columns, 36 in all. Once it is full, each new part aims at the top slot,
which is already drawn, so it lands on the pile without a second copy ever
showing. `r` empties the bin with everything else.

**The chute is short and held to the near side** (`config.h`). At full size it
drew straight across the Geneva drive gear; it is 0.85 in front of the gear
plane, so this is composition, not collision.

## Files

| File | Contents |
|---|---|
| `src/config.h` | every tunable number, PRD-section referenced |
| `src/kinematics.h` | all closed-form motion — no OpenGL, shared with the test |
| `src/prim.h` | the one box routine and one cylinder routine |
| `src/materials.h` | one flat colour per material, for the machine and the room |
| `src/scene.h` | display lists and the per-frame hierarchy |
| `src/room.h` | the cutaway workshop: walls, ceiling, bulbs and glow, cabinet switches, fan, counter |
| `src/main.cpp` | GLUT glue, the fixed pose and the switches, input, HUD, the fill and edge passes |
| `tests/mathcheck.cpp` | headless verification of PRD §10 |
| `PRD.md` | the requirements document this implements |
