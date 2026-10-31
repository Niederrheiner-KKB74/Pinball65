#!/usr/bin/env python3
"""Erzeugt src/music_data.h und src/music_data.c aus den Liedern unten.

Alle Stuecke sind eigene Kompositionen fuer dieses Spiel (Akkordfolge
Am - G - F - E und Varianten). Gespielt werden sie auf dem zweiten SID-Paar
($D420 und $D460), die Klangeffekte bleiben auf $D400 und $D440.

Schreibweise in einem Pattern (Leerzeichen trennt, Laenge in Schritten):
  A4:2   Note A4 fuer 2 Schritte   (C0..B6, # fuer Kreuz: F#3)
  -:4    Pause (Gate aus)
  =:2    Halten (die Note klingt weiter)
  @lead  Instrument wechseln
  K S H  Schlagzeug (Bassdrum, Snare, Hi-Hat), z. B. K:2
Ein Schritt dauert TEMPO Bilder (50 pro Sekunde).
"""
import os, re

src = os.path.join(os.path.dirname(__file__), "..", "src")

# ---------------------------------------------------------------- Instrumente
# wave, ad, sr, pw (12 Bit), pw_sweep (pro Bild), fx, fx_arg
#   fx 0 nichts, 1 Arpeggio (arg = Tabelle), 2 Rutschen nach unten (arg * 16 pro Bild),
#   3 Vibrato (arg = Verzoegerung in Bildern)
INSTR = {
    "bass":  (0x40, 0x08, 0x6A, 0x300, 0x18, 0, 0),
    "lead":  (0x40, 0x0A, 0x86, 0x600, 0x10, 3, 10),
    "arpm":  (0x40, 0x04, 0x52, 0x800, 0x08, 1, 0),   # Moll: 0 3 7
    "arpM":  (0x40, 0x04, 0x52, 0x800, 0x08, 1, 1),   # Dur:  0 4 7
    "kick":  (0x10, 0x08, 0x00, 0x000, 0x00, 2, 40),
    "snare": (0x80, 0x08, 0x00, 0x000, 0x00, 2, 4),
    "hat":   (0x80, 0x02, 0x00, 0x000, 0x00, 0, 0),
    "fan":   (0x20, 0x09, 0xA8, 0x000, 0x00, 3, 6),
    "chime": (0x10, 0x0A, 0x00, 0x000, 0x00, 0, 0),
    # v8.5: fuer die Spielmusik
    "saw":   (0x20, 0x08, 0x89, 0x000, 0x00, 3, 8),     # Saegezahn-Melodie mit Vibrato
    "pluck": (0x40, 0x03, 0x40, 0x400, 0x20, 0, 0),     # kurzer Puls fuer Riffs
    "sbass": (0x20, 0x06, 0x48, 0x000, 0x00, 0, 0),     # Saegezahn-Bass
}
INAMES = list(INSTR)
DRUMS = {"K": ("kick", "C4"), "S": ("snare", "A5"), "H": ("hat", "C6")}

# ---------------------------------------------------------------- Patterns
P = {}
# Spielmusik: Bass in Achteln, weiche Akkord-Arpeggios, Schlagzeug
P["bass1"] = "@bass " + " ".join(f"{n}:2" for n in
    "A2 A2 A3 A2 A2 A3 G2 A2  G2 G2 G3 G2 G2 G3 D3 G2  F2 F2 F3 F2 F2 F3 C3 F2  E2 E2 E3 E2 E2 E3 B2 G#2".split())
P["arp1"] = "@arpm A3:16 @arpM G3:16 F3:16 E3:16"
P["drum1"] = "K:2 H:2 S:2 H:2 K:2 K:2 S:2 H:2 " * 3 + "K:2 H:2 S:2 H:2 K:2 S:1 S:1 S:2 S:2"
# Titel: Melodie statt Arpeggios
P["mel1"] = ("@lead E5:2 -:1 A4:1 C5:2 E5:1 D5:1 =:4 -:4 "
             "D5:2 -:1 B4:1 G4:2 B4:1 D5:1 =:4 -:4 "
             "C5:2 -:1 A4:1 F4:2 A4:1 C5:1 =:4 -:4 "
             "B4:2 -:1 G#4:1 E4:4 =:4 -:4")
