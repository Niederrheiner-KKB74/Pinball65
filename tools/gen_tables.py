#!/usr/bin/env python3
"""Erzeugt src/tables.h und src/tables.c: Sinus (256 Schritte, 16.16) und atan-Oktant (65 Werte).
Identisch zu den Tabellen im Browser-Prototyp."""
import math, os
src = os.path.join(os.path.dirname(__file__), "..", "src")
sin = [round(math.sin(i * 2 * math.pi / 256) * 65536) for i in range(256)]
atan = [round(math.atan(i / 64) * 128 / math.pi) for i in range(65)]
# Gespeichert wird nur ein Viertel (Werte 0..63, der Wert bei 64 ist 65536).
# sin_tab() rechnet daraus jeden Wert der vollen Tabelle aus (v6.4: spart 1 KB
# RAM gegenueber der ausgerechneten Tabelle); bitgleich, geprueft unten.
q = sin[:64]
full = []
for i in range(256):
    k = i & 127
    v = (q[k] if k < 64 else 65536) if k <= 64 else q[128 - k]
    full.append(v if i < 128 else -v)
assert full == sin, "Viertelwelle weicht ab"
with open(os.path.join(src, "tables.h"), "w", newline="\n") as f:
    f.write("/* Automatisch erzeugt von tools/gen_tables.py - nicht von Hand aendern */\n")
    f.write("#ifndef TABLES_H\n#define TABLES_H\n#include <stdint.h>\n\n")
    f.write("int32_t sin_tab(uint8_t i);          /* sin(i*2pi/256) in 16.16 */\n")
    f.write("extern const uint8_t ATAN_TAB[65];   /* atan(i/64) in 256er-Winkeln */\n")
    f.write("void tables_init(void);              /* vor allem anderen aufrufen */\n")
    f.write("\n#endif\n")
with open(os.path.join(src, "tables.c"), "w", newline="\n") as f:
    f.write("/* Automatisch erzeugt von tools/gen_tables.py - nicht von Hand aendern */\n")
    f.write('#include "tables.h"\n\n')
    f.write("/* Viertelwelle sin(0..63), in 16.16 ohne Vorzeichen */\n")
    f.write("static const uint16_t SIN_Q[64] = {\n")
    for i in range(0, 64, 8):
        f.write("  " + ", ".join(str(v) for v in q[i:i+8]) + ",\n")
    f.write("};\n\n")
    f.write("void tables_init(void) {}\n\n")
    f.write("int32_t sin_tab(uint8_t i) {\n")
    f.write("  uint8_t k = (uint8_t)(i & 127);\n")
    f.write("  int32_t v = k < 64 ? (int32_t)SIN_Q[k] : k == 64 ? 65536L : (int32_t)SIN_Q[128 - k];\n")
    f.write("  return i < 128 ? v : -v;\n")
    f.write("}\n\n")
    f.write("const uint8_t ATAN_TAB[65] = {\n")
    for i in range(0, 65, 16):
        f.write("  " + ", ".join(str(v) for v in atan[i:i+16]) + ",\n")
    f.write("};\n")
print("geschrieben: src/tables.h, src/tables.c")
