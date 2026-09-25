# Making changes live — a demo survival guide

Branch `static-objects`. For changing something **while someone is watching**:
where to click, what to type, and what will bite you.

Keep this open next to the editor. For *why* a number is what it is, see
[OBJECTS.md](OBJECTS.md).

---

## The loop

```powershell
.\build.ps1 -Run       # build and run
.\build.ps1            # build only
```

Everything is `inline` in headers, so **any** change recompiles the whole
program — about two seconds. There is no hot reload: you must close the window
and run again.

> **If the build says `Permission denied`, the app is still running.** Close the
> window and build again. This is the single most common false alarm.

---

## "I want to change…" — where to go

| I want to… | Go to | Notes |
|---|---|---|
| move an object | [config.h](../src/config.h) | nearly everything is positioned by a named constant |
| resize an object | [config.h](../src/config.h) | same |
| change a colour | [materials.h](../src/materials.h) | one RGB triple per material |
| change the line's speed | `CRANK_DPS` in [config.h](../src/config.h) | degrees of crank per second |
| make things rounder / blockier | `CYL_SLICES` in [config.h](../src/config.h) | affects every cylinder |
| change the lighting | [main.cpp:42](../src/main.cpp#L42) | three arrays: bulb, off, ambient |
| move the camera | `EYE_*` / `AT_*` in [config.h](../src/config.h) | view `1`; `OVER_*` is view `2` |
| change the window size | `win_w`, `win_h` in [camera.h:16](../src/camera.h#L16) | |
| change the HUD text | [hud.h](../src/hud.h) `status_panel` | |
| add a whole new object | [room.h](../src/room.h) | see the worked example below |
| change a mechanism's motion | [layout.h](../src/layout.h) | read §7 of OBJECTS.md first |

---

## Recipe 1 — move an object

Most objects have their position typed in `config.h`. Find the constant, change
it, rebuild.

```cpp
constexpr float DRUM_X[2] = { 7.25f, 8.05f }, DRUM_Z[2] = { -1.25f, -0.95f };
constexpr float PAL_X = -4.10f, PAL_Z = 2.50f;      // the pallet
constexpr float STACK_X = 5.50f, STACK_Z = -0.80f;  // the stack light
constexpr float FAN_X = -1.70f, FAN_Y = 5.00f;      // the exhaust fan
constexpr float CAB_X0 = 6.20f, CAB_X1 = 7.60f;     // the cabinet
```

**To move the whole machine** without touching thirty constants, wrap the call
in [main.cpp:36](../src/main.cpp#L36):

```cpp
static void draw_world(const bool* sw, bool edge_pass = false) {
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 1.5f);     // shove the line towards the viewer
    scene::draw(theta, turns);
    glPopMatrix();
    room::draw(cam::eye_pos, sw, fan_deg, edge_pass);
}
```

The room stays put, so this is also the quickest way to show the machine and
the room are genuinely independent.

---

## Recipe 2 — change an object's shape

**Different size:** change its constants.

```cpp
constexpr float DRUM_R = 0.30f, DRUM_H = 0.88f;     // fatter, taller drums
```

**Different number of sides:** `cyl` takes an optional slice count, so one
object can be blockier than the rest. In [room.h:207](../src/room.h#L207):

```cpp
cyl(DRUM_R, DRUM_H, 6);      // hexagonal drums; default is CYL_SLICES = 10
```

**Different shape entirely:** swap the primitive. A drum built from `box_span`
instead of `cyl` becomes a crate. The six primitives are listed in §2 of
[OBJECTS.md](OBJECTS.md).

**Squash or stretch one instance:** wrap it.

```cpp
glPushMatrix();
glScalef(1.0f, 1.6f, 1.0f);    // 60% taller, same footprint
cyl(DRUM_R, DRUM_H);
glPopMatrix();
```

---

## Recipe 3 — change a colour

One line in [materials.h](../src/materials.h):

```cpp
constexpr Material MACHINE_PAINT = { { 0.26f, 0.48f, 0.36f } };   // green
```

Change it to `{ 0.70f, 0.20f, 0.15f }` and the panel, conveyor frame and guide
rails all turn red together — they share the material.

To recolour **one** object instead, call `mat::use()` with a different material
just before that object's geometry. Colour is set exactly one way everywhere:

```cpp
mat::use(mat::SIGNAL_RED);
```

---

## Recipe 4 — speed, stopping, and stepping

```cpp
constexpr float CRANK_DPS = 72.0f;    // 5 s per cycle. 36 = slow, 180 = fast
```

`Space` stops and starts the line at any time — no rebuild needed, and the best
way to freeze a mechanism mid-explanation.

**To add single-stepping**, in `keyboard()` at [main.cpp:113](../src/main.cpp#L113):

```cpp
case '.': theta += 5.0f * RAD;                 // nudge the crank 5 degrees
          while (theta >= 2.0f*PI) { theta -= 2.0f*PI; ++turns; }
          break;
```

Stop the line with `Space` first, then tap `.` to walk through one index one
frame at a time. Worth having ready if you are asked to show the Geneva
engaging.

---

## Recipe 5 — the lighting

Three arrays at [main.cpp:42](../src/main.cpp#L42):

```cpp
static const GLfloat BULB_DIFFUSE[4] = { 0.60f, 0.57f, 0.50f, 1.0f };  // each bulb
static const GLfloat LIGHT_OFF[4]    = { 0.00f, 0.00f, 0.00f, 1.0f };  // switched off
static const GLfloat ROOM_AMBIENT[4] = { 0.44f, 0.44f, 0.47f, 1.0f };  // the floor
```

- **Scene too dark / too flat** — raise or lower `ROOM_AMBIENT`. High ambient
  washes the shading out; low ambient makes the switches dramatic.
- **Warmer bulbs** — push `BULB_DIFFUSE` red up and blue down.
- **Show that the lights are real** — press `[` and `]`. The shading across the
  whole room changes, not just the bulb.
- **Move a bulb** — `BULB_X[2]`, `BULB_Y`, `BULB_Z` in `config.h`. The light and
  the glass both follow, because `place_lights()` reads the same constants.

There is no specular term by design, so there is no shininess to tune.

---

## Recipe 6 — add a new object (worked example)

Say you want a red toolbox on the floor.

**1.** Write the object in [room.h](../src/room.h), next to `drums()`:

```cpp
inline void toolbox() {
    mat::use(mat::SIGNAL_RED);
    box_span(-2.60f, 0.00f, 2.10f,  -2.00f, 0.35f, 2.50f);   // body
    mat::use(mat::GALVANIZED);
    box_span(-2.40f, 0.35f, 2.25f,  -2.20f, 0.42f, 2.35f);   // handle
}
```

**2.** Call it from `build_lists()` ([room.h:234](../src/room.h#L234)), inside
the floor-items list:

```cpp
glNewList(L(R_FLOOR_ITEMS), GL_COMPILE);
hazard_markings();
pallet_of_blanks();
cabinet();
drums();
toolbox();                 // <- here
glEndList();
```

**3.** Rebuild. That is all — it is lit, outlined by the edge pass, and depth
tested automatically, because it used the shared primitives. Those coordinates
put it on the floor beside the pallet, in frame in the default view.

*(Both this and the single-step key in Recipe 4 were compiled and run before
being written down here.)*

**If it should hang on a wall instead**, put the call inside that wall's list in
the `for` loop just below, and use the wall's own frame (`u` along the wall, `y`
up, `z` out into the room). It then inherits the cutaway for free.

**If it should move**, do not put it in a list at all — draw it from
`room::draw()` and give it an angle from `fan_deg` or the machine's `theta`.

---

## Recipe 7 — remove an object

Delete or comment out its call in `build_lists()`. Leave the geometry function
and its constants alone — they cost nothing and you can put it back in one
line if the examiner asks.

That is exactly how the chute and the bin came out, except those were deleted
properly because they were never coming back.

---

## Recipe 8 — the camera

```cpp
constexpr float EYE_X = 7.0f, EYE_Y = 6.2f, EYE_Z = 9.0f;   // view 1, the eye
constexpr float AT_X  = 1.4f, AT_Y  = 2.9f, AT_Z  = 0.0f;   // view 1, look-at
constexpr float OVER_EYE_X = 14.5f, ...                     // view 2, the room
```

Live, without rebuilding: `1` / `2` presets, `←` `→` orbit, `c` for the free
camera (then arrows fly, PgUp/PgDn rise and sink), `r` to reset.

> **The free camera has its own near plane** (`FREE_ZNEAR` 0.2 against `ZNEAR`
> 4.0). That is why you can fly right up to a gear in free mode but the preset
> views clip if you move them too close. If you move `EYE_*` in and geometry
> starts getting sliced away, that is the near plane, not a bug.

---

## The five things that will bite you

**1. Display lists are compiled once, at startup.** Geometry changes need a
rebuild *and* a restart. Nothing in this program re-reads a file at runtime.

**2. Back-face culling is on.** If a new face is invisible from the side you
expected, you wound it the wrong way round — reverse the vertex order. To
confirm that is the cause, comment out `glEnable(GL_CULL_FACE)` in
[main.cpp:162](../src/main.cpp#L162); if the face appears, it is winding.

**3. Lighting needs normals.** The six primitives all emit them. If you write a
raw `glBegin`/`glEnd` block yourself and forget `glNormal3f`, the object
renders with whatever normal was left over — usually flat and wrong.

**4. Colour inside a display list is baked in.** That is fine for anything
fixed. For anything that changes at runtime — a lamp following a switch — set
the colour *outside* the list, immediately before `glCallList`. The bulb globes
and stack-light lenses are the examples to copy.

**5. Coincident surfaces z-fight.** Anything lying on another surface needs a
nudge, the way the hazard border sits `HAZ_Y = 0.004` above the floor. If a
surface shimmers as the camera moves, that is what it is.

---

## When something goes wrong

| Symptom | Almost always |
|---|---|
| build: `cannot open output file … Permission denied` | the app is still running — close it |
| object doesn't appear at all | not called from `build_lists()`, or called before the list it draws from was built |
| object appears from one side only | winding — see bite #2 |
| object is flat black | no normals, or it is facing away from both bulbs with ambient turned down |
| object shimmers / flickers | z-fighting with a coincident surface — nudge it |
| geometry sliced away near the camera | the near plane (`ZNEAR` 4.0 in preset views) |
| a wall or its fittings vanish | working as intended — that is the cutaway |
| teeth stop meshing, pin cuts the wheel arms | you changed a coupled number — see §7 of [OBJECTS.md](OBJECTS.md) |

---

## Safe changes to have ready

Things you can change in ten seconds that visibly prove a point:

| Ask | Change | Shows |
|---|---|---|
| "can you slow it down?" | `CRANK_DPS` to 24 | the Geneva index, clearly |
| "is the lighting real?" | press `[` | shading across the whole room |
| "what is the shading doing?" | press `e` twice | edges off, then on |
| "are those really the same gears?" | look at the shelf | it reuses `L_GEAR0` and `L_TOOTH` |
| "how many polygons is a cylinder?" | `CYL_SLICES` to 24 | smoother, and a much busier edge pass |
| "can the machine move?" | the `glTranslatef` in Recipe 1 | machine and room are independent |