P["mel2"] = ("@lead A5:3 G5:1 E5:2 C5:2 =:4 -:4 "
             "B4:2 D5:2 G5:3 F5:1 =:4 -:4 "
             "E5:2 C5:2 A4:2 F4:2 =:4 -:4 "
             "G#4:2 B4:2 E5:4 =:4 D5:2 C5:2")
P["silence"] = "-:32"
# ================================================================ v8.5: Spielmusik
def bars(*b): return " ".join(b)
DR  = "K:2 H:2 S:2 H:2 K:2 K:2 S:2 H:2"
DRF = "K:2 H:2 S:2 H:2 K:2 S:1 S:1 S:2 S:2"
DR2 = "K:2 H:1 H:1 S:2 H:2 K:1 K:1 H:2 S:2 H:2"     # zweite Variante, treibender
DRH = "H:2 H:2 H:2 H:2 H:2 H:2 H:2 H:2"            # nur Hi-Hat (Uebergang)

# --- Stueck 1 "FAST LOADER": e-Moll, Em - C - D - H, schnell
def oct8(lo, hi): return " ".join(f"{n}:2" for n in [lo, hi] * 4)
P["g1_bass"] = "@bass " + bars(oct8("E2", "E3"), oct8("C2", "C3"), oct8("D2", "D3"), oct8("B1", "B2"))
P["g1_arp"]  = "@arpm E3:16 @arpM C3:16 D3:16 B2:16"
P["g1_mela"] = ("@saw B4:2 E5:2 G5:2 F#5:2 E5:4 B4:4 "
                "C5:2 E5:2 G5:4 E5:2 C5:2 E5:4 "
                "D5:2 F#5:2 A5:4 G5:2 F#5:2 D5:4 "
                "D#5:4 F#5:4 B5:6 -:2")
P["g1_melb"] = ("@saw G5:3 F#5:1 E5:2 B4:2 G4:4 B4:4 "
                "E5:3 D5:1 C5:2 G4:2 E4:4 G4:4 "
                "F#5:2 A5:2 D6:4 C6:2 A5:2 F#5:4 "
                "D#5:2 F#5:2 B5:4 A5:2 F#5:2 D#5:4")
P["g1_dr"]   = bars(DR, DR, DR, DRF)
P["g1_dr2"]  = bars(DR2, DR2, DR2, DRF)

# --- Stueck 2 "SPRITE DANCE": D dorisch, locker und funky
P["g2_bass"] = "@sbass " + bars(
    "D2:3 D3:1 -:2 D2:2 F2:2 A2:2 C3:2 A2:2",
    "G1:3 G2:1 -:2 G1:2 B1:2 D2:2 F2:2 D2:2") 
P["g2_bass2"] = "@sbass " + bars(
    "F2:3 F3:1 -:2 F2:2 A2:2 C3:2 F3:2 C3:2",
    "C2:3 C3:1 -:2 C2:2 E2:2 G2:2 C3:2 G2:2",
    "G1:3 G2:1 -:2 G1:2 B1:2 D2:2 G2:2 D2:2",
    "A1:3 A2:1 -:2 A1:2 C#2:2 E2:2 A2:2 G2:2")
P["g2_riff"] = "@pluck " + bars(
    "-:2 F4:1 A4:1 -:2 C5:2 -:2 A4:1 C5:1 D5:2 -:2",
    "-:2 F4:1 B4:1 -:2 D5:2 -:2 B4:1 D5:1 F5:2 -:2")
P["g2_mel"] = "@lead " + bars(
    "A4:2 C5:2 D5:3 F5:1 E5:2 D5:2 C5:2 A4:2",
    "B4:2 D5:2 G5:4 F5:2 D5:2 B4:4",
    "A4:2 C5:2 D5:3 F5:1 E5:2 D5:2 C5:2 E5:2",
    "D5:8 =:4 -:4")
P["g2_mel2"] = "@lead " + bars(
    "C5:2 F5:2 A5:4 G5:2 F5:2 C5:4",
    "E5:2 G5:2 C6:4 B5:2 G5:2 E5:4",
    "D5:2 G5:2 B5:4 A5:2 G5:2 D5:4",
    "C#5:2 E5:2 A5:6 =:2 -:4")
P["g2_dr"]  = bars("K:3 H:1 S:2 H:2 -:1 K:1 K:2 S:2 H:2", "K:3 H:1 S:2 H:2 -:1 K:1 K:2 S:2 S:1 S:1")

