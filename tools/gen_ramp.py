#!/usr/bin/env python3
"""Erzeugt src/ramp_path.h und src/ramp_path.c: den Weg der Rampe als Kette
kurzer gerader Stuecke (Festkomma 16.16).

Verlauf (Wunsch vom 25.10.2026): Einfahrt links etwa in der Mitte des Tisches,
hinten als Bruecke um den Tisch herum, rechts herunter und Ende unterhalb des
dritten Flippers, so dass der Ball zum linken Flipper rollt.

  Gerade hoch   (51,352) -> (51,122)
  Bogen r=40    nach rechts, Mittelpunkt (91,122)  -> (91,82)
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

line(51, 352, 51, 122)
arc(91, 122, 40, 180, 270, 16)        # von links (180) nach oben (270): Richtung von hoch nach rechts
line(None, None, 200, 82)
arc(200, 122, 40, 270, 360, 16)       # von oben nach rechts: Richtung von rechts nach unten
line(None, None, 240, 300)
arc(200, 300, 40, 0, 45, 8)           # von rechts nach rechts unten: Richtung nach links unten

segs = []          # x0, y0, ux, uy, s0, len
s = 0.0
for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
    dx, dy = x1 - x0, y1 - y0
    L = math.hypot(dx, dy)
    segs.append((x0, y0, dx / L, dy / L, s, L))
    s += L
total = s
climb_end = 140.0                          # erste 140 Pixel: Ball wird gebremst
desc_start = segs[[i for i, g in enumerate(segs) if abs(g[0] - 240) < 1e-6 and abs(g[1] - 122) < 1e-6][0]][4]
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
    fh.write(f"#define RAMP_X0         51\n#define RAMP_Y0         352\n")
    fh.write(f"#define RAMP_EXIT_X     {f(ex)}L   /* {ex:.1f} */\n#define RAMP_EXIT_Y     {f(ey)}L   /* {ey:.1f} */\n")
    fh.write("extern const ramp_seg_t RAMP_SEGS[RAMP_NSEG];\n\n#endif\n")

with open(os.path.join(src, "ramp_path.c"), "w", newline="\n") as fh:
    fh.write("/* Automatisch erzeugt von tools/gen_ramp.py - nicht von Hand aendern */\n")
    fh.write('#include "ramp_path.h"\n\nconst ramp_seg_t RAMP_SEGS[RAMP_NSEG] = {\n')
    for x0, y0, ux, uy, s0, L in segs:
        fh.write(f"  {{ {f(x0)}L, {f(y0)}L, {f(ux)}L, {f(uy)}L, {f(s0)}L }},\n")
    fh.write("};\n")
print(f"{len(segs)} Stuecke, Laenge {total:.1f}, Abfahrt ab {desc_start:.1f}, Ende ({ex:.1f}, {ey:.1f})")
