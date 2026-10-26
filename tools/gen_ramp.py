#!/usr/bin/env python3
"""Erzeugt src/ramp_path.h und src/ramp_path.c: den Weg der Rampe als Kette
kurzer gerader Stuecke (Festkomma 16.16).

Verlauf (Wunsch vom 25.10.2026): Einfahrt links etwa in der Mitte des Tisches,
hinten als Bruecke um den Tisch herum, rechts herunter und Ende unterhalb des
dritten Flippers, so dass der Ball zum linken Flipper rollt.

  Einfahrt      schraeg (21 Grad), Start (61,275), oeffnet nach rechts unten
  Bogen r=40    auf senkrecht, dann Gerade hoch bei x 45 bis y 122
  Bogen r=40    nach rechts -> (85,82)
  Gerade        (91,82)  -> (200,82)   Bruecke hinter den Bumpern
  Bogen r=40    nach unten, Mittelpunkt (200,122)  -> (240,122)
  Gerade runter (240,122) -> (240,300)
  Bogen r=40    nach links unten (45 Grad), Mittelpunkt (200,300) -> Ende
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

# Einfahrt (v4.1): schraeg, oeffnet sich nach rechts unten und zielt auf den rechten Flipper.
# Achse 21 Grad gegen die Senkrechte (verlaengert trifft sie y 548 bei x 166), Rampenstart C = (61, 275).
ANG = math.radians(21)
UX, UY = -math.sin(ANG), -math.cos(ANG)          # Fahrtrichtung bergauf (nach links oben)
CX, CY = 61.0, 275.0
R = 40.0
p1 = (CX + 15 * UX, CY + 15 * UY)                # 15 Pixel gerade, dann Bogen nach oben
nx, ny = -UY, UX                                 # rechte Normale (Bildschirm: y nach unten)
c1 = (p1[0] + R * nx, p1[1] + R * ny)
line(CX, CY, *p1)
a_start = math.degrees(math.atan2(p1[1] - c1[1], p1[0] - c1[0]))
arc(c1[0], c1[1], R, a_start, 180, 6)            # Richtung dreht auf senkrecht nach oben
xu = c1[0] - R                                   # senkrechte Gerade bei x = 45,2
line(None, None, xu, 122)
arc(xu + R, 122, R, 180, 270, 16)                # von hoch nach rechts
line(None, None, 222, 82)
arc(222, 122, 40, 270, 360, 16)                  # von rechts nach unten
line(None, None, 262, 252)                       # v4.3: Ende ueber dem dritten Flipper, Ball faellt darauf

# Waende der Einfahrt (fuer table.c): Muendung 40 Pixel unterhalb von C,
# halbe Breite 13, oben 15 Pixel ueber C hinaus
px, py = -UY, UX                                 # Querrichtung
mouth = (CX - 40 * UX, CY - 40 * UY)
top = (CX + 15 * UX, CY + 15 * UY)
lw = (mouth[0] - 13 * px, mouth[1] - 13 * py, top[0] - 13 * px, top[1] - 13 * py)
rw = (mouth[0] + 13 * px, mouth[1] + 13 * py, top[0] + 13 * px, top[1] + 13 * py)
print("linke Wand  %.1f %.1f -> %.1f %.1f" % lw)
print("rechte Wand %.1f %.1f -> %.1f %.1f" % rw)
print("Muendung %.1f %.1f" % mouth)
spin = (CX - 20 * UX, CY - 20 * UY)
print("Spinner Mitte %.1f %.1f  fix %d %d  P %d %d  U %d %d" % (spin[0], spin[1], round(spin[0]*65536), round(spin[1]*65536),
      round(px*65536), round(py*65536), round(UX*65536), round(UY*65536)))
def wall_x(w, y):
    t = (w[1] - y) / (w[1] - w[3]); return w[0] + t * (w[2] - w[0])
print("Gasse bei y %d: %.1f .. %.1f, bei Spinner y %.0f: %.1f .. %.1f" % (CY, wall_x(lw, CY), wall_x(rw, CY),
      spin[1], wall_x(lw, spin[1]), wall_x(rw, spin[1])))

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
    fh.write("typedef struct { int32_t x0, y0, ux, uy, s0; } ramp_seg_t;   /* 16.16 */\n")
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
        fh.write(f"  {{ {f(x0)}L, {f(y0)}L, {f(ux)}L, {f(uy)}L, {f(s0)}L }},\n")
    fh.write("};\n")
print(f"{len(segs)} Stuecke, Laenge {total:.1f}, Abfahrt ab {desc_start:.1f}, Ende ({ex:.1f}, {ey:.1f})")
