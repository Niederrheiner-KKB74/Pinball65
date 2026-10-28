#!/usr/bin/env python3
"""Mock-up der endgueltigen Tischgrafik (Meilenstein 6): echter Flippertisch mit
C64-Motiven. Nimmt das Bild aus dem PC-Test (mockup.ppm, 320 x 640) und ersetzt
den bisherigen Platzhalter-Hintergrund durch eine gemalte Grafik; Waende, Rampe,
Targets, Lampen und Flipper bleiben an ihrer Stelle.

Aufruf: art_mockup.py mockup.ppm ausgabe.png [quantisiert.png]
Keine Logos: Motive sind Einschaltbild, Platine mit Chips, Tastatur, Laufwerk."""
import sys, math
from PIL import Image, ImageDraw, ImageFont

W, H = 320, 640
BG_KEYS = {(16, 12, 56), (26, 21, 74), (34, 28, 92), (50, 120, 140)}
CHROME = {(100, 96, 150): (104, 104, 116), (160, 156, 205): (172, 172, 184), (236, 235, 255): (244, 244, 250)}
C64 = {"blue": (44, 40, 150), "lblue": (112, 110, 232), "black": (0, 0, 0), "white": (255, 255, 255)}
# Bumper-Kappen als cremefarbener Kunststoff
CHROME[(64, 49, 141)] = (238, 232, 214)

try:
    FONT = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", 8)
    FONT_S = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", 7)
except OSError:
    FONT = FONT_S = ImageFont.load_default()

