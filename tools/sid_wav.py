#!/usr/bin/env python3
"""Grobe SID-Nachbildung: liest je Bild 25 Registerbytes (aus dem PC-Test,
MUSIC=1) und schreibt eine WAV-Datei zum Probehoeren. Kein exakter SID,
nur Wellenformen, Huellkurve und Lautstaerke."""
import sys, wave, struct, math, random
CLK, RATE, FPS = 985248, 44100, 50
ATT = [2, 8, 16, 24, 38, 56, 68, 80, 100, 250, 500, 800, 1000, 3000, 5000, 8000]   # ms
DEC = [6, 24, 48, 72, 114, 168, 204, 240, 300, 750, 1500, 2400, 3000, 9000, 15000, 24000]
def render(src, dst):
    data = open(src, "rb").read()
    frames = [data[i:i + 25] for i in range(0, len(data) - 24, 25)]
    spf = RATE // FPS
    st = [dict(acc=0.0, env=0.0, phase="rel", gate=0, lfsr=0x7FFFF8, noise=0.0) for _ in range(3)]
    out = []
    for fr in frames:
        vol = (fr[0x18] & 15) / 15.0
        for n in range(spf):
            mix = 0.0
            for v in range(3):
                r = fr[v * 7:v * 7 + 7]; s = st[v]
                freq = (r[0] | r[1] << 8) * CLK / 16777216
                pw = ((r[2] | r[3] << 8) & 0xFFF) / 4096 or 0.5
                ctrl, ad, sr = r[4], r[5], r[6]
                gate = ctrl & 1
                if gate and not s["gate"]: s["phase"] = "att"
                if not gate and s["gate"]: s["phase"] = "rel"
                s["gate"] = gate
                sus = (sr >> 4) / 15.0
                if s["phase"] == "att":
                    s["env"] += 1000 / (ATT[ad >> 4] * RATE)
                    if s["env"] >= 1: s["env"] = 1; s["phase"] = "dec"
                elif s["phase"] == "dec":
                    s["env"] = max(sus, s["env"] - 1000 / (DEC[ad & 15] * RATE))
                else:
                    s["env"] = max(0.0, s["env"] - 1000 / (DEC[sr & 15] * RATE))
                old = s["acc"]; s["acc"] = (s["acc"] + freq / RATE) % 1.0
                ph = s["acc"]; w = 0.0
                if ctrl & 0x80:
                    if s["acc"] < old: s["noise"] = random.uniform(-1, 1)
                    w = s["noise"]
                elif ctrl & 0x40: w = 1.0 if ph < pw else -1.0
                elif ctrl & 0x20: w = 2 * ph - 1
                elif ctrl & 0x10: w = 4 * ph - 1 if ph < 0.5 else 3 - 4 * ph
                mix += w * s["env"]
            out.append(max(-1, min(1, mix * vol / 3)))
    with wave.open(dst, "wb") as wf:
        wf.setnchannels(1); wf.setsampwidth(2); wf.setframerate(RATE)
        wf.writeframes(b"".join(struct.pack("<h", int(x * 30000)) for x in out))
if __name__ == "__main__":
    render(sys.argv[1], sys.argv[2])