# --- Stueck 3 "RASTER RUN": a-Moll, Am - F - C - G, treibend mit Wechselbass
def pump(a, b): return " ".join(f"{n}:2" for n in [a, a, b, a, a, b, a, b])
P["g3_bass"] = "@bass " + bars(pump("A2", "E3"), pump("F2", "C3"), pump("C2", "G2"), pump("G2", "D3"))
P["g3_arp"]  = "@arpm A3:16 @arpM F3:16 C3:16 G3:16"
P["g3_mel"]  = ("@saw E5:4 A5:4 G5:2 E5:2 C5:4 "
                "F5:4 C5:4 A4:4 C5:4 "
                "E5:2 G5:2 C6:4 B5:2 G5:2 E5:4 "
                "D5:4 G5:4 B5:6 -:2")
P["g3_mel2"] = ("@saw A5:3 G5:1 E5:2 A5:2 C6:4 A5:4 "
                "F5:3 E5:1 C5:2 F5:2 A5:4 F5:4 "
                "G5:3 F5:1 E5:2 C5:2 G4:4 C5:4 "
                "D5:2 E5:2 G5:2 B5:2 D6:6 -:2")
P["g3_dr"]   = bars(DR2, DR2, DR2, DRF)
P["g3_drh"]  = bars(DRH, DRH, DRH, DRF)

# Jingles
P["fan_hi"] = "@fan C5:1 E5:1 G5:1 C6:1 E5:1 G5:1 C6:6 =:4 -:2"
P["fan_lo"] = "@bass C3:4 G2:4 C3:6 -:2"
P["fan_dr"] = "S:1 S:1 S:1 S:1 K:4 K:6 -:2"
P["mb_hi"] = "@chime A4:1 C5:1 E5:1 A5:1 C6:1 E6:1 A5:1 C6:1 E6:1 A6:1 =:6 -:2"
P["mb_lo"] = "@bass A2:2 A3:2 A2:2 A3:2 A2:6 -:2"
P["mb_dr"] = "S:1 S:1 S:1 S:1 S:1 S:1 S:1 S:1 K:6 -:2"
P["go_hi"] = "@lead E5:4 D5:4 C5:4 B4:4 A4:12 -:4"
P["go_lo"] = "@bass A2:8 G2:8 F2:4 E2:8 -:4"

# ---------------------------------------------------------------- Lieder
# name: (Tempo in Bildern pro Schritt, Wiederholen?, [Folge Stimme 1, 2, 3])
SONGS = [
    ("MUS_GAME",    6, True,  [["bass1"], ["arp1"], ["drum1"]]),
    # v8.5: Spielmusik, drei Stuecke im Wechsel
    ("MUS_PLAY1",   5, True,  [["g1_bass"] * 6,
                               ["g1_arp", "g1_arp", "g1_mela", "g1_melb", "g1_mela", "g1_melb"],
                               ["g1_dr", "g1_dr", "g1_dr2", "g1_dr2", "g1_arp", "g1_dr2"]]),
    ("MUS_PLAY2",   6, True,  [["g2_bass", "g2_bass", "g2_bass", "g2_bass", "g2_bass2", "g2_bass2"],
                               ["g2_riff", "g2_riff", "g2_mel", "g2_mel2", "g2_mel2"],
                               ["g2_dr"] * 8]),
    ("MUS_PLAY3",   5, True,  [["g3_bass"] * 6,
                               ["g3_arp", "g3_mel", "g3_mel2", "g3_arp", "g3_mel", "g3_mel2"],
                               ["g3_drh", "g3_dr", "g3_dr", "g3_dr", "g3_dr", "g3_dr"]]),
    ("MUS_TITLE",   6, True,  [["bass1", "bass1"], ["mel1", "mel2"], ["drum1", "drum1"]]),
    ("MUS_FANFARE", 5, False, [["fan_lo"], ["fan_hi"], ["fan_dr"]]),
    ("MUS_MULTI",   4, False, [["mb_lo"], ["mb_hi"], ["mb_dr"]]),
    ("MUS_OVER",    7, False, [["go_lo"], ["go_hi"], ["silence"]]),
]

