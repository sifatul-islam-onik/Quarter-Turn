# Quarter Turn

An automated stamping line in OpenGL — CSE 4207 Computer Graphics, KUET.

> **Branch `unlit-demo`: no lighting and no shading.** Every part is drawn in a
> flat colour, and a second pass outlines the geometry so the shapes still
> read. This is the build for the modelling, transformation, animation and
> viewing demo. Lighting, materials and the three shading modes are on `main`.

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

**The technical claim:** the machine's entire animation state is one
accumulating float (`theta`, the crankshaft angle) and one integer (`cycles`).
Every motion it makes — five meshing gears, the press, the intermittent belt and
the parts it carries — is a closed-form function of those two, evaluated in the
render loop. No keyframes, no stored poses, no interpolation. (The room's fan
has its own switch, so it keeps its own angle; it is the one exception.)

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

| Key | Action |
|---|---|
| `Space` | machine on / off |
| `[` `]` | left / right bulb on / off |
| `f` | exhaust fan on / off (spins up and runs down) |
| `h` | show / hide the technical readout |
| `.` | while the machine is off, step `theta` forward 5° |
| `↑` `↓` | speed ±6 ppm, range 6–120 (also `+` `-`, in every view) |
| `←` `→` | orbit the camera |
| `1` `2` `3` `4` | three-quarter / front elevation / top plan / whole room |
| `c` | free camera on / off, starting from the current view |
| free camera: `↑` `↓` `←` `→` | fly forward / back along the view, strafe left / right |
| free camera: `PgUp` `PgDn` | rise / sink |
| free camera: left-drag | look around |
| `e` | edge lines on / off |
| `w` | wireframe |
| `r` | reset to `theta` = 90°, `cycles` = 0 |
| `Esc` | quit |

## What to demonstrate

**1. The motion is computed, not authored.** Hold `↑` to 120 ppm and back. Every
gear stays meshed, the press stays in step, and the belt still advances exactly
one station per stroke — because belt travel is derived from `theta`, not from
elapsed time. The interlock line in the readout (`h`) asserts, live, that belt travel is
strictly constant whenever the punch is below the top of an unstamped blank.

**2. The interlock is derived, not chosen.** Switch the machine off with
`Space` and step with `.`. The belt
only moves during INDEX (315°–45°); the punch is inside the blank zone for
132.03°–227.97°, a root computed in `kin::blank_zone_cos()` rather than typed in.
Change the crank throw, the rod, the ram or the belt height and the window moves
with them.

**3. Temporal aliasing.** Set the speed nearest `fps / 0.6` and all five gears
appear to stand still while the crank, ram and belt keep moving. They freeze at
once because every gear in the train passes teeth at the same rate. The readout
(`h`) prints the speed at which this happens for the current frame rate.

**4. The hierarchy, in wireframe (`w`).** Stop the machine with `Space` and
step with `.`. The crank disc turns about its shaft, the rod follows the pin,
and the ram slides between its rails. Every gear is a child of the drive panel,
not of the gear that drives it: a child inherits its parent's rotation, but a
meshing gear turns the other way at a different rate. The brass and copper
alternate along the train, so each mesh is between two colours.

**5. Why the edges are there (`e`).** Without lighting, every face of a part is
the same colour. Press `e`: the drive panel, the conveyor frame and the bin
merge into one flat green shape, and a turning roller looks still. The edge
pass draws the whole scene a second time as lines (`glPolygonMode(GL_LINE)`).
Three details make it work:

- `glPolygonOffset` pushes the faces back, so a line does not fight its own face in the depth buffer.
- A blend of `GL_ZERO, GL_CONSTANT_COLOR` darkens the colour already in the framebuffer, because the display lists set their own colours.
- Back-face culling and the depth test still apply to lines, so hidden edges stay hidden.

**6. Viewing.** `1`–`4` are four `gluLookAt` presets, and `←` `→` orbit the
eye about the look-at point. The walls between the camera and the machine
disappear as it orbits. `c` hands the same eye to a free camera, whose view
direction comes from a yaw and a pitch.

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
| `src/main.cpp` | GLUT glue, state and switches, input, HUD, the fill and edge passes |
| `tests/mathcheck.cpp` | headless verification of PRD §10 |
| `PRD.md` | the requirements document this implements |
