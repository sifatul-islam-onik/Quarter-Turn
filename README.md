# Quarter Turn

An automated stamping line in OpenGL — CSE 4207 Computer Graphics, KUET.

A motor drives a train of five meshing brass gears. The largest carries a crank,
and a crank-slider drives a press ram over a conveyor. The last gear turns a
Geneva mechanism on the conveyor's head roller, converting continuous rotation
into exactly one quarter turn of the roller followed by a locked pause. Each
quarter turn advances the belt one station: blanks drop from a magazine, ride the
belt, stop under the press, are flattened while the belt is locked, and leave
under an exit hood.

**The technical claim:** the entire animation state is one accumulating float
(`theta`, the crankshaft angle) and one integer (`cycles`). Every visible motion
— five meshing gears, the press, the intermittent belt and the parts it carries
— is a closed-form function of those two, evaluated in the render loop. No
keyframes, no stored poses, no interpolation.

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
| `Space` | run / stop |
| `.` | while stopped, step `theta` forward 5° |
| `↑` `↓` | speed ±6 ppm, range 6–120 |
| `←` `→` | orbit the camera |
| `1` `2` `3` | three-quarter / front elevation / top plan |
| `s` | shading mode: flat → Gouraud → Phong |
| `l` | fill light on / off |
| `n` | `GL_NORMALIZE` on / off |
| `w` | wireframe |
| `r` | reset to `theta` = 90°, `cycles` = 0 |
| `Esc` | quit |

## What to demonstrate

**1. The motion is computed, not authored.** Hold `↑` to 120 ppm and back. Every
gear stays meshed, the press stays in step, and the belt still advances exactly
one station per stroke — because belt travel is derived from `theta`, not from
elapsed time. The HUD's interlock line asserts, live, that belt travel is
strictly constant whenever the punch is below the top of an unstamped blank.

**2. The interlock is derived, not chosen.** Stop and step with `.`. The belt
only moves during INDEX (315°–45°); the punch is inside the blank zone for
132.03°–227.97°, a root computed in `kin::blank_zone_cos()` rather than typed in.
Change the crank throw, the rod, the ram or the belt height and the window moves
with them.

**3. Temporal aliasing.** Set the speed nearest `fps / 0.6` and all five gears
appear to stand still while the crank, ram and belt keep moving. They freeze at
once because every gear in the train passes teeth at the same rate. The HUD
prints the speed at which this happens for the current frame rate.

**4. Three shading modes (`s`).** Watch the spotlight's pool on the floor: in
Gouraud it is a blocky patch following the 48×18 floor grid, in Phong a smooth
ellipse with a clean cone edge. Watch a blank's chamfer: Gouraud smears a dull
band across it, Phong resolves a tight white point. Flat shows hard facet
boundaries on the crank disc and gear bodies. The fixed pipeline gives
**Blinn-Phong** `(N·H)^ns`; the shader gives **true Phong** `(V·R)^ns` per
fragment — the HUD names which is on screen.

**5. The normal-transformation error (`n`).** With `GL_NORMALIZE` off the
stamped blanks' tops blow out to white and their rims go dull, while unstamped
blanks on the same belt are unchanged — the comparison sits in a single frame.
The squash is the only `glScalef` in the scene, so nothing else moves.
`GL_RESCALE_NORMAL` would be the wrong fix: it applies one correction factor,
but here the walls need `√q` and the caps `1/q`.

## Verification

`build\mathcheck.exe` includes the same `src/kinematics.h` the renderer does, so
it checks the shipping code rather than a transcription. 51 checks covering the
belt pitch and loop closure, the press stack, the analytic blank-zone root
against a 0.01° scan, the Geneva geometry and the pin's position in a slot
throughout the index, belt-travel monotonicity across both 45° and the wrap, all
four gear meshes and tooth-to-gap alignment at every angle, volume preservation
under the squash, and every clearance.

## Deviations from the PRD

Each is in the source next to the number it changes.

**The polished parts have a 45° chamfer** (`prim.h`, `cyl()`). This is the one
that matters. Every surface normal in the scene otherwise lies in an axis plane
— boxes face along the axes, and a cylinder's wall normals stay in the plane
normal to its axis. The half vector between the press lamp (69° above the die)
and the default camera (16° above the belt) sits at about 43°, so **nothing
faced it**, and the mandatory specular highlight evaluated to about 10⁻¹²
everywhere at ns 89.6. It could not be fixed by moving the light: for a
horizontal surface the light must sit at the camera's own elevation on the
opposite side, which is behind the drive panel. A 45° chamfer supplies normals
at every azimuth on a 45° cone, which contains the half vector to within about
1.5°. Real stamped parts have a broken edge anyway. The slides' ns 89.6 is kept
unchanged.

**Black plastic is lightened** (`materials.h`), exactly the escape hatch FR-11
anticipates: `ka` 0 → 0.02 and `kd` 0.01 → 0.05, because the motor, magazine and
hood sit against a dark panel and outside the spot cone. `ks` and `ns` are
untouched, so it still reads as plastic.

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

**The chute is short and held to the near side** (`config.h`). At full size it
drew straight across the Geneva drive gear; it is 0.85 in front of the gear
plane, so this is composition, not collision.

## Files

| File | Contents |
|---|---|
| `src/config.h` | every tunable number, PRD-section referenced |
| `src/kinematics.h` | all closed-form motion — no OpenGL, shared with the test |
| `src/prim.h` | the one box routine and one cylinder routine |
| `src/materials.h` | the six materials |
| `src/shaders.h` | the GLSL 1.10 Phong vertex and fragment shaders |
| `src/lighting.h` | the two-light rig and the shading-mode switch |
| `src/scene.h` | display lists and the per-frame hierarchy |
| `src/main.cpp` | GLUT glue, state, input, HUD |
| `tests/mathcheck.cpp` | headless verification of PRD §10 |
| `PRD.md` | the requirements document this implements |