# ---------------------------------------------------------------- Uebersetzen
NOTE = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
def note_num(t):
    m = re.fullmatch(r"([A-G])(#?)([0-6])", t)
    assert m, t
    return int(m.group(3)) * 12 + NOTE[m.group(1)] + (1 if m.group(2) else 0)

def compile_pat(text):
    out, cur_len, cur_ins = [], None, None
    def length(n):
        nonlocal cur_len
        assert 1 <= n <= 32, n
        if n != cur_len: out.append(0x60 + n - 1); cur_len = n
    def instr(name):
        nonlocal cur_ins
        i = INAMES.index(name)
        if i != cur_ins: out.append(0x80 + i); cur_ins = i
    for tok in text.split():
        if tok.startswith("@"): instr(tok[1:]); continue
        name, n = tok.split(":"); n = int(n)
        if name in DRUMS:
            ins, nt = DRUMS[name]; instr(ins); length(n); out.append(note_num(nt))
        elif name == "-": length(n); out.append(0xFD)
        elif name == "=": length(n); out.append(0xFE)
        else: length(n); out.append(note_num(name))
    out.append(0xFF)
    return out

def steps(text):
    return sum(int(t.split(":")[1]) for t in text.split() if not t.startswith("@"))

pat_names = list(P)
data, ofs = [], []
for n in pat_names:
    ofs.append(len(data)); data += compile_pat(P[n])
for name, tempo, loop, voices in SONGS:
    lens = [sum(steps(P[p]) for p in v) for v in voices]
    assert len(set(lens)) == 1 or not loop, (name, lens)

# Frequenztabelle: Oktave 6 (C6..B6) fuer PAL (985248 Hz), tiefere Oktaven durch Schieben
base = [round(440 * 2 ** ((72 + i - 57) / 12) * 16777216 / 985248) for i in range(12)]
assert max(base) < 65536

with open(os.path.join(src, "music_data.h"), "w", newline="\n") as f:
    f.write("/* Automatisch erzeugt von tools/gen_music.py - nicht von Hand aendern */\n")
    f.write("#ifndef MUSIC_DATA_H\n#define MUSIC_DATA_H\n#include <stdint.h>\n\n")
    f.write("enum { " + ", ".join(s[0] for s in SONGS) + ", MUS_COUNT };\n")
    f.write("typedef struct { uint8_t tempo, loop; const uint8_t *order[3]; } song_t;\n")
    f.write("typedef struct { uint8_t wave, ad, sr; uint16_t pw; uint8_t sweep, fx, arg; } instr_t;\n")
    f.write("extern const song_t SONGS[MUS_COUNT];\nextern const instr_t INSTR[];\n")
    f.write("extern const uint8_t MUS_PAT[];\nextern const uint16_t MUS_OFS[];\nextern const uint16_t NOTE6[12];\n")
    f.write("\n#endif\n")
with open(os.path.join(src, "music_data.c"), "w", newline="\n") as f:
    f.write("/* Automatisch erzeugt von tools/gen_music.py - nicht von Hand aendern */\n")
    f.write('#include "music_data.h"\n\n')
    f.write("const uint16_t NOTE6[12] = { " + ", ".join(map(str, base)) + " };\n")
    f.write("const instr_t INSTR[] = {\n")
    for n, v in INSTR.items():
        f.write("  { 0x%02X, 0x%02X, 0x%02X, 0x%03X, %d, %d, %d },   /* %s */\n" % (v + (n,)))
    f.write("};\n")
    f.write("const uint8_t MUS_PAT[] = {\n")
    for i in range(0, len(data), 16):
        f.write("  " + ", ".join("0x%02X" % b for b in data[i:i + 16]) + ",\n")
    f.write("};\n")
    f.write("const uint16_t MUS_OFS[] = { " + ", ".join(map(str, ofs)) + " };\n")
    for name, tempo, loop, voices in SONGS:
        for k, v in enumerate(voices):
            f.write("static const uint8_t %s_%d[] = { %s, 0xFF };\n" % (name, k, ", ".join(str(pat_names.index(p)) for p in v)))
    f.write("const song_t SONGS[MUS_COUNT] = {\n")
    for name, tempo, loop, voices in SONGS:
        f.write("  { %d, %d, { %s } },\n" % (tempo, 1 if loop else 0, ", ".join("%s_%d" % (name, k) for k in range(3))))
    f.write("};\n")
print("Patterns: %d Byte, %d Lieder" % (len(data), len(SONGS)))
