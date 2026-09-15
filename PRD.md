# PRD — "Quarter Turn"
## An automated stamping line in OpenGL

**Course:** CSE 4207 Computer Graphics, Prof. Dr. Sk. Md. Masudul Ahsan, KUET
**Author:** sifat
**Scope decision:** core requirements plus a per-pixel Phong shader. The 2D clipping HUD, hand-built oblique projection, and multi-viewport layout are **out of scope** and recorded in §9 as deferred.
**Timeline:** 3–4 weeks
**Language:** C++17, fixed-function OpenGL 1.1 + freeglut, with a GLSL 1.10 shader for the Phong mode
**Target:** Windows, Intel Core i3-1215U, integrated Intel UHD (Xe-LP)

---

## 1. What this is

A short automated stamping line on a factory floor, seen from the front right. An electric motor on a steel drive panel turns a train of five brass gears. The largest gear carries a crank, and a crank-slider drives a press ram up and down over a conveyor belt. The last gear in the train turns a Geneva mechanism on the conveyor's head roller, which converts its continuous rotation into exactly one quarter turn of the roller followed by a locked pause. Each quarter turn advances the belt by one station: round steel blanks drop from a feed magazine, ride the belt, stop under the press, are flattened by the ram while the belt is locked, and move on under an exit hood. A three-colour stack light shows which phase the line is in, and a spotlight over the press makes the polished punch and blanks carry the mandatory specular highlight.

The title is the unit of the whole machine. One quarter turn of the Geneva wheel is one pitch of belt, one station, one press stroke and one finished part, and every rate in the scene is a fixed multiple of it.

**The one-sentence technical claim, which the whole project exists to prove:** the entire animation state of the scene is one accumulating float plus one integer, and every visible motion — five meshing gears, a press, an intermittent belt and the parts it carries — is a closed-form function of them, evaluated inside the render loop.

## 2. Goals

The project must convince an examiner of five things, in this order of importance. That all five taught model transformation types appear in the scene doing necessary work rather than decoration. That the motion is computed, not authored — demonstrable by changing the speed and watching every gear stay meshed, the press stay in step, and the belt still advance exactly one station per stroke. That lighting is maintained throughout with a mandatory, unmistakable specular highlight. That per-pixel Phong shading was understood well enough to implement it even though the slides state OpenGL does not provide it. And that the optimization was deliberate, with named decisions rather than an absence of obvious waste.

A sixth, softer goal: the scene should be legible in three seconds. Someone who has never seen a press line should be able to watch it and see power flow from the motor through the gears to the press and the belt — and see that the belt only moves while the press is up.

## 3. Non-goals

This is not a realistic press line. Gear teeth are rectangular blocks, not involute profiles. There are no forces, torques, inertia or motor dynamics. Blanks do not deform except by one volume-preserving squash, and nothing collides — the interlock between press and belt is guaranteed by timing geometry, not by detection. There are no textures, no shadows, no post-processing, no sound, and no loaded mesh files; every object is a hand-generated box or cylinder. There is no factory building: a floor and the machine's own drive panel are the only large surfaces, because the camera sees nothing else. There is no operator, no forklift and no smoke. The spotlight's fixture is not drawn, because it sits above the frame.

Deliberately excluded for scope reasons, all recorded in §9: the 2D clipping HUD, oblique projection, multiple viewports, and the mirror-reflection effect.

## 4. Scene definition

### 4.1 Coordinate system and framing

Right-handed, Y up. One unit is roughly 25 cm, so the belt top at 3.0 units is a 75 cm working height. The floor is the plane `y = 0`. Material flows in `+x`, left to right. The front of the machine, where the viewer stands, is `+z`. **Every rotating shaft in the scene is parallel to `z`**, so every mechanism turns in a plane that faces the camera.

Default camera: eye at `(7.0, 6.2, 9.0)`, look-at `(1.4, 2.9, 0.0)`, up `(0, 1, 0)`, `gluPerspective(45°, aspect, 4.0, 25.0)`. This three-quarter front-right view shows the gear train on the drive panel, the belt running across the frame, the Geneva wheel on the near right, and the press almost exactly at the centre of the frame. All three animated systems are visible at once without moving the camera.

