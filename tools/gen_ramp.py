#!/usr/bin/env python3
"""Erzeugt src/ramp_path.h und src/ramp_path.c: den Weg der Rampe als Kette
kurzer gerader Stuecke (Festkomma 16.16).

Verlauf (Wuensche vom 25. und 26.10.2026, Stand v4.3): Einfahrt links,
hinten als Bruecke um den Tisch herum, rechts herunter und Ende ueber dem
dritten Flipper, auf den der Ball faellt.

  Einfahrt      senkrecht bei x 128, Start y 300 (v5.4), Muendung nach unten
  Bogen r=40    nach links oben, Schraege am Kick-Out-Hole vorbei,
  Bogen r=40    auf senkrecht, dann Gerade hoch bei x 45 bis y 122
  Bogen r=40    nach rechts -> (85,82)
  Gerade        (85,82)  -> (222,82)   Bruecke hinter den Bumpern
  Bogen r=40    nach unten, Mittelpunkt (222,122) -> (262,122)
  Gerade runter (262,122) -> (262,252) Ende ueber dem dritten Flipper
"""
import math, os

src = os.path.join(os.path.dirname(__file__), "..", "src")
FIX = 65536

pts = []           # (x, y) Stuetzpunkte
def line(x0, y0, x1, y1):
    if not pts: pts.append((x0, y0))     # sonst beginnt die Gerade am letzten Punkt
    pts.append((x1, y1))
def arc(cx, cy, r, a0, a1, n):
    for i in range(1, n + 1):
        a = math.radians(a0 + (a1 - a0) * i / n)
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))

# Einfahrt (v5.4): senkrecht, Muendung bei x 128 (dort fliegt der Standardschuss
# des linken Flippers hin, im PC-Test x 121-136). Der Weg biegt nach links ab,
# fuehrt schraeg am Kick-Out-Hole vorbei und trifft bei x 45 auf den Aufstieg.
CX, CY = 128.0, 300.0
UX, UY = 0.0, -1.0                               # Fahrtrichtung am Start: senkrecht nach oben
R = 40.0
line(CX, CY, CX, 285)
arc(CX - R, 285, R, 0, -45, 6)                   # nach links oben (45 Grad)
s45 = math.sqrt(0.5)
p_end = (45 + R * (1 - s45), 0)                  # Bogen 2 endet senkrecht bei x 45
start = pts[-1]
L = (start[0] - p_end[0]) / s45                  # Laenge der Schraegen
p2 = (start[0] - L * s45, start[1] - L * s45)
line(None, None, *p2)
c2 = (p2[0] + R * s45, p2[1] - R * s45)
arc(c2[0], c2[1], R, 135, 180, 6)                # zurueck auf senkrecht nach oben
xu = c2[0] - R                                   # 45
line(None, None, xu, 122)
arc(xu + R, 122, R, 180, 270, 16)                # von hoch nach rechts
line(None, None, 222, 82)
arc(222, 122, 40, 270, 360, 16)                  # von rechts nach unten
line(None, None, 262, 252)                       # v4.3: Ende ueber dem dritten Flipper, Ball faellt darauf

print("Schraege von (%.1f, %.1f) nach (%.1f, %.1f), Aufstieg ab y %.1f" % (start[0], start[1], p2[0], p2[1], c2[1]))

segs = []          # x0, y0, ux, uy, s0, len
s = 0.0
for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
    dx, dy = x1 - x0, y1 - y0
    L = math.hypot(dx, dy)
    segs.append((x0, y0, dx / L, dy / L, s, L))
    s += L
total = s
climb_end = 140.0                          # erste 140 Pixel: Ball wird gebremst
desc_start = segs[[i for i, g in enumerate(segs) if abs(g[0] - 262) < 1e-6 and abs(g[1] - 122) < 1e-6][0]][4]
ex, ey = pts[-1]

def f(v): return int(round(v * FIX))

with open(os.path.join(src, "ramp_path.h"), "w", newline="\n") as fh:
    fh.write("/* Automatisch erzeugt von tools/gen_ramp.py - nicht von Hand aendern */\n")
    fh.write("#ifndef RAMP_PATH_H\n#define RAMP_PATH_H\n#include <stdint.h>\n\n")
    fh.write("/* v6.4 kompakt: x0, y0 in 12.4, ux, uy in 2.14, s0 in 16.16 (spart 392 Byte) */\n")
    fh.write("typedef struct { int16_t x0, y0, ux, uy; int32_t s0; } ramp_seg_t;\n")
    fh.write("#define RS_P(v) ((int32_t)(v) << 12)   /* Lage nach 16.16 */\n")
    fh.write("#define RS_U(v) ((int32_t)(v) << 2)    /* Richtung nach 16.16 */\n")
    fh.write(f"#define RAMP_NSEG       {len(segs)}\n")
    fh.write(f"#define RAMP_LEN        {f(total)}L   /* {total:.1f} Pixel */\n")
    fh.write(f"#define RAMP_CLIMB_END  {f(climb_end)}L\n")
    fh.write(f"#define RAMP_DESC_START {f(desc_start)}L   /* {desc_start:.1f}: rechts abwaerts */\n")
    fh.write(f"#define RAMP_X0         {int(CX)}\n#define RAMP_Y0         {int(CY)}   /* Einfahrt: Ball kreuzt diese Zeile nach oben */\n")
    fh.write(f"#define RAMP_EXIT_X     {f(ex)}L   /* {ex:.1f} */\n#define RAMP_EXIT_Y     {f(ey)}L   /* {ey:.1f} */\n")
    fh.write("extern const ramp_seg_t RAMP_SEGS[RAMP_NSEG];\n\n#endif\n")

with open(os.path.join(src, "ramp_path.c"), "w", newline="\n") as fh:
    fh.write("/* Automatisch erzeugt von tools/gen_ramp.py - nicht von Hand aendern */\n")
    fh.write('#include "ramp_path.h"\n\nconst ramp_seg_t RAMP_SEGS[RAMP_NSEG] = {\n')
    for x0, y0, ux, uy, s0, L in segs:
        fh.write(f"  {{ {round(x0*16)}, {round(y0*16)}, {round(ux*16384)}, {round(uy*16384)}, {f(s0)}L }},\n")
    fh.write("};\n")
print(f"{len(segs)} Stuecke, Laenge {total:.1f}, Abfahrt ab {desc_start:.1f}, Ende ({ex:.1f}, {ey:.1f})")
