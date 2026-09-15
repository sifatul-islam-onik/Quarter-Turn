# Project Proposal — "Quarter Turn"
### An automated stamping line in OpenGL

**Course:** CSE 4207 Computer Graphics · **Instructor:** Prof. Dr. Sk. Md. Masudul Ahsan, KUET
**Student:** Sifat · **Tools:** C++ with fixed-function OpenGL and freeglut, plus one GLSL shader

---

**The scene.** A short automated stamping line on a factory floor, seen from the front right. An electric motor on a steel drive panel turns a train of five meshing brass gears. The largest gear carries a crank, and a crank-slider mechanism converts its rotation into the up-and-down stroke of a press ram above a conveyor belt. The last gear drives a Geneva mechanism on the conveyor's head roller, which turns the roller exactly a quarter turn and then locks it. Each quarter turn advances the belt by one station: steel blanks drop from a feed magazine, ride the belt, stop under the press, are flattened while the belt is locked, and move on to an exit chute. A three-colour stack light shows the machine's current phase. A spotlight over the press makes the polished punch and blanks carry the mandatory specular highlight. The title is the unit of the whole machine: one quarter turn of the Geneva wheel is one station, one press stroke, and one finished part.

**The technical claim.** The entire animation state of the scene is **one accumulating float** (the crankshaft angle) and **one integer** (the cycle count). Every visible motion — each gear, the press, the belt and the parts it carries — is a closed-form function of those two values, evaluated inside the render loop. There is no pre-computed animation, no keyframe data, and no stored poses. Changing the speed therefore rescales the whole line in correct proportion: the teeth stay meshed, and the belt still advances exactly one station per press stroke, because belt travel is derived from the crank angle rather than from elapsed time.

**Concepts demonstrated.**
- **Transformations.** Translation, rotation, scaling and composite transformation, each doing necessary work rather than decoration. The scaling is a volume-preserving squash of each blank as it is pressed.
- **Hierarchical modelling.** Built as four chains that form two closed kinematic loops, at the press ram and at the Geneva pin. Meshing gears are deliberately modelled as constrained siblings rather than parent and child.
- **Lighting.** Two lights with full ambient, diffuse and specular terms, spotlight cutoff, spot exponent and distance attenuation, and an emissive stack light.
- **Materials and normals.** Six materials with specular exponents from 4 to 89.6, and a `GL_NORMALIZE` demonstration on the stamped blanks.
- **Shading.** Three switchable modes — flat, Gouraud, and per-pixel Phong written in GLSL, because the fixed pipeline does not provide it.
- **Hidden surfaces and z-fighting.** Hidden-surface removal by depth buffering and back-face culling, plus deliberate z-fighting prevention.
- **Optimisation.** Display lists and instancing: one tooth drawn 124 times across five gears.
- **Temporal aliasing.** At a predictable speed the whole gear train appears to stand still.

---

### Objects in the scene

| # | Object | Primitive | Motion |
|---|---|---|---|
| 1 | Floor | subdivided quad grid | static |
| 2 | Drive panel with ram guide rails | boxes | static |
| 3 | Motor and pinion gear (12 teeth) | cylinder, gear | **rotates** at 3× crank speed |
| 4 | Crankshaft, crank gear (36 teeth), crank disc | cylinders, gear | **rotates** — the single driver |
| 5 | Two idler gears (20 teeth) | gears | **rotate**, each reversing direction |
| 6 | Geneva shaft, drive gear (36 teeth), driver arm and pin | cylinder, gear, box | **rotates** |
| 7 | Geneva wheel, four slots | hub and slot bars | **quarter turns**, then locks |
| 8 | Connecting rod | box | **swings** between crank pin and ram |
| 9 | Ram and punch | box | **translates** vertically |
| 10 | Conveyor frame and two rollers | boxes, cylinders | rollers **quarter turn** |
| 11 | Belt and 24 cleats | strips, instanced boxes | cleats **travel the loop** |
| 12 | Blanks | one cylinder, instanced ×10 | **translate**, then **squash** |
| 13 | Feed magazine, exit hood, chute, bin | boxes | static |
| 14 | Stack light | three emissive cylinders | **colour follows phase** |

Fourteen objects, of which ten move. Every object is a hand-generated box or cylinder: no loaded meshes, no textures, no shadows. Nothing is modelled twice — one tooth serves all five gears, one cleat is instanced along the belt, and one blank is instanced along the line.

**The mechanism.** Meshing gears obey `ω₂ = −(N₁/N₂)·ω₁`, and each gear's tooth phase is computed so that a gap always faces its neighbour's tooth. The press ram follows the crank-slider solution `s(θ) = Y_C + r·cos θ − √(L² − r²·sin²θ)`, giving a stroke of exactly `2r`. From that formula the punch is below the top of a blank for `θ ∈ (132°, 228°)`. So the Geneva wheel is placed to turn only during `θ ∈ (315°, 45°)`, centred on the top of the stroke, with 87° of margin at each end. The belt therefore never moves while the punch is down — on a real press, indexing during the stroke would shear the part. This interlock is what makes the animation readable as a machine rather than as motion.

**Interaction.** Eleven key actions:
- run/stop
- speed control from 6 to 120 parts per minute
- single-step through a cycle
- shading-mode cycle
- wireframe
- fill light on/off
- normalise toggle
- three camera presets
- camera orbit
- reset
- quit

An on-screen overlay reports the shading mode, the crank angle and current phase (INDEX / APPROACH / STAMP / RETREAT), punch clearance, belt position, parts made, speed, and frame rate — so a viewer can read *what* is moving and *why*.