The near and far planes are chosen by measurement, not habit. The eye is 11.10 units from the look-at point. The nearest geometry inside the view frustum is the exit hood's front-right corner at `(4.45, 3.5, 0.55)`, 9.23 units away, followed by the Geneva driver arm at 9.53. The farthest visible point is the floor's far-left corner at `(−4.5, 0, −1.5)`, 16.76 units away. A near plane at 4.0 clears everything with room for the orbit and camera presets to come closer, and 25.0 is generous at the far end. That gives `far/near = 6.25`. Depth-buffer precision at distance `z` scales roughly as `z²/near`, so pushing the near plane from a habitual 0.1 out to 4.0 is about a forty-fold gain in usable depth resolution across the scene. That is the z-fighting prevention the slides name, and quoting these numbers turns it from a slogan into a demonstrated choice. (The floor's near corner is closer, at 8.64, but it lies outside the frustum and is clipped by the bottom plane, so it does not constrain the near plane.)

### 4.2 Object inventory

Every dimension below is a starting value to be tuned on screen, unless §4.3 or FR-3 to FR-6 derive it. The ones that are derived — gear centres, the press's vertical stack, the Geneva geometry and the belt pitch — have to agree with each other, and changing one means recomputing the others.

All five gears lie in one **gear plane** at `z = −0.85`, 0.08 thick, with tooth module `m = 0.05`.

| # | Object | Primitive | Approx. size | Position | Material | Moves? |
|---|---|---|---|---|---|---|
| 1 | Floor | quad grid, 48 × 18 cells | 12.0 × 4.5 | `y = 0`; x −4.5 → 7.5, z −1.5 → 3.0 | concrete | no |
| 2 | Drive panel, with ram guide rails and bracket | boxes | panel 5.2 × 5.5 × 0.10; rails 0.06 × 0.90 × 0.10 | panel centre (3.2, 2.75, −1.0); rails at x 2.115 ± 0.24, y 3.2 → 4.1 | machine paint | no |
| 3 | Motor and pinion G1 | cylinder body; 12-tooth gear | body r 0.35 × 0.80; G1 pitch r 0.30 | axis (2.115, 5.950); body z −0.95 → −1.75 | black plastic / brass | **rotates**, 3× |
| 4 | Crankshaft, crank gear G2, crank disc and pin | cylinders; 36-tooth gear | G2 pitch r 0.90; disc r 0.35; throw 0.25 | axis (2.115, 4.750); shaft z −0.95 → −0.05 | brass / polished silver | **rotates** — the driver |
| 5 | Idler gears G3, G4 | 20-tooth gear ×2 | pitch r 0.50 | (3.342, 4.076), (4.010, 3.332) | brass | **rotate**, 1.8× |
| 6 | Geneva shaft, drive gear G5, driver arm and pin | cylinder; 36-tooth gear; box; cylinder | G5 pitch r 0.90; pin at radius 0.55 | axis (4.550, 2.040); shaft z −0.95 → 0.78 | brass / polished silver | **rotates**, 1× |
| 7 | Geneva wheel | hub cylinder + 8 slot bars (4 slots) | r 0.55; slots from 0.20 to 0.55 | head roller axis (4.00, 2.59), centred z 0.62 | brass | **quarter turns** |
| 8 | Connecting rod | box | 0.08 × 1.00 × 0.08 | crank pin → wrist pin, in plane `z = 0` | polished silver | **swings** |
| 9 | Ram and punch | box | 0.36 × 0.40 × 0.36 | x 2.115; top face at `s(θ)` | polished silver | **translates** |
| 10 | Conveyor frame and rollers | boxes; cylinder ×2 | rails 6.7 long at z ±0.55; rollers r 0.39 × 1.04 | roller axes (−2.283, 2.59), (4.000, 2.59) | machine paint / polished silver | rollers **quarter turn** |
| 11 | Belt and cleats | two strips + two half-shells; box ×24 | belt 1.0 wide, 0.02 thick; cleat 0.03 × 0.03 × 0.96 | loop around the rollers; top surface y 3.00 | rubber | cleats **travel the loop** |
| 12 | Blanks | cylinder, instanced up to 10 | r 0.18 × 0.20 | stations 1–9 on the belt | polished silver | **translate, squash** |
| 13 | Feed magazine, exit hood, chute, bin | boxes | magazine 0.5 × 1.36 × 0.5; hood 0.78 × 0.5 × 1.04 | magazine at station 1, y 3.24 → 4.60; hood x 3.67 → 4.45; bin x 5.0 → 6.2 | black plastic / machine paint | no |
| 14 | Stack light | cylinder ×3 on a post | r 0.10, 0.25 tall each | (5.5, 5.5 → 6.3, −0.8) | emissive | **colour follows phase** |

There are fourteen objects, and ten of them move. Nothing is modelled twice. One tooth display list serves all 124 teeth of the five gears, one cleat list is instanced 24 times, one blank list up to ten times, and one box routine and one cylinder routine generate every other part at its true size.

### 4.3 Structural decisions

Ten layout decisions below are each forced by a constraint. None of them should be simplified back.

**The gears are on the drive panel behind the line, not in front of it.** The crank, rod and ram have to lie in the belt's centre plane `z = 0`, because the punch strikes blanks on the belt centreline. A 0.9-radius gear in front of that plane would hide the crank from every camera on the operator's side: a three-quarter view shifts it sideways by a fraction of its radius, not by a whole radius. Behind the plane, the gear becomes a backdrop that the crank is seen against, and the crank-slider — rotation becoming translation — stays in plain view.

**The Geneva mechanism is on the front end of the head roller, and the gear train reaches it through a shaft.** The Geneva wheel has to share the head roller's axis to turn it, and behind the belt it would be hidden by the belt and frame. So the wheel sits on the roller's front stub, and G5's shaft runs forward from the panel to the driver arm, passing below and to the right of the belt's end. Its axis is 0.778 from the roller axis, which leaves 0.318 between the shaft's surface and the belt's outer surface. The wheel's left edge is 1.335 from the press station, so it does not cover the punch from the default camera.

**There are two idlers, not one.** The crank gear G2 and the Geneva gear G5 are 3.643 apart. A single idler would need at least 37 teeth to bridge that — and whatever its size, it would run the belt backwards. Every mesh reverses direction. The crank turns clockwise, so after three meshes G5 turns anticlockwise, the Geneva wheel turns clockwise, and the belt's top surface moves `+x` as required. With one idler there are only two meshes, and the belt runs the wrong way. Two 20-tooth idlers reach up to 3.8. The chain bends 19.3° off the straight line between G2 and G5, upwards, so both idlers stay visible above belt level. Distance and direction both force the second idler.

**The press is at station 7.** The press has to stand over a station so that a blank stops under it. Station 6 would put G2 4.09 from G5, beyond the two-idler reach of 3.8. Station 7 is the furthest upstream station the train can reach. Being as far upstream as possible keeps the press clear of the Geneva wheel and leaves two stations after the press, so finished parts are on show before they reach the exit.

**The roller centres are exactly 10 pitches apart.** Cleats on a closed loop only line up at the seam if the loop's length is a whole number of cleat spacings. The loop is `2D + 2πR_c` long. Because the pitch is *defined* as a quarter turn of the roller, `p = R_c·π/2`, the two roller arcs together are always exactly `4p`, whatever the roller's size. So only the straight run matters: `D = 10p` gives a loop of `24p = 15.080` and 24 cleats. It also makes every piece of the belt path a whole number of pitches long (FR-6).

**Cleats sit half a pitch from the stations.** Blanks sit on stations and cleats sit between them. A fully stamped blank has radius 0.2546 and the clear half-gap between cleats is 0.2992, which leaves a 0.045 margin. That margin also caps the squash: a blank flattened below about 0.072 tall would widen enough to touch a cleat.

**`glScalef` appears exactly once in the frame.** Every box and cylinder is generated at its true size by routines that take dimensions as arguments. The only scaling transform in the scene is the blank squash in FR-7. That keeps the normal-transformation demonstration clean: when `GL_NORMALIZE` is turned off, the blanks are the only thing that changes.

**The motor sits above the panel's top edge, with its body behind the gear plane.** The pinion has to lie in the gear plane to mesh with G2. A motor body in front of that plane would cover the first mesh in the train, and a body behind the panel would be invisible. The panel top is at 5.5 and the motor body spans y 5.60 → 6.30, so the motor is seen over the panel while its pinion meshes in front of it.

**Coincident surfaces always face opposite ways.** Several pairs of surfaces share a plane exactly: blank bottoms on the belt, cleat bottoms on the belt, and the punch face on a fully stamped blank at bottom dead centre (both at y 3.10). In each pair the two faces point in opposite directions, so back-face culling discards one of them from any camera above belt level — and the presets and orbit never put the camera anywhere else. Surfaces that face the *same* way are kept apart on purpose: gear back faces are 0.06 in front of the panel, and the Geneva wheel is 0.06 in front of the roller's end cap.

**Blanks appear and disappear only under cover.** The belt loops forever, so parts must enter and leave. A new blank appears inside the magazine tube only once the previous blank has cleared it by 0.02. The tube's lower edge is at 3.24, above an unstamped blank's top. The new blank then drops to the belt. A finished part moving from station 9 to station 10 passes under the exit hood, which starts 0.30 past station 9, so it is hidden before it is removed. Neither transition is ever visible, and both are functions of `θ` (FR-6).

## 5. Functional requirements

### FR-1 — Frame-rate-independent timing

The render loop obtains `dt` from successive `glutGet(GLUT_ELAPSED_TIME)` readings and passes it to a single update function. No motion may use a per-frame constant increment. On the first frame `dt` is forced to zero. `dt` is clamped to a maximum of 0.1 s, so that a window drag or a debugger breakpoint cannot throw the line forward by a large fraction of a cycle. At the top speed one clamped `dt` is 72° of crank rotation.

This requirement matters twice over. It is the correct way to animate, and it is the only way the demo behaves identically on a throttled 15 W laptop and on a lab machine.

### FR-2 — Single source of motion

Exactly two variables make up the animation state. The first is `theta`, the crankshaft angle in radians, accumulated as `theta += omega * dt` and wrapped to `[0, 2π)`. It increases clockwise as seen from the front. The second is `cycles`, a monotonically increasing integer incremented once per wrap of `theta`. Every other moving quantity in the scene is a pure function of these two plus fixed constants. There are no arrays of positions, no keyframes, no timelines, and no interpolation between stored poses.

| Derived quantity | Function of | Defined in |
|---|---|---|
| Gear angles φ₁ … φ₅, driver arm angle | θ | FR-3, FR-5 |
| Ram height `s` | θ | FR-4 |
| Phase name, stack light colour | θ | FR-5 |
| Belt travel `B`, roller and Geneva wheel angle | θ, cycles | FR-5 |
| Cleat positions | B | FR-6 |
| Blank positions, fresh-blank drop | θ | FR-6 |
| Blank heights | θ | FR-7 |
| Parts made | θ, cycles | FR-14 |

The state is the crankshaft angle rather than the motor angle, even though the motor is what "drives". Because the gear ratio is exact, this is only a choice of which shaft to count. The crankshaft is chosen because one revolution of it is one part, and every phase boundary in FR-5 is keyed to it. So the wrap and the cycle count happen on the shaft where they mean something.

`omega` is user-adjustable and shown to the user as parts per minute, since one revolution is one part. **The default is 30 ppm and the range is 6 to 120.** At 30 ppm one part takes 2 s and the index lasts 30 frames at 60 fps, which is slow enough to follow the pin into its slot. At 120 ppm the index still lasts 7.5 frames. The upper bound is set deliberately to include 100 ppm: at 60 fps the entire gear train appears to stand still there while the press and belt keep moving (FR-3). That is temporal aliasing — the render loop sampling a periodic motion too coarsely — and it is worth demonstrating and naming rather than hiding.

The wrap must be a `while` loop, not an `if`. Within this speed range one clamped `dt` never exceeds a revolution, but belt travel in FR-5 depends on `cycles` being incremented exactly once per revolution. A `while` keeps that true if the range is ever raised.

On reset, `theta = 90°` and `cycles = 0`. That puts the belt at rest, the ram descending, and belt travel exactly zero.

### FR-3 — Gear train

Each gear's pitch radius is `r = m·N/2` with module `m = 0.05`, and two meshing gears sit exactly `r_i + r_j` apart. The tooth counts, not the radii, set the speeds: `ω_j = −(N_i/N_j)·ω_i`, where the minus sign is the reversal at every mesh.

| Gear | Teeth | Pitch r | Centre | Meshes with | Speed × θ̇ | Sense |
|---|---|---|---|---|---|---|
| G1 motor pinion | 12 | 0.30 | (2.115, 5.950) | G2 | 3.0 | anticlockwise |
| G2 crank gear | 36 | 0.90 | (2.115, 4.750) | G1, G3 | 1.0 | clockwise |
| G3 idler | 20 | 0.50 | (3.342, 4.076) | G2, G4 | 1.8 | anticlockwise |
| G4 idler | 20 | 0.50 | (4.010, 3.332) | G3, G5 | 1.8 | clockwise |
| G5 Geneva gear | 36 | 0.90 | (4.550, 2.040) | G4 | 1.0 | anticlockwise |

G2's centre is fixed by the press (FR-4) and G5's by the Geneva geometry (FR-5), so G3 and G4 are not placed by eye. The chain of centre distances 1.40 + 1.00 + 1.40 is laid out symmetrically between them, bent 19.26° off the straight line. Gears that do not mesh stay at least 0.867 apart after allowing for both tooth tips. At the 30 ppm default the motor turns at 90 rpm.

**Tooth phase.** Angles follow the `glRotatef` convention: anticlockwise positive about `+z`, as seen from the front. Tooth 0 of every gear lies along its local `+x` axis, and tooth `k` sits at `φ + 2πk/N`. When gear `j` is driven by gear `i`, and `ψ_ij` is the direction from `i`'s centre to `j`'s,

```
φ_j = −(N_i/N_j)·(φ_i − ψ_ij) + ψ_ij + π + π/N_j
```

evaluated from `φ₂ = −θ` along G2→G1 and G2→G3→G4→G5. The first term is the speed ratio and the reversal. The `ψ_ij + π` term turns `j`'s frame to face back along the line of centres whenever `i` has a tooth pointing along it. The `π/N_j` term turns `j` by half a tooth, so that a *gap* faces `i`'s tooth rather than another tooth.

At the contact point, let `u_i` be how far `i` is through its current tooth, as a fraction of one tooth spacing, and define `u_j` the same way for `j`. These always satisfy `u_i + u_j ≡ ½ (mod 1)`, for every `θ`. A mesh that is correct at one angle is therefore correct forever. The failure mode is worth knowing: forgetting `π/N_j` leaves every pair of teeth sitting tip-to-tip *through* each other for all time. That looks like a modelling error rather than a maths error, and at speed it can go unnoticed.

**Every gear in the train passes teeth at the same rate.** All the gears share the pitch-line speed and the module, so `N_i·ω_i` is identical for all of them — here `36·θ̇`, which is `0.6 × ppm` teeth per second. That has a visible consequence. Every gear has rotational symmetry of one tooth, so at a displayed frame rate `F` the whole train appears to stand still at `F / 0.6` ppm, and to turn backwards between `F / 1.2` and `F / 0.6` ppm. At 60 fps those are 100 ppm and 50–100 ppm. The crank, the ram and the belt have no such symmetry and keep visibly moving, which is exactly what makes the effect unmistakable. All five gears freeze *at once* because of the mesh law.

**Drawing.** One tooth is a box from the root radius `r − 1.25m` to the tip radius `r + m` (0.1125 radially), 0.08 deep. Its tangential width is 0.45 of the circular pitch `πm` (0.0707) — less than half — so that rectangular teeth, which lack an involute profile's clearance, do not visibly intersect at the mesh. That width is a starting value: step through a cycle in wireframe and narrow it if corners cross. The gear body is a cylinder at the root radius with a hub.

### FR-4 — Crank-slider press

The crankshaft turns clockwise with `θ`, so the crank pin, at throw `r` from the axis, is at `(x_P + r·sin θ, Y_C + r·cos θ)`, at the top when `θ = 0`. The ram is constrained to the vertical line `x = x_P` below the crank and is connected to the pin by a rigid rod of length `L`. Solving the rod constraint gives the height of the wrist pin, which is the ram's top face:

```
s(θ) = Y_C + r·cos θ − √(L² − r²·sin²θ)
```

with `r = 0.25`, `L = 1.00` and `Y_C = 4.75`. The maximum is `s = 4.00` at `θ = 0` and the minimum is `s = 3.50` at `θ = π`, so the stroke is `0.50`.

The stroke is *exactly* `2r`. `sin θ = 0` at both extremes, so the square-root term equals `L` at each and cancels out of the difference. The rod length shapes how the ram moves between the extremes, but not how far it travels. That the extremes fall at 0 and π follows from the derivative:

```
ds/dθ = −r·sin θ · [1 − r·cos θ / √(L² − r²·sin²θ)]
```

The bracket is positive everywhere, because `r²cos²θ < L² − r²sin²θ` reduces to `r² < L²`. So `s` falls monotonically across `(0, π)` and rises monotonically back.

**Constraint:** `L ≥ 2.5r` must hold. The reason is *not* that the square root would otherwise go negative: its smallest value is `L² − r² = 0.9375`, which is positive for any `L > r`. The real reason is rod obliquity. The rod's largest angle from the ram's axis is `arcsin(r/L)`, which here is 14.48°. Keep the ratio above about 2.5 and the rod stays near vertical and the motion near sinusoidal. Let it fall towards 1 and the rod flails through a wide arc and stops reading as a press. Assert the ratio at startup, and give the obliquity reason in the report.

**The vertical stack at the press** is derived from the bottom up, and it is the one place in the scene where the numbers have to close:

| Level | y | Derived from |
|---|---|---|
| Belt top surface | 3.00 | roller axis 2.59 + roller radius 0.39 + belt 0.02 |
| Unstamped blank top | 3.20 | belt top + blank height 0.20 |
| Stamped blank top = punch face at bottom dead centre | 3.10 | belt top + stamped height 0.10 |
| Punch face at top dead centre | 3.60 | `s_max` − ram height 0.40 |
| Wrist pin (ram top) | 4.00 ↔ 3.50 | `s(θ)` |
| Crank axis `Y_C` | 4.75 | `s_min + r + L` |
| Crank pin, highest | 5.00 | `Y_C + r` |
| G2 lowest tooth tip | 3.80 | `Y_C − 0.95` |

The punch clears an unstamped blank by 0.40 at the top of the stroke. The guide rails span 3.2 → 4.1 and hold the ram over its whole travel (3.10 → 4.00). The rails' bracket reaches the panel below 3.80, so it passes under G2. If the stamped height, the ram height or the belt height change, the crank axis moves, G2 moves with it, and FR-3's idler positions must be recomputed.

This formula answers the composite-transformation requirement for the press: a rotation converted into a translation.

### FR-5 — Geneva indexing and the press interlock

The Geneva mechanism turns the continuous rotation of G5's shaft into intermittent motion of the head roller, with a positive lock during the pause. That is why real indexing lines use it, and why the belt here can stand perfectly still while the press strokes.

**Geometry.** The wheel has `n = 4` slots and the driver pin runs at radius `a = 0.55`. For the pin to enter and leave each slot without impact, the slot must line up with the pin's direction of travel at the moment of entry. That requires a centre distance `c = a / sin(π/n) = 0.7778` and a wheel radius `√(c² − a²) = 0.55`. The driver is engaged within `90° − 180°/n = 45°` either side of the line of centres. So the wheel turns `2π/n = 90°` per revolution of the driver and is locked for the remaining 270°.

With `λ = sin(π/n) = 0.7071`, and `α` the driver's angle from the line of centres, the wheel's rotation during engagement is

```
β(α) = atan2(λ·sin α, 1 − λ·cos α)        for |α| ≤ 45°
```

At `α = ±45°`, `tan β = λ²/(1 − λ²) = 1`, so `β = ±45°` and the wheel turns exactly a quarter turn across the engagement. The pin is deepest at `α = 0`, at radius `c − a = 0.2278` from the wheel's centre, which is why the slots reach inward to 0.20. The wheel's angular speed, `dβ/dα = λ(cos α − λ) / (1 − 2λ·cos α + λ²)`, is zero at both ends of the engagement. The belt starts and stops smoothly with no step in speed, which is both why the mechanism is used and why it looks right.

**Placement.** The line of centres from the driver to the wheel points at 135°. The driver arm angle is `135° + θ` (the arm is keyed to G5, which turns at `+θ̇`), so `α` is simply `θ` wrapped to `(−180°, 180°]`, and the index is centred on the press's top dead centre.

**The interlock is derived, not chosen.** The punch is below the top of an unstamped blank when `s(θ) − 0.40 < 3.20`. Substituting `c = cos θ`:

```
4.35 + 0.25c − √(1 − 0.0625·(1 − c²)) = 3.20
0.25c + 1.15 = √(0.9375 + 0.0625c²)
0.0625c² + 0.575c + 1.3225 = 0.9375 + 0.0625c²
c = −0.385 / 0.575 = −0.66957        →   θ = 132.03°  and  227.97°
```

So the punch is inside the blank zone for `θ ∈ (132.03°, 227.97°)`, and **the belt may only move outside that window.** The rule is measured against the *unstamped* height on purpose. After bottom dead centre the punch leaves the flattened part at 3.10 immediately, but the next blank arriving at the press is unstamped, so the lock must hold until the punch clears 3.20. On a real press, indexing during the stroke shears the part or throws it off the line. Stating that reason on screen and in the report satisfies the requirement for animation with clear indications of *why* something moves.

| Phase | θ range | Width | Geneva | Ram | Belt |
|---|---|---|---|---|---|
| INDEX | 315°–45° | 90° | pin in slot; wheel turns 90° clockwise | near the top, ≥ 0.34 above the blank | advances one pitch, 0.628 |
| APPROACH | 45°–132.03° | 87.03° | locked | descending | stopped |
| STAMP | 132.03°–227.97° | 95.94° | locked | below the blank top; flattens it by 180°, then lifts | stopped |
| RETREAT | 227.97°–315° | 87.03° | locked | rising | stopped |

INDEX lies inside the safe region with 87.0° of margin at each end. The smallest punch clearance during INDEX is 0.34, at its two edges. A four-slot wheel gives the press 270° of locked belt, three times what it needs.

**Belt travel.** Belt travel is measured in pitches as `B = I + g`, where

```
I = cycles − 1   if θ < 45°,   else cycles
g = (β(α) + 45°) / 90°   if |α| ≤ 45°,   else 0
```

**`I` must count indices completed at the *end* of INDEX (θ = 45°), not at the wrap.** INDEX spans 315°→45° and therefore straddles `θ = 0`, which is exactly where `cycles` increments. Taking `I = cycles` directly would make the belt jump a full pitch at top dead centre once per part — at the one moment the belt is visibly moving and the viewer is watching it. With the definition above, `B` is continuous and monotonic across both 45° and the wrap, and it stays derived rather than stored, so FR-2's two-variable claim survives.

The head roller and the Geneva wheel share the angle `−90°·B` (the `glRotatef` convention), with slots at local 0°, 90°, 180° and 270°. The driver arm is keyed to G5's shaft, so `arm angle − φ₅` is a constant. Compute that constant once at startup from `θ = 0`, rather than typing a number in, so the arm cannot drift out of step with its own gear.

### FR-6 — Belt, cleats and blanks

The belt's surface travels `d = p·B`, with pitch `p = R_c·π/2 = 0.62832` and `R_c = 0.40` the radius of the belt's centreline (roller 0.39 plus half the belt's thickness). The tail roller is driven by the belt and has the same radius, so it shares the head roller's angle.

**The belt strips themselves are static.** Without textures, a uniform strip that moves looks identical to one that does not. The two straight strips and two half-shells are therefore one static display list, and all of the belt's visible motion is carried by the 24 cleats. This is honest rather than a shortcut: motion is only visible when something breaks the symmetry, and the cleats are that something.

**Belt path.** Arclength `s` is measured along the centreline from the top of the tail roller, moving `+x`. Positions are placed on the outer surface, at radius 0.41. By §4.3 every piece is a whole number of pitches:

| s range | Piece | Position | Cleat rotation |
|---|---|---|---|
| `[0, 10p)` | top run | `(X_T + s, 3.00)` | 0° |
| `[10p, 12p)` | around head roller | `φ = 90° − (s − 10p)/R_c`; `(4.00 + 0.41 cos φ, 2.59 + 0.41 sin φ)` | `φ − 90°` |
| `[12p, 22p)` | bottom run | `(4.00 − (s − 12p), 2.18)` | 180° |
| `[22p, 24p)` | around tail roller | `φ = 270° − (s − 22p)/R_c`; `(X_T + 0.41 cos φ, 2.59 + 0.41 sin φ)` | `φ − 90°` |

with `X_T = −2.2832`. Cleat `k` (for `k = 0 … 23`) sits at `s_k = ((B + k + ½)·p) mod 24p`. Each cleat is a translation to its path position composed with a rotation to the belt's outward normal.

**Blanks.** Station `j` is at `x_j = X_T + j·p`: station 1 (−1.655) is under the magazine, station 7 (2.115) is under the press, and station 10 (4.000) is at the top of the head roller, under the exit hood. Blanks carry labels `j = 1 … 9`, and blank `j` is drawn at `(X_T + (j + g)·p, 3.00)`. When an index completes, `g` returns to 0 and `I` advances, so every label now refers to the blank one station further on. The label-9 blank reaches station 10 under the hood and simply stops being drawn.

A fresh blank is drawn at station 1 during INDEX once `g ≥ 0.605`. At that point the departing blank has moved 0.38, two radii plus a 0.02 gap. The fresh blank's height is `y = 3.24 − 0.24·clamp((g − 0.605)/0.10, 0, 1)`: it starts hidden inside the magazine tube and drops onto the belt over the next tenth of the index. When `g` reaches 1 it is exactly where the label-1 blank will be drawn after the index completes, so the handover is seamless. At most ten blanks are drawn.

Blank labels and cleats are offset by `(I + k + ½ − j)·p`, which is always a half-integer number of pitches. So blanks never overlap cleats, at any `θ`.

### FR-7 — Stamping, scaling and normal transformation

Only the blank at the press station changes height. Its height is a function of `θ` alone:

```
h₇(θ) = clamp(s(θ) − 0.40 − 3.00, 0.10, 0.20)    for 45° ≤ θ < 180°
h₇(θ) = 0.10                                     otherwise
```

Blanks with a label below 7 are unstamped (`h = 0.20`), and those above 7 are stamped (`h = 0.10`). Checking against FR-5: before 45° the label-7 blank is the finished part leaving the press, and from 45° it is the new arrival. From 132.03° it flattens, exactly as far as the punch has descended, and from 180° it stays flat. Every boundary is continuous, and nothing is stored.

**The squash preserves volume.** With `q = h / 0.20`, the blank is drawn with `glScalef(1/√q, q, 1/√q)`, so a flattened part widens as metal would. At full squash it is 0.10 tall with radius 0.2546, and `0.18² × 0.20 = 0.2546² × 0.10`. A squash that only shortened the blank would read as the part shrinking, not being pressed. The scale is applied **about the blank's base**: translate to the belt surface first, then scale, then draw a cylinder that rises from `y = 0`. The order is a composite transformation in its own right. Scaling about the centre would lift the flattened blank off the belt.

This is the scene's only non-uniform scale (§4.3). It is the project's reason to call `glEnable(GL_NORMALIZE)`, which is the runtime face of the `N' = (M⁻¹)ᵀ N` slide, and a key toggles it off so the error can be shown deliberately.

**Be precise about what the error actually is.** The scale is `S = diag(1/√q, q, 1/√q)`, so `(S⁻¹)ᵀ = diag(√q, 1/q, √q)`. The blank's wall normals lie in the XZ plane, and X and Z are scaled equally, so their directions stay correct and only their length changes — to `√q`, which is 0.707 at full squash. The cap normals `(0, ±1, 0)` also keep their direction, but their length becomes `1/q`, which is 2.0. So the error is purely one of length, in opposite directions for walls and caps, and polished silver's specular exponent of 89.6 magnifies it enormously. Fixed-function lighting raises `max(N·H, 0)` to the exponent. On a cap, `N·H` can exceed 1 wherever the surface is within 60° of `H`, and `2^89.6` saturates to white. On a wall, `N·H ≤ 0.707`, and `0.707^89.6 ≈ 3 × 10⁻¹⁴`, so the rim highlight vanishes and the diffuse term drops by about 30%.

The visible defect: **flattened parts' tops blow out to white while their rims go dull, and the unstamped blanks beside them on the same belt stay correct.** Those unstamped blanks are drawn with no scale at all, so the comparison sits in a single frame. During STAMP the error grows continuously as `q` falls. `GL_RESCALE_NORMAL` is the wrong fix and worth saying so: it applies a single correction factor, which is only right for uniform scaling. Here the walls and caps need different corrections, and only normalising each normal provides them.

### FR-8 — Model transformations

Every taught type must appear doing necessary work. This table is the requirement, not an illustration of it.

| Type | Where it appears | Necessary? |
|---|---|---|
| Translation | ram height `s(θ)`, blanks along the belt, cleats on the straight runs, the fresh-blank drop, all object placement | yes — parts cannot travel otherwise |
| Rotation | five gears, crankshaft, Geneva driver and wheel, both rollers, rod swing, cleats turning around the roller arcs | yes |
| Scaling | volume-preserving blank squash | yes — it is the press's product |
| Composite | the crank-slider `s(θ)`; the Geneva `β(α)`; the rod's placement from both endpoints; the cleat path (translate along the path, rotate to the normal); scaling about the blank's base | yes — these are the mechanisms |
| Hierarchical | four chains and two closed loops, FR-9 | yes |

### FR-9 — Hierarchy, including two closed loops

Chain A: `floor → drive panel → crankshaft → crank disc → crank pin → connecting rod → ram → punch`.
Chain B: `floor → drive panel → guide rails → ram`.
Chain C: `floor → drive panel → Geneva shaft → driver arm → pin`.
Chain D: `floor → conveyor frame → head roller → Geneva wheel`, with `conveyor frame → belt path → cleat k` and `conveyor frame → station j → blank → squash` as its other branches.

Chains A and B meet at the ram, and chains C and D meet where the pin sits in the slot. That makes the scene two **closed kinematic loops** rather than a tree, one structural level above what a hierarchical-modelling requirement normally asks for. Both are free, because each loop is closed analytically — the first by `s(θ)`, the second by `β(α)` — rather than iteratively. Worth naming explicitly in the report.

**Meshing is not parenting.** It is tempting to make G3 a child of G2, and it is wrong. A child inherits its parent's rotation, but a meshing gear turns the opposite way at a different rate, so G3's own transform would first have to undo G2's rotation. The hierarchy would be lying about the structure it claims to describe. Every gear is instead a child of the drive panel, and the mesh law in FR-3 is a constraint between siblings. This is the obvious-looking wrong answer, so it is worth one sentence in the viva.

Implemented with `glPushMatrix`/`glPopMatrix` only. During development every angle — θ, each gear, the wheel, the belt travel — can be overridden from a debug key, so each part can be verified in isolation before the chains are connected.

### FR-10 — Lighting

Two lights, both with full ambient, diffuse and specular terms, plus a low global ambient.

**Light 0 — press lamp, spotlight.** Position `(2.115, 8.0, 1.8)`, aimed at the die, `(2.115, 3.10, 0.0)`, 5.22 away. `GL_SPOT_CUTOFF` 25°, `GL_SPOT_EXPONENT` 10. The cone is about 2.4 in radius at die height, so the pool covers the press, the stations either side of it, and the crank gear behind. Attenuation `a₀ = 1.0`, `a₁ = 0.03`, `a₂ = 0.01`, so the radial formula `1/(a₀ + a₁d + a₂d²)` from the slides is genuinely in play: it gives 0.70 at the die and about 0.5 on the floor. Diffuse warm white `(1.00, 0.96, 0.88)`, specular `(1, 1, 1)`, ambient `(0, 0, 0)` — a spotlight contributing ambient would defeat its own cone.

**Light 1 — fill, point light.** Position `(9.0, 6.0, 8.0)`, no spot. Diffuse is a low, cool `(0.25, 0.26, 0.30)`, specular `(0.15, 0.15, 0.18)`, with mild attenuation. Its job is to keep the Geneva wheel, the tail of the line and the dark motor readable outside the spot, without washing out the cone. A key toggles it, so the two-light sum can be demonstrated as a sum.

**Light model.** `GL_LIGHT_MODEL_AMBIENT` `(0.10, 0.10, 0.12)`. `GL_LIGHT_MODEL_LOCAL_VIEWER` set to `GL_TRUE`. The default, `GL_FALSE`, does not merely dim highlights: it substitutes a constant view direction `(0, 0, 1)` in eye space for the true per-vertex direction to the eye. Highlights then stop shifting correctly as a surface moves across the frame. Moving highlights are exactly what this scene shows — sweeping across brass teeth, and sliding along blank tops as they travel — so paying for the accurate view vector is not optional.

**The stack light is emissive.** The lit segment of the stack light — amber, green or red according to the phase (FR-14) — has a matching `GL_EMISSION`, and the unlit segments have a dim one. That puts the `+ I_e` term of the slides' illumination equation on screen, doing useful work. That term is in the equation the report will quote, and it is otherwise the one component nothing in a scene like this demonstrates.

**Critical implementation constraint.** `glLightfv(..., GL_POSITION, ...)` transforms the position by the current modelview matrix, and `glLightfv(..., GL_SPOT_DIRECTION, ...)` transforms the direction by its upper-left 3×3. *Both* must therefore be specified once per frame, *after* the camera transform is established and *before* any model transform. Getting this wrong produces lights that swim with the objects, the single most common lighting bug in fixed-function OpenGL. Getting the position right but forgetting the direction produces a spotlight whose cone points somewhere else — the same bug wearing a different hat. Specify the position's fourth component explicitly as `1.0`. A `w` of `0.0` silently makes the light directional, which discards both the position and the attenuation, and the resulting flat, unattenuated scene looks plausible enough to go unnoticed for an hour. All of this lives in one function called at one place.

**Consequence for geometry.** In fixed-function mode the spotlight cone is evaluated per vertex, so the surfaces the cone falls on must be subdivided or its edge will be invisible until the shader runs. The floor is a 48 × 18 grid, the belt's static top strip is split into 40 segments along its length, and the drive panel is a 26 × 22 grid. These are the only places the project deliberately spends triangles, and the reason is worth documenting.

### FR-11 — Materials

Six materials: three taken verbatim from the slides' coefficient table (credited there to Hill, after McReynolds and Blythe) and three authored. The `ns` values span 4 to 89.6, which demonstrates the specular-exponent figure within a single frame.

| Material | ka | kd | ks | ns | Applied to |
|---|---|---|---|---|---|
| Polished silver *(slide)* | .23125 | .2775 | .773911 | 89.6 | blanks, ram and punch, rod, shafts, crank disc and pin, Geneva pin, rollers |
| Brass *(slide)* | .329/.224/.027 | .780/.569/.114 | .992/.941/.808 | 27.8974 | gears G1–G5, Geneva wheel |
| Black plastic *(slide)* | 0, 0, 0 | .01, .01, .01 | .5, .5, .5 | 32 | motor body, magazine, exit hood, stack-light housing |
| Machine paint *(authored)* | .05/.08/.06 | .20/.34/.26 | .30 | 20 | drive panel, conveyor frame, guide rails, chute, bin |
| Rubber *(authored)* | .02 | .06 | .05 | 6 | belt, cleats |
| Concrete *(authored)* | .10/.10/.09 | .45/.44/.42 | .03 | 4 | floor |

**Known issue with black plastic.** The slide's values give zero ambient and a diffuse of 0.01, so the motor, magazine and hood are essentially black except where the specular term catches them. That is arguably accurate and looks striking, but it may leave them unreadable against the dark panel. If so, raise `kd` to about 0.05 and `ka` to about 0.02 for those parts, and **document the deviation in the report** — an examiner will respect a stated, reasoned departure far more than an undocumented one.

### FR-12 — Three shading modes

A key cycles flat → Gouraud → per-pixel Phong, and the current mode is shown on screen. This is the highest-value feature in the project, because the slides devote a section to comparing exactly these three and state that the last is not available in OpenGL.

Flat mode uses `glShadeModel(GL_FLAT)` with the fixed pipeline. Gouraud mode uses `glShadeModel(GL_SMOOTH)` with the fixed pipeline. Phong mode binds a GLSL program that interpolates the normal and evaluates the illumination equation per fragment:

```
I = ka·Ia + kd·Il·max(N·L, 0) + ks·Il·pow(max(V·R, 0), ns) + Ie
R = 2(N·L)N − L
```

The shader writes it in that form rather than using GLSL's `reflect()`, so the source visibly matches the slide. Both lights are summed, and both the spotlight cone factor and the radial attenuation are applied per fragment.

**The difference between the modes is larger than per-vertex versus per-fragment, and this is the most valuable thing in the document.** Fixed-function OpenGL does not implement the Phong specular term at all. It implements **Blinn–Phong**: it forms the half-vector `H = (L + V) / |L + V|` and raises `N·H` to the shininess, exactly as the slides present it as the cheaper alternative. So the toggle demonstrates three things at once: flat versus interpolated shading, per-vertex versus per-fragment evaluation, and `(N·H)^ns` versus `(V·R)^ns`. The last is *visible*. For the same exponent the half-vector highlight is noticeably broader, because the angle between `N` and `H` is roughly half the angle between `V` and `R`; that is why matching a Blinn highlight to a Phong one needs about four times the exponent. Expect highlights to *tighten* when Phong mode engages even before considering the per-pixel effect, and say why. Both models are in the slides. Being able to point at which one the API gave you and which one you had to write yourself is the whole argument of the feature.

**Both modes must read the same state, or the comparison is worthless.** GLSL 1.10 exposes the fixed-function state directly:

- `gl_LightSource[i]`, with `.position`, `.diffuse`, `.specular`, `.spotDirection`, `.spotCosCutoff`, `.spotExponent` and the three attenuation coefficients
- `gl_FrontMaterial`, with `.ambient`, `.diffuse`, `.specular`, `.shininess` and `.emission`
- `gl_LightModel.ambient`

The shader therefore needs **no uniforms plumbed through at all.** It reads the identical lights and materials the fixed pipeline reads, so the three modes are a genuine controlled comparison rather than three differently tuned looks, and an entire category of bug disappears. Two details will cost time if unknown. `spotCosCutoff` is the *cosine* of the cutoff angle, not the angle. And `gl_LightSource[i].position` is already in **eye space**, because `glLightfv` transformed it when it was set, so the fragment shader must do all its work in eye space and must not transform it again.

**Acceptance:** at the default camera, during APPROACH, the highlight on the punch's front face and on the blank tops must be plainly present in Phong mode and plainly absent or dim in Gouraud mode. Both are large flat faces with few vertices, which is exactly where per-vertex lighting misses a highlight that lands between vertices. If the contrast is not obvious, *lower* the blank's slice count until it is.

Three smaller things are worth knowing before writing the shader. First, GLSL's built-in `gl_NormalMatrix` is exactly the inverse transpose of the modelview's upper 3×3 — literally the `(M⁻¹)ᵀ` of the slide. It is not normalised, though, so the vertex shader must write `normalize(gl_NormalMatrix * gl_Normal)`. That call is the shader-side equivalent of `GL_NORMALIZE`, with the consequence recorded in FR-13. Second, in GLSL 1.10 interstage variables are declared `varying`, not `in`/`out`, and the fragment output is `gl_FragColor`. Third, Intel's GLSL compiler is stricter than most about implicit conversion. A literal `1` where a `float` is expected is a compile error rather than a warning, which produces a confusing first-run failure that has nothing to do with the lighting maths.

### FR-13 — Interaction

| Key | Action |
|---|---|
| `Space` | run / stop |
| `↑` `↓` | speed ±6 ppm, range 6–120 |
| `.` | while stopped, step `θ` forward by 5° |
| `s` | cycle shading mode: flat → Gouraud → Phong |
| `w` | wireframe toggle, to expose tessellation and check tooth clearance |
| `l` | fill light on / off |
| `n` | `GL_NORMALIZE` on / off, to show the normal-transformation error on stamped blanks |
| `1` `2` `3` | camera presets: three-quarter, front elevation, top plan |
| `←` `→` | orbit camera about the line |
| `r` | reset: `θ = 90°`, `cycles = 0` |
| `Esc` | quit |

Eleven actions, none of which required extra geometry. The front-elevation preset shows the gear train as a flat drawing, and the top plan shows the stations and the stamped parts. Together the presets lightly touch the orthographic-multiview topic at almost no cost.

The step key advances the state variable itself; it is not a separate animation path. It only steps forward, because `cycles` is monotonic. It is the tool for checking every phase boundary and every tooth mesh by eye.

One interaction between two of these keys has to be handled, or it looks like a bug. The `n` key toggles a fixed-function state, and Phong mode does not use the fixed-function pipeline: the shader normalises its own interpolated normal, so pressing `n` in Phong mode correctly does nothing at all. There are two acceptable answers. One is to grey out the HUD's normalize indicator in Phong mode and demonstrate the toggle in Gouraud, which is honest and costs a line. The other is to have the shader read the flag and skip its `normalize()` call, mirroring the behaviour; that is a better demonstration and costs a uniform. Whichever is chosen, say which — an examiner pressing keys in the wrong order should not be able to make the project look broken.

### FR-14 — On-screen indications

A bitmap-text overlay is drawn in an orthographic pass after the 3D scene. It shows:

- shading mode, and the normalize state
- crankshaft angle `θ` in degrees, and the **phase name** (INDEX / APPROACH / STAMP / RETREAT) with "belt locked" or "belt moving"
- punch height and its clearance above the blank at the press
- Geneva wheel angle and belt travel in stations
- parts made, `cycles + [θ ≥ 180°]`
- speed in ppm and motor speed in rpm
- frame time in milliseconds and frame rate

The frame rate is what makes the gear-freeze observation in FR-3 checkable. The stack light repeats the phase in the scene itself: amber during INDEX, green during APPROACH and RETREAT, red during STAMP.

The phase line is the most important one. It converts the line from something that moves into something the viewer can *read*: the belt is locked because the punch is down. The requirement asks for animation with clear indications of what is moving and why, and this satisfies it.

Drawing the overlay is a five-step ritual in which every step is load-bearing, so it belongs in one function with the order fixed:

1. Unbind the shader with `glUseProgram(0)`, or the fragment stage will compute lighting for the glyphs and overwrite their colour.
2. Disable `GL_LIGHTING`, or the text takes its colour from the current material instead of `glColor3f` and comes out black on black.
3. Disable `GL_DEPTH_TEST`, or the overlay is hidden by whatever it happens to sit behind.
4. Push and load identity on *both* the projection and the modelview, setting `gluOrtho2D(0, w, 0, h)` on the projection. `glutBitmapCharacter` draws glyphs in screen space, but `glRasterPos` is transformed by the full pipeline, so the raster position must be in that orthographic space or the text lands off screen.
5. Pop both matrices and re-enable lighting and depth testing.

Skipping any one of these produces text that is invisible, black, or behind the machine, and all three failures look as if the text was never drawn.

### FR-15 — Optimization

Static geometry is compiled into display lists once: floor, drive panel with rails, conveyor frame, belt strips, and magazine with hood, chute and bin. One tooth list is drawn 124 times across the five gears, one cleat list 24 times, and one blank list up to 10 times. Each gear body is its own list. `glEnable(GL_CULL_FACE)` is on throughout. `sin θ` and `cos θ` are computed once per frame and passed down. The five gear angles come from a single walk of the chain per frame, four evaluations of the mesh law. Light positions are re-specified exactly once per frame. There is no geometry outside the view frustum, because the framing is the line itself and nothing exists off camera.

Culling does two jobs here. It is the taught hidden-surface technique, and by §4.3 it is also what stops the coincident, opposite-facing surfaces of blank, belt, cleat and punch from fighting in the depth buffer.

Two constraints follow from using display lists, and they are easier to obey from the start than to retrofit. First, a display list captures the *values* passed to `glMaterialfv` at the moment it is compiled, not a reference to them. Any material that changes at runtime — the stack light's emission, or black plastic under FR-11's escape hatch — must be set outside the list, immediately before calling it. Second, the hand-written cylinder and box routines must use consistent winding. Back-face culling silently deletes half of any cylinder whose quad strip runs the wrong way, and a half-missing cylinder reads as a modelling mistake rather than a state one.

`glutInitDisplayMode` must request `GLUT_DEPTH` alongside `GLUT_DOUBLE | GLUT_RGB`, and `GL_DEPTH_TEST` must be enabled. This is worth stating precisely because it is easy to omit, and because depth buffering plus back-face culling are the taught hidden-surface-elimination techniques. They belong in the report as a claimed topic, not as an assumed default.

The frame-time readout is part of this requirement, not a debug leftover: a stable number on screen is what makes the optimization claim checkable.

## 6. Requirements traceability

Every line of the teacher's brief, mapped to where it is satisfied. This table is the document's real purpose: it is what to check before submission and what to walk an examiner through in a viva.

| Teacher's requirement | Satisfied by | Verify |
|---|---|---|
| Industrial factory scene: multiple gears moving one another, a conveyor belt | a five-gear train driving a press and a Geneva-indexed conveyor | FR-3, FR-5, FR-6 |
| Simple 3D scene, not overly complex | 14 objects, all boxes and cylinders, floor and drive panel only | §4.2 |
| Implemented in OpenGL | fixed-function 1.1 + freeglut, GLSL 1.10 for one mode | — |
| Runs continuously in the render loop | idle callback → update(dt) → redisplay | FR-1 |
| Visible motion | gears turn continuously; the press strokes and the belt indexes every 2 s at the default speed | FR-3 – FR-6 |
| Clear indications of what moves and why | phase readout, stack light, and the belt-locked-during-stroke reason | FR-5, FR-14 |
| Translation | ram, blanks, cleats, fresh-blank drop | FR-8 |
| Rotation | gears, crankshaft, Geneva driver and wheel, rollers, rod, cleats on arcs | FR-8 |
| Scaling | volume-preserving blank squash about its base | FR-7 |
| Combination / composition | crank-slider, Geneva `β(α)`, cleat path, scale about base | FR-4, FR-5, FR-6, FR-7 |
| Hierarchical | four chains and two closed loops; meshing modelled as a sibling constraint | FR-9 |
| Lighting properly maintained | two lights, full ambient/diffuse/specular, correct per-frame positioning | FR-10 |
| **Specular mandatory** | polished silver at ns 89.6 on the punch and moving blanks under a spotlight; brass teeth sweeping highlights | FR-11, FR-12 |
| Phong preferred, more marks | three-mode toggle; the fixed pipeline gives Blinn–Phong `(N·H)^ns`, the shader gives true Phong `(V·R)^ns` per fragment | FR-12 |
| Demonstrates concepts over spectacle | no textures, shadows, smoke or building | §3 |
| Optimized | display lists, 124-tooth instancing, static belt strips, culling, hoisted trig, frame-time readout | FR-15 |
| **No pre-computed animation** | two state variables, everything else closed-form | FR-2 |
| Interactive or clearly indicated | eleven key actions, HUD, stack light | FR-13, FR-14 |
| Feasible for one student | primitives only, closed-form maths, no physics or meshes | — |

Four taught topics are claimed here that a reader might not notice the project covers, and each deserves a sentence in the report rather than being left implicit:

- **Hidden-surface elimination**, by depth buffering and back-face culling (FR-15). Culling here also resolves the scene's coincident opposite-facing surfaces.
- **Z-fighting prevention**, three times over: the measured near plane in §4.1, opposite-facing coincident pairs, and deliberate gaps between same-facing surfaces (§4.3).
- **The emission term `I_e`**, which nothing in a scene like this normally uses, by the stack light (FR-10).
- **Temporal aliasing**, by the gear train appearing to freeze at a predictable speed (FR-3). It is the sampling theorem made visible, and the frame-rate readout makes it checkable.

## 7. Acceptance criteria

The project is done when all of the following are true. Each is stated so it can be checked by looking at the screen rather than by opinion.

Doubling the speed setting halves the wall-clock time per part and doubles every gear's speed. It leaves unchanged the press stroke, the stamped height, and the belt travel per part, which stays exactly one pitch. That last clause is the important one: it is only true if belt travel is derived from `θ` rather than from time.

Belt travel `B` is strictly constant in every frame in which the punch face is below 3.20. Assert this literally in code for one run. The punch is inside the blank zone for `θ ∈ (132.03°, 227.97°)` and INDEX occupies `(315°, 45°)`, so the assertion should never fire; if it does, a phase boundary has been mistyped. Check at both 6 and 120 ppm, because a wide `dt` at the top speed is the case that would step over a boundary.

Across a cycle stepped 5° at a time with the `.` key, belt travel never jumps, the Geneva wheel does not move outside INDEX, and in wireframe the driver pin is visibly inside a slot throughout INDEX. At the frame after `θ` wraps, the belt continues from exactly where it was.

Stepping a full cycle in wireframe, no gear tooth passes through another at any of the four meshes.

Running the program with `dt` artificially inflated (drag the window, or pause on a breakpoint for a second) does not put the belt out of step with the press, because `dt` is clamped, the wrap is a `while` loop, and every phase is a function of `θ` rather than a stored state.

At a displayed frame rate `F`, setting the speed nearest `F / 0.6` ppm makes all five gears appear stationary while the crank, ram and belt visibly continue. Slightly below that speed, the gears appear to turn backwards.

At the default camera during APPROACH, switching from Gouraud to Phong produces a visibly brighter, tighter highlight on the punch face and blank tops. It is tighter for two compounding reasons: per-fragment evaluation, and the change from the half-vector model to the reflection-vector model. Switching to flat produces visible facet boundaries on the rollers and crankshaft.

In flat or Gouraud mode, turning `GL_NORMALIZE` off makes the stamped blanks' tops blow out to white and their rims lose their highlight, while unstamped blanks on the same belt are unchanged. Turning it back on restores agreement. In Phong mode the key behaves as FR-13 specifies.

Frame time is stable and under 16 ms with the laptop plugged in, and it does not drift upward over several minutes of running, which would indicate a leak or unbounded instance growth.

Every object in §4.2 marked as static appears exactly once in a display list, and `grep` for geometry-emitting calls finds no duplicated part definitions.

## 8. Risks

Ranked by how much time they can cost if unanticipated.

**GLSL entry points are not available from `opengl32.dll` on Windows.** Windows ships OpenGL 1.1 only, so `glCreateShader` and friends must be resolved at runtime. Mitigation: link GLEW (`pacman -S mingw-w64-ucrt-x86_64-glew`, add `-lglew32`) and call `glewInit()` immediately after `glutCreateWindow`. Without this the shader phase cannot start, so it is resolved in Phase 0 rather than discovered in Phase 5.

**The interlock silently violating its own rule.** This is the highest-value risk in the document, because it is invisible. The line looks as if it is working whether or not the belt moves under a descending punch, and only the assertion in §7 will tell you. The interlock is a *derived* constraint. Change `r`, `L`, the ram height, the blank heights, the belt height or the slot count, and the blank-zone window in FR-5 moves, so the phase boundaries must be recomputed. Mitigation: keep the derivation in FR-5 next to the numbers, and re-run the assertion after any change to the press stack.

**Belt travel jumping a pitch at the wrap.** Counting indices from `cycles` directly instead of from the end of INDEX produces a one-pitch jump at top dead centre once per part. It is easy to miss at speed and obvious at 6 ppm. Mitigation: the definition in FR-5, and the step-key check in §7.

**Tooth phase errors.** A missing `π/N_j`, a wrong `ψ_ij`, or evaluating the chain in the wrong order leaves teeth permanently interpenetrating. It looks like a modelling fault and survives casual viewing at speed. Mitigation: build the train from the one formula in FR-3, then step a full cycle in wireframe at each mesh before adding anything else.

**Sign conventions in the Geneva and belt.** Mixing clockwise-positive `θ` with anticlockwise-positive `glRotatef` angles makes the wheel turn the wrong way, the pin leave its slot, or the belt run backwards while the blanks still advance. Mitigation: one convention, stated in FR-3, used everywhere; and the pin-in-slot check in §7.

**Light position transformed by the modelview matrix.** This produces lights that appear to move with objects. Mitigation: one `setup_lights()` function, called from exactly one place, immediately after the camera transform.

**Spotlight cone invisible under fixed-function shading.** Because the cone is evaluated per vertex, an unsubdivided floor or panel shows no cone at all. Mitigation: the subdivision in FR-10, and the expectation that the cone looks much better in Phong mode — itself a demonstration worth pointing out.

**Rectangular teeth intersecting at the mesh.** Block teeth lack involute clearance, so near the line of centres their corners can cross even when the phase is correct. Mitigation: tooth width at 0.45 of the circular pitch, narrowed further if the wireframe step check shows contact.

**The normalize demonstration contaminated by other scaled geometry.** If any other part is drawn through `glScalef`, turning `GL_NORMALIZE` off changes it too, and the demonstration stops being about the blanks. Mitigation: §4.3's rule that the squash is the frame's only scale, checked with `grep`.

**The gear freeze mistaken for a bug.** An examiner who speeds the line up to around 100 ppm sees the gears stop. Mitigation: it is documented in FR-3 as an observation, the frame rate is on the HUD, and it should be demonstrated deliberately before anyone finds it by accident.

**Black plastic rendering the motor and hood unreadable.** Mitigation and the documentation requirement are in FR-11.

**Include order and initialisation order for GLEW.** `<GL/glew.h>` must be included *before* `<GL/freeglut.h>`, or the compiler reports a wall of redefinition errors that look like a broken installation. `glewInit()` must be called *after* `glutCreateWindow`, because it needs a live context, and its return value must be checked. Mitigation: Phase 0 exists partly for this.

**The Geneva wheel's slots proving fiddly to draw.** It is the least important *geometry* in the mechanism, even though its motion matters most. Mitigation: if the slot bars resist, draw the wheel as a plain four-armed cross and keep the pin visible in front of it. The motion, which is all the requirement marks, is unaffected.

**Thermal throttling on battery.** The i3-1215U is a 15 W part. Mitigation: demo plugged in, with Windows set to Best Performance. Because of FR-1 this affects smoothness only, never correctness — though it does change the frame rate, and with it the speed at which the gears appear to freeze.

## 9. Deferred — not in scope

Recorded so the decisions are visible rather than forgotten, in the order I would add them if time remains after acceptance:

1. **A rotating amber beacon** on the stack light: a third spotlight whose `GL_SPOT_DIRECTION` is rotated every frame and switched on during STAMP. It is cheap, adds a moving light to a scene whose lights are otherwise fixed, and exercises the per-frame spot-direction rule in FR-10 a second time.
2. **Finished parts dropping into the bin** along a parabola whose parameter is the index progress `g`, instead of vanishing under the hood — a closed-form projectile path, so it keeps FR-2 intact.
3. **A 2D HUD inset with a deliberately undersized clip window**, running real Cohen–Sutherland on line segments and Sutherland–Hodgman on polygons, plus an explicit window-to-viewport mapping. This is the only way to make the whole clipping chapter visible, since the GPU clips where nobody can see it. It is the highest-value deferred item for marks, but also the most work.
4. **A two-link pick-and-place arm** moving finished parts to the bin, with joint angles from closed-form two-link inverse kinematics of a target point that depends on `θ`. It must not blend between stored poses, which would break the no-pre-computed-animation rule.
5. **A V-belt from a separate motor pulley to the pinion**, reusing FR-6's path function for a second belt loop.
6. **A hand-built cavalier and cabinet oblique projection** of the gear train, loaded via `glMultMatrixf`, since OpenGL provides no call for oblique projection and it is something the API demonstrably cannot do for you.
7. **A mirror reflection** of the line in a polished steel floor plate, built as `M_N,P` from the slide's own `T_−P⁻¹ · A_N⁻¹ · M_xy · A_N · T_−P` composition. It would also demonstrate that a single root-level matrix mirrors an entire articulated hierarchy.
8. **A four-viewport CAD-style layout** covering orthographic multiview and isometric projection properly, rather than through camera presets.

## 10. Verification record

Every derived number in this document was checked numerically against the geometry it claims to follow from. The results below are what `tests/mathcheck.cpp` must assert. They are recorded because several are the kind of thing an examiner may probe, and a stated check is stronger than an unstated assumption.

**Belt.** Pitch `0.2π = 0.62832`, roller-centre distance `10p = 6.2832`, tail roller at x −2.2832, loop length 15.0796 = 24.000000 pitches.

**Press.** `s` runs from 4.0000 to 3.5000, a stroke of 0.5000. The punch face runs from 3.6000 at top dead centre to 3.1000 at bottom dead centre, clearing an unstamped blank by 0.4000. `L/r = 4.0`, and the largest rod obliquity is 14.48°.

**Blank-zone window.** The analytic root `cos θ = −0.66957` gives (132.03°, 227.97°). A 0.01° scan agrees, finding contact from 132.04° to 227.96°.

**Geneva.** `c = 0.7778`, wheel radius 0.5500, `β(±45°) = ±45°` exactly. The windows are disjoint, with 87.03° of margin on each side, and the smallest punch clearance during INDEX is 0.3425. At every 0.1° of INDEX the driver pin lies on one of the wheel's slot lines, to within 2 × 10⁻¹³ degrees, which confirms the sign conventions of FR-5. The pin's deepest radius is 0.2278.

**Belt travel.** Simulated over three revolutions in 0.2° steps from the reset state, `B` is monotonic, never changes by more than 0.0054 pitch in a step (no jump at 45° or at the wrap), and ends at exactly 3.000000. Reset at `θ = 90°` gives `B = 0`.

**Gear train.**
- The G2–G5 distance is 3.6432. The two-idler reach is 3.8000, while a single 24-tooth idler reaches only 3.0000.
- All four meshes sit at exactly their pitch-radius sums: 1.2000, 1.4000, 1.0000 and 1.4000.
- The narrowest non-meshing gaps are 0.867, for G2–G4 and G3–G5.
- Tooth-to-gap alignment holds at every mesh for every `θ`, to within 3 × 10⁻¹⁴ of a tooth.
- Speed ratios are +3.0, −1.0, +1.8, −1.8 and +1.0.
- Tooth-passing rate is 36 teeth per radian of `θ` for all five gears.

**Clearances.**
- Geneva shaft to belt surface: 0.318.
- G2's lowest tooth tip: 3.800, above the rail bracket.
- G3's lowest tooth tip: 3.526, above the belt; its left edge is at 2.792, clear of the guide rails.
- Geneva wheel to press station: 1.335.
- Stamped blank to cleat: 0.045.
- The fresh blank is revealed at `g ≥ 0.6048`.

**Squash.** Volume is preserved exactly. The unnormalised normal lengths at full squash are 0.707 on the walls and 2.000 on the caps.

**Camera.** Every key point — tail roller, magazine, motor, stack light, Geneva driver, bin, panel corners, conveyor legs — projects inside the frame at a 16:9 aspect. The nearest visible geometry is 9.23 away and the farthest is 16.76.

**Speed.** At 60 fps the tooth-passing rate is 0.30 teeth per frame at 30 ppm, 1.00 at 100 ppm (apparent standstill), and 1.20 at 120 ppm. INDEX lasts 30 frames at the default and 7.5 at the top speed. One clamped `dt` at 120 ppm is 72°.

---

*Next document: `IMPLEMENTATION-PLAN.md` — architecture, phases, schedule, and cut order.*
