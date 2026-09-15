// Headless verification of every derived number in PRD 10.  It includes the
// same kinematics.h the renderer does, so this checks the shipping code rather
// than a transcription of it.
#include <cmath>
#include <cstdio>
#include "../src/kinematics.h"

using namespace cfg;
using namespace kin;

static int fails = 0;
static void ck(bool ok, const char* what) {
    printf("  %-58s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok) ++fails;
}
static bool near(float a, float b, float tol) { return fabsf(a - b) <= tol; }

int main() {
    printf("\n-- belt ----------------------------------------------------\n");
    printf("  pitch %.6f  span %.6f  tail x %.6f  loop %.6f = %.6f p\n",
           pitch(), span(), tail_x(), loop_len(), loop_len() / pitch());
    ck(near(pitch(), 0.628319f, 1e-5f), "pitch = 0.2*pi");
    ck(near(span(), 6.283185f, 1e-5f), "roller centres 10p apart");
    ck(near(tail_x(), -2.283185f, 1e-5f), "tail roller at x -2.2832");
    ck(near(loop_len() / pitch(), 24.0f, 1e-4f), "loop is 24.000000 pitches");
    ck(near(station_x(1), -1.6549f, 1e-3f), "station 1 under the magazine");
    ck(near(station_x(PRESS_STATION), PRESS_X, 1e-4f), "station 7 at the press");
    ck(near(station_x(10), HEAD_X, 1e-4f), "station 10 at the head roller");

    printf("\n-- press ---------------------------------------------------\n");
    const float smax = ram_top(0.0f), smin = ram_top(PI);
    printf("  s %.4f -> %.4f  stroke %.4f  punch %.4f -> %.4f\n",
           smax, smin, smax - smin, punch_face(0.0f), punch_face(PI));
    ck(near(smax, 4.0f, 1e-4f) && near(smin, 3.5f, 1e-4f), "s runs 4.0000 to 3.5000");
    ck(near(smax - smin, 2.0f * CRANK_R, 1e-5f), "stroke is exactly 2r");
    ck(near(punch_face(0.0f) - (BELT_TOP_Y + BLANK_H), 0.40f, 1e-4f),
       "punch clears an unstamped blank by 0.4000 at TDC");
    ck(near(punch_face(PI), BELT_TOP_Y + BLANK_H_FLAT, 1e-4f),
       "punch face at BDC = stamped blank top, 3.1000");
    ck(near(ROD_L / CRANK_R, 4.0f, 1e-5f), "L/r = 4.0");
    ck(near(asinf(CRANK_R / ROD_L) * DEG, 14.48f, 0.01f), "max obliquity 14.48 deg");
    {   // s falls monotonically across (0, pi) and rises monotonically back
        bool mono = true;
        for (int i = 1; i <= 1800; ++i) {
            const float a = PI * (i - 1) / 1800.0f, b = PI * i / 1800.0f;
            if (ram_top(b) > ram_top(a) + 1e-6f) mono = false;
        }
        ck(mono, "s is monotonic on (0, pi)");
    }

    printf("\n-- blank zone ----------------------------------------------\n");
    printf("  cos = %.6f  window %.3f to %.3f deg\n",
           blank_zone_cos(), blank_zone_lo_deg(), blank_zone_hi_deg());
    ck(near(blank_zone_cos(), -0.669565f, 1e-5f), "analytic root cos = -0.66957");
    ck(near(blank_zone_lo_deg(), 132.03f, 0.02f), "contact at 132.03 deg");
    {   // a 0.01 deg scan must agree with the analytic root
        float lo = 999.0f, hi = -999.0f;
        for (int i = 0; i < 36000; ++i) {
            const float d = i * 0.01f;
            if (punch_face(d * RAD) < BELT_TOP_Y + BLANK_H) {
                if (d < lo) lo = d;
                if (d > hi) hi = d;
            }
        }
        printf("  0.01 deg scan finds contact from %.2f to %.2f deg\n", lo, hi);
        ck(lo >= blank_zone_lo_deg() && lo <= blank_zone_lo_deg() + 0.02f
           && hi <= blank_zone_hi_deg() && hi >= blank_zone_hi_deg() - 0.02f,
           "scan agrees with the analytic window");
    }

    printf("\n-- geneva --------------------------------------------------\n");
    const float E = engage_half() * DEG;
    printf("  c %.4f  wheel r %.4f  engagement +/-%.1f deg\n",
           gen_centre_dist(), gen_wheel_r(), E);
    ck(near(gen_centre_dist(), 0.7778f, 1e-3f), "c = a/sin(pi/n) = 0.7778");
    ck(near(gen_wheel_r(), 0.5500f, 1e-4f), "wheel radius 0.5500");
    ck(near(geneva_beta(E * RAD) * DEG, 45.0f, 1e-3f), "beta(+45) = +45 exactly");
    ck(near(geneva_beta(-E * RAD) * DEG, -45.0f, 1e-3f), "beta(-45) = -45 exactly");
    ck(near(gen_centre_dist() - GEN_A, 0.2278f, 1e-3f), "pin's deepest radius 0.2278");
    {   // the index window and the blank zone must be disjoint, with margin
        const float margin = blank_zone_lo_deg() - E;
        printf("  margin from end of INDEX to first contact: %.2f deg\n", margin);
        ck(margin > 80.0f, "INDEX sits inside the safe region with margin");
        // smallest punch clearance during INDEX, at its two edges
        float minc = 1e9f;
        for (int i = 0; i <= 900; ++i) {
            const float d = 360.0f - E + i * (2.0f * E) / 900.0f;
            const float th = fmodf(d, 360.0f) * RAD;
            const float c = punch_face(th) - (BELT_TOP_Y + BLANK_H);
            if (c < minc) minc = c;
        }
        printf("  smallest punch clearance during INDEX: %.4f\n", minc);
        ck(near(minc, 0.3425f, 2e-3f), "smallest INDEX clearance 0.3425");
    }
    {   // at every 0.1 deg of INDEX the pin must lie on one of the slot lines
        float worst = 0.0f;
        for (int i = 0; i <= 900; ++i) {
            const float alpha = (-E + i * (2.0f * E) / 900.0f) * RAD;
            const float th = alpha < 0.0f ? alpha + 2.0f * PI : alpha;
            const float B = belt_travel(th, alpha < 0.0f ? 0 : 1);
            // pin, relative to the wheel centre
            const float c = gen_centre_dist();
            const float loc = GEN_LOC_DEG * RAD;
            const float ax = c * cosf(loc - PI), ay = c * sinf(loc - PI);
            const float arm = loc + alpha;
            const float px = ax + GEN_A * cosf(arm), py = ay + GEN_A * sinf(arm);
            const float pin_deg = atan2f(py, px) * DEG;
            // nearest slot line of the wheel at this travel
            float e = fmodf(pin_deg - wheel_angle_deg(B), 90.0f);
            if (e < 0.0f) e += 90.0f;
            if (e > 45.0f) e -= 90.0f;
            if (fabsf(e) > worst) worst = fabsf(e);
        }
        printf("  worst pin-to-slot-line error over INDEX: %.3e deg\n", worst);
        ck(worst < 1e-3f, "pin lies on a slot line throughout INDEX");
    }

    printf("\n-- belt travel ---------------------------------------------\n");
    {   // three revolutions from the reset state, 0.2 deg steps
        float th = RESET_THETA_DEG * RAD;
        long cyc = 0;
        float prev = belt_travel(th, cyc), jump = 0.0f;
        bool mono = true;
        ck(near(prev, 0.0f, 1e-6f), "reset at theta = 90 gives B = 0");
        for (int i = 1; i <= 3 * 1800; ++i) {
            th += 0.2f * RAD;
            while (th >= 2.0f * PI) { th -= 2.0f * PI; ++cyc; }
            const float B = belt_travel(th, cyc);
            if (B < prev - 1e-6f) mono = false;
            if (B - prev > jump) jump = B - prev;
            prev = B;
        }
        printf("  after 3 revolutions B = %.6f, largest step %.6f pitch\n",
               prev, jump);
        ck(mono, "B is monotonic across both 45 deg and the wrap");
        ck(jump < 0.006f, "no jump: largest step under 0.006 pitch");
        ck(near(prev, 3.0f, 1e-4f), "B ends at exactly 3.000000");
    }
    {   // PRD 7: B must be strictly constant whenever the punch is in the zone
        long faults = 0;
        for (float ppm = PPM_MIN; ppm <= PPM_MAX + 0.1f; ppm += PPM_STEP) {
            float th = RESET_THETA_DEG * RAD;
            long cyc = 0;
            float prev = belt_travel(th, cyc);
            const float dth = ppm * 2.0f * PI / 60.0f * DT_CLAMP;  // worst case
            for (int i = 0; i < 4000; ++i) {
                th += dth;
                while (th >= 2.0f * PI) { th -= 2.0f * PI; ++cyc; }
                const float B = belt_travel(th, cyc);
                if (punch_face(th) < BELT_TOP_Y + BLANK_H && B != prev) ++faults;
                prev = B;
            }
        }
        printf("  interlock faults over the whole speed range: %ld\n", faults);
        ck(faults == 0, "belt never moves while the punch is in the blank zone");
    }

    printf("\n-- gear train ----------------------------------------------\n");
    {
        const float dx = LAY.g[4].cx - LAY.g[1].cx;
        const float dy = LAY.g[4].cy - LAY.g[1].cy;
        printf("  G2-G5 distance %.4f   two-idler reach %.4f\n",
               sqrtf(dx*dx + dy*dy), 2.0f*(LAY.g[1].r + LAY.g[2].r) + 1.0f);
        printf("  G3 (%.4f, %.4f)   G4 (%.4f, %.4f)\n",
               LAY.g[2].cx, LAY.g[2].cy, LAY.g[3].cx, LAY.g[3].cy);
        ck(near(sqrtf(dx*dx + dy*dy), 3.6432f, 1e-3f), "G2-G5 distance 3.6432");
        ck(near(LAY.g[2].cx, 3.342f, 2e-3f) && near(LAY.g[2].cy, 4.076f, 2e-3f),
           "G3 derived at (3.342, 4.076)");
        ck(near(LAY.g[3].cx, 4.010f, 2e-3f) && near(LAY.g[3].cy, 3.332f, 2e-3f),
           "G4 derived at (4.010, 3.332)");
    }
    {   // every mesh sits at exactly its pitch-radius sum
        bool ok = true;
        for (int j = 0; j < 5; ++j) {
            const int i = LAY.drivenBy[j];
            if (i < 0) continue;
            const float dx = LAY.g[j].cx - LAY.g[i].cx;
            const float dy = LAY.g[j].cy - LAY.g[i].cy;
            const float d = sqrtf(dx*dx + dy*dy), want = LAY.g[i].r + LAY.g[j].r;
            printf("  G%d-G%d  %.4f (want %.4f)\n", i+1, j+1, d, want);
            if (!near(d, want, 1e-3f)) ok = false;
        }
        ck(ok, "all four meshes at their pitch-radius sums");
    }
    {   // narrowest gap between gears that do NOT mesh, allowing for tooth tips
        float worst = 1e9f;
        for (int a = 0; a < 5; ++a) for (int b = a + 1; b < 5; ++b) {
            if (LAY.drivenBy[a] == b || LAY.drivenBy[b] == a) continue;
            const float dx = LAY.g[a].cx - LAY.g[b].cx;
            const float dy = LAY.g[a].cy - LAY.g[b].cy;
            const float gap = sqrtf(dx*dx + dy*dy)
                            - (LAY.g[a].r + MODULE) - (LAY.g[b].r + MODULE);
            if (gap < worst) worst = gap;
        }
        printf("  narrowest non-meshing gap %.4f\n", worst);
        ck(near(worst, 0.867f, 5e-3f), "narrowest non-meshing gap 0.867");
    }
    {   // speed ratios and tooth-passing rate
        float p0[5], p1[5];
        const float h = 1e-3f;
        gear_angles(LAY.g, LAY.drivenBy, 0.0f, p0);
        gear_angles(LAY.g, LAY.drivenBy, h,    p1);
        const float want[5] = { 3.0f, -1.0f, 1.8f, -1.8f, 1.0f };
        bool ok = true, rate = true;
        for (int i = 0; i < 5; ++i) {
            const float ratio = (p1[i] - p0[i]) / h;
            printf("  G%d  N %2d  r %.2f  ratio %+6.3f  teeth/rad %.2f\n",
                   i + 1, LAY.g[i].N, LAY.g[i].r, ratio,
                   fabsf(ratio) * LAY.g[i].N);
            if (!near(ratio, want[i], 1e-2f)) ok = false;
            if (!near(fabsf(ratio) * LAY.g[i].N, 36.0f, 0.2f)) rate = false;
        }
        ck(ok, "speed ratios +3.0, -1.0, +1.8, -1.8, +1.0");
        ck(rate, "36 teeth per radian of theta for all five gears");
    }
    {   // tooth-to-gap alignment: u_i + u_j = 1/2 (mod 1) at every mesh, always
        float worst = 0.0f;
        for (int s = 0; s < 720; ++s) {
            float phi[5];
            gear_angles(LAY.g, LAY.drivenBy, s * 0.5f * RAD, phi);
            for (int j = 0; j < 5; ++j) {
                const int i = LAY.drivenBy[j];
                if (i < 0) continue;
                const float psi = atan2f(LAY.g[j].cy - LAY.g[i].cy,
                                         LAY.g[j].cx - LAY.g[i].cx);
                const float ui = (phi[i] - psi) / (2.0f * PI / LAY.g[i].N);
                const float uj = (phi[j] - psi - PI) / (2.0f * PI / LAY.g[j].N);
                float e = fmodf(ui + uj + 0.5f, 1.0f);
                if (e < 0.0f) e += 1.0f;
                if (e > 0.5f) e -= 1.0f;
                if (fabsf(e) > worst) worst = fabsf(e);
            }
        }
        printf("  worst tooth-to-gap error: %.3e of a tooth\n", worst);
        ck(worst < 1e-4f, "tooth faces a gap at every mesh, at every theta");
    }

    printf("\n-- squash --------------------------------------------------\n");
    {
        const float q = squash_q(BLANK_H_FLAT);
        const float r2 = BLANK_R / sqrtf(q);
        printf("  full squash: q %.3f  radius %.4f  wall |N| %.3f  cap |N| %.3f\n",
               q, r2, sqrtf(q), 1.0f / q);
        ck(near(r2, 0.2546f, 1e-3f), "stamped radius 0.2546");
        ck(near(BLANK_R*BLANK_R*BLANK_H, r2*r2*BLANK_H_FLAT, 1e-6f),
           "volume preserved exactly");
        ck(near(sqrtf(q), 0.707f, 1e-3f) && near(1.0f / q, 2.0f, 1e-4f),
           "unnormalised |N| is 0.707 on walls, 2.000 on caps");
        const float halfgap = 0.5f * (pitch() - CLEAT_W);
        printf("  clear half-gap between cleats %.4f, margin %.4f\n",
               halfgap, halfgap - r2);
        ck(near(halfgap, 0.2992f, 1e-3f), "clear half-gap 0.2992");
        ck(near(halfgap - r2, 0.045f, 2e-3f), "stamped blank to cleat 0.045");
    }
    {   // What must be continuous is the *scene*, not the label-to-height map.
        // press_blank_h is discontinuous at 45 deg by design: the labels shift
        // there, so label 7 stops being the finished part leaving the press and
        // becomes the new arrival.  The test is therefore positional - the
        // blank at a given place on the belt must not change height or jump -
        // which is what an examiner would actually see.
        float px[16], ph[16], worst_h = 0.0f, worst_x = 0.0f;
        int pn = 0;
        for (int i = 0; i <= 36000; ++i) {
            const float th = i * 0.01f * RAD;
            const float g = index_progress(th);
            float x[16], h[16];
            int n = 0;
            for (int j = 1; j <= LABELS; ++j) {
                x[n] = tail_x() + (j + g) * pitch();
                h[n] = blank_h(j, th);
                ++n;
            }
            for (int a = 0; a < n && pn; ++a) {
                int best = 0;
                for (int b = 1; b < pn; ++b)
                    if (fabsf(px[b] - x[a]) < fabsf(px[best] - x[a])) best = b;
                const float dx = fabsf(px[best] - x[a]);
                const float dh = fabsf(ph[best] - h[a]);
                // the blank entering at station 1 has no predecessor to match
                if (x[a] > station_x(1) + 0.5f * pitch()) {
                    if (dx > worst_x) worst_x = dx;
                    if (dh > worst_h) worst_h = dh;
                }
            }
            for (int a = 0; a < n; ++a) { px[a] = x[a]; ph[a] = h[a]; }
            pn = n;
        }
        printf("  over a 0.01 deg scan, a blank's largest step: dx %.2e, dh %.2e\n",
               worst_x, worst_h);
        ck(worst_h < 1e-4f, "no blank changes height abruptly, 45 deg included");
        ck(worst_x < 1e-3f, "no blank jumps along the belt");
        // and the handover itself, stated explicitly
        const float before = blank_h(PRESS_STATION - 1, (E - 0.001f) * RAD);
        const float after  = blank_h(PRESS_STATION,     (E + 0.001f) * RAD);
        printf("  at the handover, station 7 holds h %.3f then %.3f\n",
               before, after);
        ck(near(before, after, 1e-4f), "station 7's blank is unchanged at 45 deg");
    }

    printf("\n-- clearances ----------------------------------------------\n");
    {
        const float d = gen_centre_dist();
        printf("  Geneva shaft to belt surface %.4f\n", d - BELT_R_O - GEN_SHAFT_R);
        ck(near(d - BELT_R_O - GEN_SHAFT_R, 0.318f, 2e-3f),
           "Geneva shaft to belt surface 0.318");
        ck(near(CRANK_Y - (LAY.g[1].r + MODULE), 3.80f, 1e-3f),
           "G2's lowest tooth tip at 3.800, above the rail bracket");
        printf("  G3 lowest tip %.4f  left edge %.4f\n",
               LAY.g[2].cy - (LAY.g[2].r + MODULE),
               LAY.g[2].cx - (LAY.g[2].r + MODULE));
        ck(LAY.g[2].cy - (LAY.g[2].r + MODULE) > BELT_TOP_Y,
           "G3's lowest tooth tip is above the belt");
        ck(LAY.g[2].cx - (LAY.g[2].r + MODULE) > PRESS_X + RAIL_X_OFF,
           "G3's left edge is clear of the guide rails");
        ck(near(HEAD_X - gen_wheel_r() - PRESS_X, 1.335f, 2e-3f),
           "Geneva wheel to press station 1.335");
        ck(near(FRESH_G, 0.605f, 1e-3f), "fresh blank revealed at g = 0.605");
        printf("  departing blank has moved %.4f when the fresh one appears\n",
               FRESH_G * pitch());
        ck(near(FRESH_G * pitch(), 2.0f * BLANK_R + 0.02f, 3e-3f),
           "that is two radii plus a 0.02 gap");
    }

    printf("\n-- speed ---------------------------------------------------\n");
    printf("  at 60 fps: %.2f teeth/frame at 30 ppm, %.2f at 100, %.2f at 120\n",
           0.6f * 30.0f / 60.0f, 0.6f * 100.0f / 60.0f, 0.6f * 120.0f / 60.0f);
    printf("  INDEX lasts %.1f frames at 30 ppm, %.1f at 120 ppm\n",
           60.0f * (2.0f * E / 360.0f) / (30.0f / 60.0f),
           60.0f * (2.0f * E / 360.0f) / (120.0f / 60.0f));
    printf("  one clamped dt at 120 ppm is %.0f deg\n",
           120.0f / 60.0f * 360.0f * DT_CLAMP);
    ck(near(0.6f * 100.0f / 60.0f, 1.0f, 1e-4f),
       "gears appear to stand still at 100 ppm on a 60 fps display");

    printf("\n%s  (%d failure%s)\n\n", fails ? "FAILED" : "ALL CHECKS PASSED",
           fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
