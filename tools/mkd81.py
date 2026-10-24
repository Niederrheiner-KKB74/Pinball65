#!/usr/bin/env python3
"""Erzeugt ein D81-Diskettenimage (1581-Format) mit einer oder mehreren PRG-Dateien.

Ohne externe Abhaengigkeiten, laeuft mit Python 3.8+ unter Windows, Linux und macOS.

Aufruf:
    python tools/mkd81.py <ausgabe.d81> <DISKNAME> <datei.prg> [<datei2.prg> ...]

Der Dateiname auf der Diskette ist der Dateiname ohne Endung in Grossbuchstaben,
z. B. build/pinball65.prg -> PINBALL65.
"""
import os
import sys

TRACKS, SECTORS, SECSIZE = 80, 40, 256
DIR_TRACK = 40


def offset(track, sector):
    return ((track - 1) * SECTORS + sector) * SECSIZE


def petscii_name(name, length=16):
    raw = name.upper().encode("ascii", "replace")[:length]
    return raw + b"\xa0" * (length - len(raw))


def check_prg(path, data):
    """Das geladene Programm muss unter $A000 enden.

    Im C65-Modus liegt bei $A000-$BFFF beim Laden das BASIC-ROM. Meilenstein 2
    (Version 2.0) reichte bis $B14E und stuerzte auf Xemu und MEGA65 ab.
    Variablen (.bss) duerfen darueber liegen, sie werden erst beim Start angelegt.
    """
    if len(data) < 2:
        return
    load = data[0] | (data[1] << 8)
    end = load + len(data) - 2
    print(f"  {os.path.basename(path)}: ${load:04X}-${end - 1:04X}")
    if os.path.basename(path).lower().startswith("loadtest"):
        return                      # Testprogramm, reicht absichtlich ueber $A000
    if load == 0x2001 and end > 0xA000:
        raise SystemExit(f"FEHLER: Programm endet bei ${end:04X}, erlaubt ist hoechstens $A000.")


def build(out_path, disk_name, prg_paths, disk_id=b"65"):
    img = bytearray(TRACKS * SECTORS * SECSIZE)
    used = {t: set() for t in range(1, TRACKS + 1)}
    used[DIR_TRACK] = set(range(SECTORS))  # Verzeichnisspur komplett reserviert

    # Freie Sektoren ausserhalb der Verzeichnisspur, von innen nach aussen
    def free_sectors():
        order = []
        for d in range(1, 40):
            for t in (DIR_TRACK - d, DIR_TRACK + d):
                if 1 <= t <= TRACKS:
                    order.append(t)
        for t in order:
            for s in range(SECTORS):
                if s not in used[t]:
                    yield t, s

    alloc = free_sectors()
    entries = []
    for path in prg_paths:
        data = open(path, "rb").read()
        check_prg(path, data)
        chunks = [data[i:i + 254] for i in range(0, len(data), 254)] or [b""]
        secs = [next(alloc) for _ in chunks]
        for (t, s) in secs:
            used[t].add(s)
        for i, chunk in enumerate(chunks):
            t, s = secs[i]
            o = offset(t, s)
            if i + 1 < len(chunks):
                img[o], img[o + 1] = secs[i + 1]
            else:
                img[o], img[o + 1] = 0, len(chunk) + 1
            img[o + 2:o + 2 + len(chunk)] = chunk
        name = os.path.splitext(os.path.basename(path))[0]
        entries.append((name, secs[0], len(chunks)))

    if len(entries) > 8:
        raise SystemExit("Dieses einfache Werkzeug unterstuetzt bis zu 8 Dateien.")

    # Header 40/0
    o = offset(DIR_TRACK, 0)
    img[o:o + 4] = bytes([DIR_TRACK, 3, 0x44, 0x00])
    img[o + 4:o + 20] = petscii_name(disk_name)
    img[o + 20:o + 22] = b"\xa0\xa0"
    img[o + 22:o + 24] = disk_id
    img[o + 24] = 0xA0
    img[o + 25:o + 27] = b"3D"
    img[o + 27:o + 29] = b"\xa0\xa0"

    # BAM 40/1 (Spuren 1-40) und 40/2 (Spuren 41-80)
    for bam_sec, first, link in ((1, 1, (DIR_TRACK, 2)), (2, 41, (0, 0xFF))):
        o = offset(DIR_TRACK, bam_sec)
        img[o:o + 8] = bytes([link[0], link[1], 0x44, 0xBB, disk_id[0], disk_id[1], 0xC0, 0x00])
        for i in range(40):
            t = first + i
            bits = bytearray(5)
            free = 0
            for s in range(SECTORS):
                if s not in used[t]:
                    bits[s // 8] |= 1 << (s % 8)
                    free += 1
            e = o + 16 + i * 6
            img[e] = free
            img[e + 1:e + 6] = bits

    # Verzeichnis 40/3
    o = offset(DIR_TRACK, 3)
    img[o], img[o + 1] = 0, 0xFF
    for i, (name, (t, s), size) in enumerate(entries):
        e = o + i * 32
        img[e + 2] = 0x82  # PRG, geschlossen
        img[e + 3], img[e + 4] = t, s
        img[e + 5:e + 21] = petscii_name(name)
        img[e + 30] = size & 0xFF
        img[e + 31] = size >> 8

    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    with open(out_path, "wb") as f:
        f.write(img)
    for name, _, size in entries:
        print(f"  {name.upper():16s} {size:4d} Bloecke")
    print(f"D81 geschrieben: {out_path}")


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(__doc__)
        sys.exit(1)
    build(sys.argv[1], sys.argv[2], sys.argv[3:])
