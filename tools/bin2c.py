#!/usr/bin/env python3
"""Binaerdatei als C-Array (v7.7: Konstanten des Spiels fuer den Lader).
Aufruf: bin2c.py eingabe.bin ausgabe.h NAME"""
import sys
src, dst, name = sys.argv[1:4]
data = open(src, "rb").read()
with open(dst, "w") as f:
    f.write("/* Automatisch erzeugt von tools/bin2c.py aus %s - nicht von Hand aendern */\n" % src)
    f.write("static const unsigned char %s[%d] = {\n" % (name, len(data)))
    for i in range(0, len(data), 16):
        f.write("  " + ", ".join("0x%02X" % b for b in data[i:i + 16]) + ",\n")
    f.write("};\n")
print("%s: %d Byte" % (dst, len(data)))