def art_layer():
    a = Image.new("RGB", (W, H), (18, 14, 40))
    d = ImageDraw.Draw(a)
    # Grundflaeche: Verlauf von Nachtblau (oben) zu Violett (unten)
    for y in range(H):
        t = y / H
        d.line([(0, y), (W, y)], fill=(int(18 + 20 * t), int(14 + 6 * t), int(52 + 10 * t)))
    # --- oben: C64-Einschaltbild als grosser Bildschirm hinter Lanes und Bumpern
    d.rounded_rectangle([34, 14, 286, 236], 26, fill=C64["lblue"])
    d.rounded_rectangle([44, 26, 276, 226], 16, fill=C64["blue"])
    for i, t in enumerate(["**** PINBALL 65 BASIC V2 ****", "64K RAM SYSTEM  38911 BASIC BYTES FREE"]):
        tw = d.textlength(t, font=FONT_S)
        d.text((160 - tw / 2, 108 + 9 * i), t, font=FONT_S, fill=C64["lblue"])
    d.text((60, 206), "READY.", font=FONT, fill=C64["lblue"])
    d.rectangle([92, 206, 98, 214], fill=C64["lblue"])
    # Rasterlinien wie auf einem alten Bildschirm
    for y in range(28, 226, 3):
        d.line([(46, y), (274, y)], fill=(58, 44, 130))
    # --- Mitte: Regenbogen-Streifen schraeg ueber den Tisch
    cols = [(210, 50, 50), (236, 128, 36), (240, 206, 48), (70, 170, 80), (60, 100, 210)]
    for k, c in enumerate(cols):
        off = k * 9
        poly = [(0, 400 + off), (0, 409 + off), (320, 249 + off), (320, 240 + off)]
        d.polygon(poly, fill=c)
    d.line([(0, 399), (320, 239)], fill=(10, 8, 20), width=1)
    d.line([(0, 446), (320, 286)], fill=(10, 8, 20), width=1)
    # --- unten: Platine mit Leiterbahnen
    pcb = Image.new("RGB", (W, H), (0, 0, 0))
    pd = ImageDraw.Draw(pcb)
    for y in range(300, 600):
        pd.line([(0, y), (W, y)], fill=(18, 58, 38))
    for x in range(14, 310, 22):
        pd.line([(x, 455), (x, 600)], fill=(150, 120, 52), width=1)
    for y in range(470, 600, 26):
        pd.line([(14, y), (306, y)], fill=(150, 120, 52), width=1)
    for x in range(14, 310, 22):
        for y in range(470, 600, 26):
            pd.ellipse([x - 2, y - 2, x + 2, y + 2], fill=(200, 170, 80))
    mask = Image.new("L", (W, H), 0)
    md = ImageDraw.Draw(mask)
    md.polygon([(0, 450), (320, 290), (320, 640), (0, 640)], fill=255)
    a.paste(pcb, (0, 0), mask)
    # Chips mit Beschriftung neben ihren Target-Gruppen
    def chip(x, y, w, h, label):
        d.rectangle([x, y, x + w, y + h], fill=(24, 24, 28), outline=(70, 70, 80))
        for px in range(x + 3, x + w - 1, 5):
            d.rectangle([px, y - 3, px + 2, y], fill=(190, 190, 200))
            d.rectangle([px, y + h, px + 2, y + h + 3], fill=(190, 190, 200))
        d.ellipse([x + 2, y + h // 2 - 2, x + 5, y + h // 2 + 1], fill=(60, 60, 70))
        tw = d.textlength(label, font=FONT_S)
        d.text((x + w / 2 - tw / 2, y + h / 2 - 4), label, font=FONT_S, fill=(210, 210, 220))
    chip(36, 446, 44, 14, "VIC-II")
    chip(234, 446, 44, 14, "SID")
    chip(222, 244, 40, 14, "6510")
    chip(52, 182, 30, 12, "PLA")
    # --- 1541-Laufwerk um das Kick-Out-Hole
    d.rectangle([150, 242, 208, 300], fill=(206, 196, 168), outline=(120, 110, 90))
    d.rectangle([154, 290, 204, 294], fill=(60, 56, 50))
    d.rectangle([196, 280, 201, 284], fill=(230, 40, 40))
    d.text((153, 244), "1541", font=FONT_S, fill=(80, 70, 60))
    # --- Tastatur: RUN/STOP und RESTORE in den Out-Lanes, Tasten an den Inlanes
    def key(x, y, w, h, label):
        d.rounded_rectangle([x, y, x + w, y + h], 3, fill=(88, 66, 48), outline=(40, 30, 22))
        d.rounded_rectangle([x + 2, y + 2, x + w - 2, y + h - 5], 2, fill=(140, 112, 84))
        tw = d.textlength(label, font=FONT_S)
        d.text((x + w / 2 - tw / 2, y + 3), label, font=FONT_S, fill=(240, 232, 210))
    key(12, 520, 24, 16, "RUN")
    key(12, 538, 24, 16, "STOP")
    key(265, 520, 24, 16, "RES")
    key(265, 538, 24, 16, "TORE")
    # --- Holzleisten an den Seiten
    for x0, x1 in ((0, 9), (311, 320)):
        for x in range(x0, x1):
            d.line([(x, 0), (x, H)], fill=(110 + (x % 3) * 8, 70 + (x % 3) * 5, 36))
    return a

def main(src, dst, dstq=None):
    game = Image.open(src).convert("RGB")
    art = art_layer()
    out = game.copy()
    gp, ap, op = game.load(), art.load(), out.load()
    for y in range(H):
        for x in range(W):
            c = gp[x, y]
            if c in BG_KEYS: op[x, y] = ap[x, y]
            elif c in CHROME: op[x, y] = CHROME[c]
    big = out.resize((W * 2, H * 2), Image.NEAREST)
    big.save(dst)
    if dstq:
        q = out.quantize(colors=160, dither=Image.Dither.NONE).convert("RGB")
        n = len(set(q.getdata()))
        q.resize((W * 2, H * 2), Image.NEAREST).save(dstq)
        print("Farben nach Quantisierung:", n)
    print("Farben im Entwurf:", len(set(out.getdata())))

if __name__ == "__main__":
    main(*sys.argv[1:])
