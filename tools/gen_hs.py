#!/usr/bin/env python3
"""Erzeugt build/pb65hs.prg: Highscore-Datei mit den Vorgabewerten (v7.4).
Aufbau wie in src/hiscore.c: Ladeadresse $0F00, "HS1", 5 x (3 Buchstaben, Punkte 32 Bit)."""
import struct, sys
DEFAULT = [("C64", 500000), ("VIC", 400000), ("SID", 300000), ("PLA", 200000), ("CIA", 100000)]
data = struct.pack("<H", 0x0F00) + b"HS1" + b"".join(n.encode() + struct.pack("<I", s) for n, s in DEFAULT)
open(sys.argv[1], "wb").write(data)
