#!/usr/bin/env python3
"""Erzeugt src/tables.h und src/tables.c: Sinus (256 Schritte, 16.16) und atan-Oktant (65 Werte).
Identisch zu den Tabellen im Browser-Prototyp."""
import math, os
src = os.path.join(os.path.dirname(__file__), "..", "src")
sin = [round(math.sin(i * 2 * math.pi / 256) * 65536) for i in range(256)]
atan = [round(math.atan(i / 64) * 128 / math.pi) for i in range(65)]
with open(os.path.join(src, "tables.h"), "w", newline="\n") as f:
    f.write("/* Automatisch erzeugt von tools/gen_tables.py - nicht von Hand aendern */\n")
    f.write("#ifndef TABLES_H\n#define TABLES_H\n#include <stdint.h>\n\n")
    f.write("extern const int32_t SIN_TAB[256];   /* sin(i*2pi/256) in 16.16 */\n")
    f.write("extern const uint8_t ATAN_TAB[65];   /* atan(i/64) in 256er-Winkeln */\n")
    f.write("\n#endif\n")
with open(os.path.join(src, "tables.c"), "w", newline="\n") as f:
    f.write("/* Automatisch erzeugt von tools/gen_tables.py - nicht von Hand aendern */\n")
    f.write('#include "tables.h"\n\n')
    f.write("const int32_t SIN_TAB[256] = {\n")
    for i in range(0, 256, 8):
        f.write("  " + ", ".join(f"{v}L" for v in sin[i:i+8]) + ",\n")
    f.write("};\n\nconst uint8_t ATAN_TAB[65] = {\n")
    for i in range(0, 65, 16):
        f.write("  " + ", ".join(str(v) for v in atan[i:i+16]) + ",\n")
    f.write("};\n")
print("geschrieben: src/tables.h, src/tables.c")
