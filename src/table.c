/*
 * table.c – Kollisionsmaske und Platzhaltergrafik, beim Start berechnet
 *
 * Wandstaerke wie im Prototyp (Strichbreite 4): Scheibe mit Radius 2.
 * Die Grafik wird in Baendern von 8 Zeilen berechnet und spaltenweise ins
 * Attic-RAM geschrieben (je Spalte ein Streifen von 640 x 8 Byte).
 */
#include <mega65.h>
#include "table.h"
#include "hw.h"
#include "fixmath.h"
#include "ramp_path.h"
#include "dmd.h"

int16_t win_y0 = -1000;
const uint8_t MASK_BIT[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
uint8_t cls_r0;

const int16_t BUMPERS[N_BUMPERS][3] = { {124, 140, 13}, {196, 140, 13}, {160, 188, 13} };

/* Fenster ab Tischzeile y0 aus dem Farb-RAM laden */
void mask_load(int16_t y0) {
  if (y0 > TH - WIN_ROWS) y0 = TH - WIN_ROWS;
  if (y0 < 0) y0 = 0;
  win_y0 = y0;
  dma_copy(MASK_FAR + (uint32_t)(uint16_t)y0 * 40, CPUADDR(MASKWIN), WIN_ROWS * 40);
  uint8_t r0 = (uint8_t)(y0 >> 3);
  if (r0 > TH / 8 - CLS_ROWS) r0 = TH / 8 - CLS_ROWS;
  cls_r0 = r0;
  dma_copy(CLS_FAR + (uint16_t)r0 * 40, CPUADDR(CLSWIN), CLS_ROWS * 40);
}

void table_build_palette(void) {
  set_color(0, 0, 0, 0);
  set_color(C_BG, 16, 12, 56);
  set_color(C_GRID, 26, 21, 74);
  set_color(C_CHROME_DK, 104, 104, 116);   /* v7.1: Chrom silbern statt violett */
  set_color(C_CHROME_MID, 172, 172, 184);
  set_color(C_CHROME_HI, 244, 244, 250);
  set_color(C_SLING_BODY, 40, 33, 100);
  set_color(C_RUBBER, 216, 48, 64);
  set_color(C_LANE, 7, 5, 26);
  set_color(C_FLIP, 243, 234, 210);
  set_color(C_FLIP_EDGE, 224, 48, 58);
  set_color(C_BALL_DK, 85, 87, 122);
  set_color(C_BALL_MID, 201, 203, 224);
  set_color(C_BALL_HI, 255, 255, 255);
  set_color(C_TARGET_VIC, 80, 200, 255);
  set_color(C_TARGET_6510, 120, 240, 140);
  set_color(C_TARGET_SID, 255, 120, 220);
  set_color(C_BUMPER_RING, 200, 32, 46);
  set_color(C_C64_DARK, 238, 232, 214);     /* v7.1: Bumper-Kappen cremefarben */
  set_color(C_C64_LIGHT, 120, 106, 189);    /* Einschaltbild: Schrift */
  set_color(C_HOLE, 4, 3, 12);
  set_color(C_HOLE_RING, 255, 210, 60);
  set_color(C_LAMP, 120, 96, 30);           /* Lampe aus (gedimmtes Gelb) */
  set_color(C_LABEL, 170, 160, 230);
  set_color(C_TRACE, 34, 28, 92);
  set_color(C_TRACE2, 50, 120, 140);
  set_color(C_KICKBACK, 220, 50, 64);
  set_color(C_DMD_BG, 0, 0, 0);
  set_color(C_DMD_OFF, 40, 18, 4);
  set_color(C_DMD_ON, 255, 160, 40);
  set_color(C_DMD_EDGE, 170, 90, 16);
  set_color(C_LOGO_RED, 232, 69, 60);
  set_color(C_LOGO_BLUE, 61, 107, 234);
  set_color(C_TARGET_PLA, 255, 150, 40);
  set_color(C_KICK_RING, 255, 210, 60);
  set_color(C_RAMP_RAIL, 220, 230, 255);    /* Rampe: Chromschienen ... */
  set_color(C_RAMP_RAIL_DK, 120, 130, 190);
  set_color(C_RAMP_FLOOR, 70, 150, 210);    /* ... und durchsichtiger Boden (Raster) */
  for (uint8_t k = 0; k < 4; k++) set_color((uint8_t)(C_SPIN0 + k), 90, 90, 110);
}


void art_palette_load(void) {
  dma_copy(ATTIC_PAL, CPUADDR(COLBUF), N_ART_COLORS * 3);
  const uint8_t *c = COLBUF;
  for (uint8_t i = 0; i < N_ART_COLORS; i++, c += 3) set_color((uint8_t)(C_ART + i), c[0], c[1], c[2]);
}

#ifdef PB_BUILDER
/* ===== Ab hier nur im Lader (PINBALL65): Aufbau von Maske und Grafik ===== */

/* Wandsegmente: x0, y0, x1, y1, Klasse */
static const int16_t SEGS[][5] = {
  {10, 156, 10, 640, M_WALL},
  {310, 156, 310, 640, M_WALL},
  {290, 200, 290, 640, M_WALL},           /* Innenwand Abschussbahn */
  {114, 34, 114, 70, M_WALL}, {138, 34, 138, 70, M_WALL},      /* Lanes C 6 4 */
  {162, 34, 162, 70, M_WALL}, {186, 34, 186, 70, M_WALL},
  /* v6.1: Orbit rundherum. Innenwaende parallel zum oberen Bogen, sie enden an
   * den aeusseren Lane-Waenden: links vom Orbit-Aufstieg, rechts von der Abschussbahn */
  {31, 170, 31, 146, M_WALL}, {31, 146, 35, 122, M_WALL}, {35, 122, 43, 99, M_WALL}, {43, 99, 56, 78, M_WALL}, {56, 78, 72, 60, M_WALL}, {72, 60, 92, 45, M_WALL}, {92, 45, 114, 34, M_WALL},
  /* v6.3: rechts keine innere Wand mehr; der obere Bogen ist eine Fuehrungsschiene
   * (M_RAIL, ohne Abprall), an der der Ball entlanglaeuft */
  /* v6.1: Tor nach unten zum dritten Flipper. Es ist offen, wenn der Ball in der
   * Abschussbahn nicht schnell nach oben laeuft; physics.c lenkt ihn dann hinaus. */
  {290, 140, 290, 172, M_WALL},
  {290, 172, 290, 200, M_GATE},
  {36, 297, 36, 428, M_WALL},             /* Begrenzung links unter der Orbit-Einfahrt */
  {46, 314, 10, 252, M_WALL},             /* linker Orbit, untere Wand parallel zur Schusslinie */
  {66, 293, 31, 233, M_WALL}, {31, 233, 31, 170, M_WALL},   /* linker Orbit, obere Wand */
  {162, 306, 170, 268, M_WALL}, {196, 306, 188, 268, M_WALL},  /* Scoop 1541 */
  {170, 268, 179, 256, M_WALL}, {179, 256, 188, 268, M_WALL},
  /* v6.1: alles ab den Target-Reihen 28 Pixel tiefer (Flipper bei y 590) */
  {10, 418, 36, 428, M_WALL},             /* Out-Lane links oben zu */
  {290, 418, 264, 428, M_WALL},           /* Out-Lane rechts oben zu ... */
  {290, 318, 264, 336, M_WALL},           /* ... und der Gang hinter den SID-Targets auch oben */
  {264, 336, 264, 428, M_WALL},
  /* Inlanes; v6.1: Trennwand ab y 452 (v6.0 entsprach 444, v5.7 456) */
  {40, 452, 40, 547, M_WALL}, {40, 547, 109, 588, M_WALL},     /* Inlane links */
  {260, 452, 260, 547, M_WALL}, {260, 547, 191, 588, M_WALL},  /* Inlane rechts */
  {100, 595, 83, 640, M_WALL},
  {200, 595, 217, 640, M_WALL},
  {64, 490, 64, 532, M_WALL}, {64, 532, 92, 552, M_WALL},      /* Slingshot links */
  {236, 490, 236, 532, M_WALL}, {236, 532, 208, 552, M_WALL},  /* Slingshot rechts */
  {64, 490, 92, 552, M_SLING_L},
  {236, 490, 208, 552, M_SLING_R},
  /* Einfahrt der Rampe: senkrecht bei x 128, Muendung mit kleinem Trichter.
   * v6.1: Seitenwaende bis y 282 und ein spitzes Dach dahinter (unter der Rampe):
   * von hinten kommt der Ball nicht mehr durch die Einfahrt, und auf dem Dach
   * kann er nicht liegen bleiben. Ein Ball in der Einfahrt ist ab y 300 auf der
   * Rampe und beruehrt diese Waende nicht. */
  {115, 282, 115, 330, M_WALL}, {141, 282, 141, 330, M_WALL},
  {115, 282, 129, 272, M_WALL}, {129, 272, 141, 282, M_WALL},
  {115, 330, 106, 344, M_WALL}, {141, 330, 150, 344, M_WALL},
  {290, 268, 279, 294, M_WALL},           /* Fuehrung ueber dem Drehpunkt des dritten Flippers */
};
#define NSEGS (sizeof(SEGS) / sizeof(SEGS[0]))

static const int16_t SLING_TRI[2][6] = {
  {64, 490, 64, 532, 92, 552},
  {236, 490, 236, 532, 208, 552},
};
/*
 * Targets als kurze Wandstuecke (Breite 5 wie Waende): x0, y0, x1, y1.
 * Jede Gruppe bildet eine lueckenlose, schraege Linie. Waagerechte Flaechen
 * und Luecken in Ballbreite waeren Ruheplaetze, weil die Schwerkraft bei uns
 * senkrecht wirkt (im PC-Test blieb der Ball zwischen zwei Targets liegen).
 */
static const int16_t TARGETS[N_TARGETS][4] = {
  /* VIC-II (v4.2): links, genau gegenueber den SID-Targets */
  {54, 420, 61, 409}, {61, 409, 69, 397}, {69, 397, 76, 386},
  /* 6510 (v4.2): schraeg links vom geraden Abstieg der Rampe, zeigt zum linken Flipper */
  {214, 196, 220, 205}, {221, 206, 227, 215}, {227, 217, 233, 226}, {234, 227, 240, 236},
  {246, 420, 239, 409}, {239, 409, 231, 397}, {231, 397, 224, 386},  /* SID: gegenueber VIC-II */
  /* PLA (v5.3): links oben, senkrecht direkt rechts neben dem aufsteigenden Rampenstueck */
  {60, 120, 60, 134}, {60, 139, 60, 153}, {60, 158, 60, 172},
};

/* Beim Aufbau: Band, in das gerade gezeichnet wird */
static int16_t band_y0, band_y1;

static void set_px(int16_t x, int16_t y, uint8_t c) {
  if (x < 0 || x >= TW || y < band_y0 || y >= band_y1) return;
  uint16_t yy = (uint16_t)(y - band_y0);
  MASKWIN[(yy << 5) + (yy << 3) + ((uint16_t)x >> 3)] |= MASK_BIT[x & 7];
  uint16_t r = (uint8_t)(((uint16_t)y >> 3) - cls_r0);
  uint8_t *g = &CLSWIN[(r << 5) + (r << 3) + ((uint16_t)x >> 3)];
  if (c > *g) *g = c;
}

static const int8_t DISC[13][2] = {
  {0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1},
  {2, 0}, {-2, 0}, {0, 2}, {0, -2},
};

static void stamp_seg(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t c) {
  int16_t dx = x1 - x0, dy = y1 - y0;
  int16_t adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
  int16_t n = (adx > ady ? adx : ady) * 2;
  if (n == 0) n = 1;
  int32_t sx = ((int32_t)dx << 8) / n, sy = ((int32_t)dy << 8) / n;
  int32_t px = (int32_t)x0 << 8, py = (int32_t)y0 << 8;
  for (int16_t i = 0; i <= n; i++) {
    int16_t cx = (int16_t)((px + 128) >> 8), cy = (int16_t)((py + 128) >> 8);
    if (cy >= band_y0 - 2 && cy < band_y1 + 2)
      for (uint8_t k = 0; k < 13; k++) set_px(cx + DISC[k][0], cy + DISC[k][1], c);
    px += sx; py += sy;
  }
}

static int16_t edge(int16_t ax, int16_t ay, int16_t bx, int16_t by, int16_t x, int16_t y) {
  return (int16_t)((bx - ax) * (y - ay) - (by - ay) * (x - ax));
}
static uint8_t in_tri(const int16_t *t, int16_t x, int16_t y) {
  int16_t e0 = edge(t[0], t[1], t[2], t[3], x, y);
  int16_t e1 = edge(t[2], t[3], t[4], t[5], x, y);
  int16_t e2 = edge(t[4], t[5], t[0], t[1], x, y);
  return (e0 >= 0 && e1 >= 0 && e2 >= 0) || (e0 <= 0 && e1 <= 0 && e2 <= 0);
}

static uint16_t SQ[161];

/* ---------- v7.1: Tischgrafik ---------- */
static const uint8_t ART_RGB[N_ART_COLORS][3] = {
  {16, 14, 50}, {18, 14, 52}, {20, 15, 54}, {22, 15, 56}, {24, 16, 57}, {26, 16, 58}, {28, 17, 59}, {30, 17, 60},
  {32, 18, 61}, {34, 18, 62}, {36, 19, 63}, {38, 19, 64}, {40, 20, 65}, {42, 20, 66}, {44, 21, 67}, {46, 21, 68},
  {112, 110, 232}, {44, 40, 150}, {36, 32, 128}, {150, 148, 255},
  {210, 50, 50}, {236, 128, 36}, {240, 206, 48}, {70, 170, 80}, {60, 100, 210}, {10, 8, 20},
  {18, 58, 38}, {12, 42, 28}, {150, 120, 52}, {205, 172, 82},
  {24, 24, 28}, {72, 72, 82}, {190, 190, 200}, {214, 214, 224},
  {206, 196, 168}, {150, 140, 115}, {60, 56, 50}, {230, 40, 40}, {80, 70, 60},
  {88, 66, 48}, {140, 112, 84}, {40, 30, 22}, {240, 232, 210},
  {110, 70, 36}, {120, 76, 38}, {130, 82, 40},
};
_Static_assert(sizeof ART_RGB / 3 == N_ART_COLORS, "Grafikfarben passen nicht");
void table_store_art_palette(void) {
  dma_copy(CPUADDR(ART_RGB), ATTIC_PAL, N_ART_COLORS * 3);
  art_palette_load();
}

/* Abgerundetes Rechteck (Radius r): Punkt innen? */
static uint8_t in_rrect(int16_t x, int16_t y, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t r) {
  if (x < x0 || x > x1 || y < y0 || y > y1) return 0;
  int16_t cx = x < x0 + r ? x0 + r : x > x1 - r ? x1 - r : x;
  int16_t cy = y < y0 + r ? y0 + r : y > y1 - r ? y1 - r : y;
  int16_t dx = x - cx, dy = y - cy;
  return dx * dx + dy * dy <= r * r;
}

/* Hintergrund der Tischgrafik an (x, y): Holzleisten, Bildschirm, Regenbogen,
 * Platine, sonst Farbverlauf */
static uint8_t art_bg(int16_t x, int16_t y) {
  if (x < 9 || x > 311) return (uint8_t)(C_WOOD0 + (x % 3));
  if (in_rrect(x, y, 34, 14, 286, 236, 26)) {
    if (!in_rrect(x, y, 44, 26, 276, 226, 16)) return C_SCR_EDGE;
    return (y % 3) == 0 ? C_SCR_LINE : C_SCR_IN;
  }
  int16_t s = y + (x >> 1) - 400;               /* Regenbogen: Streifen parallel zu y = 400 - x/2 */
  if (s >= -1 && s <= 45) {
    if (s < 0 || s == 45) return C_RB_EDGE;
    return (uint8_t)(C_RB0 + s / 9);
  }
  if (s > 50) {                                 /* Platine unterhalb des Regenbogens */
    int16_t gx = (x - 14) % 22, gy = (y - 470) % 26;
    if (y >= 456 && x >= 12 && x <= 308) {
      if (gx >= -2 && gx <= 2 && gy >= -2 && gy <= 2 && y >= 468 && x > 11 && (gx * gx + gy * gy) <= 5 && y < 600) return C_PAD;
      if (gx == 0 || (gy == 0 && y >= 470 && y < 600)) return C_COPPER;
    }
    return ((x & 3) == 1 && (y & 3) == 1 && ((x ^ y) & 4)) ? C_PCB_DK : C_PCB;
  }
  return (uint8_t)(C_GRAD0 + (y >> 6) + (y >> 7));
}

/* Punkt der Grafik setzen, aber nicht ueber Waenden */
static void band_put_bg(uint8_t *buf, int16_t x, int16_t y, uint8_t col) {
  if (x < 0 || x >= TW || y < band_y0 || y >= band_y1 || mask_solid(x, y)) return;
  buf[((uint16_t)(x >> 3) << 6) + ((y - band_y0) << 3) + (x & 7)] = col;
}
static void band_box(uint8_t *buf, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t fill, uint8_t edge) {
  int16_t ya = y0 < band_y0 ? band_y0 : y0, yb = y1 >= band_y1 ? band_y1 - 1 : y1;
  for (int16_t y = ya; y <= yb; y++)
    for (int16_t x = x0; x <= x1; x++)
      band_put_bg(buf, x, y, (x == x0 || x == x1 || y == y0 || y == y1) ? edge : fill);
}
/* Text in der 5x7-Schrift des Displays */
static void band_text5(uint8_t *buf, int16_t x, int16_t y, const char *str, uint8_t col) {
  for (; *str; str++, x += 6) {
    uint8_t ch = (uint8_t)*str;
    if (ch < 32 || ch > 90) continue;
    for (uint8_t r = 0; r < 7; r++) {
      uint8_t bits = FONT5X7[ch - 32][r];
      for (uint8_t c = 0; c < 5; c++) if (bits & (0x10 >> c)) band_put_bg(buf, x + c, y + r, col);
    }
  }
}
static void band_chip(uint8_t *b, int16_t x, int16_t y, int16_t w, const char *label) {
  if (band_y0 > y + 18 || band_y1 < y - 4) return;
  band_box(b, x, y, x + w, y + 13, C_CHIP, C_CHIP_EDGE);
  for (int16_t px = x + 3; px < x + w - 1; px += 5) {
    band_box(b, px, y - 3, px + 2, y - 1, C_PIN, C_PIN);
    band_box(b, px, y + 14, px + 2, y + 16, C_PIN, C_PIN);
  }
  band_box(b, x + 2, y + 5, x + 4, y + 8, C_CHIP_EDGE, C_CHIP_EDGE);
  int16_t n = 0;
  while (label[n]) n++;
  band_text5(b, x + (w + 1 - n * 6) / 2 + 1, y + 3, label, C_CHIP_TEXT);
}
static void band_key(uint8_t *b, int16_t x, int16_t y, const char *label) {
  if (band_y0 > y + 18 || band_y1 < y) return;
  band_box(b, x, y, x + 24, y + 16, C_KEY, C_KEY_EDGE);
  band_box(b, x + 2, y + 2, x + 22, y + 11, C_KEY_TOP, C_KEY_TOP);
  int16_t n = 0;
  while (label[n]) n++;
  band_text5(b, x + (25 - n * 6) / 2 + 1, y + 3, label, C_KEY_TEXT);
}
/* Motive: Einschalttext, Chips neben den Targets, Laufwerk 1541, Tasten */
static void band_art(uint8_t *b) {
  if (band_y0 < 122 && band_y1 > 108) band_text5(b, 73, 109, "**** PINBALL 65 BASIC V2 ****", C_SCR_TEXT);
  if (band_y0 < 131 && band_y1 > 117) band_text5(b, 94, 118, "38911 BASIC BYTES FREE", C_SCR_TEXT);
  if (band_y0 < 224 && band_y1 > 208) {
    band_text5(b, 134, 209, "READY.", C_SCR_TEXT);
    band_box(b, 170, 209, 174, 215, C_SCR_TEXT, C_SCR_TEXT);
  }
  band_chip(b, 40, 424, 44, "VIC-II");
  band_chip(b, 216, 424, 44, "SID");
  band_chip(b, 222, 244, 40, "6510");
  band_chip(b, 70, 178, 30, "PLA");
  if (band_y0 <= 300 && band_y1 > 242) {        /* Laufwerk um das Kick-Out-Hole */
    band_box(b, 150, 242, 208, 300, C_DRV, C_DRV_DK);
    band_box(b, 154, 290, 204, 294, C_DRV_SLOT, C_DRV_SLOT);
    band_box(b, 196, 280, 201, 284, C_DRV_LED, C_DRV_LED);
    band_text5(b, 153, 245, "1541", C_DRV_TEXT);
  }
  band_key(b, 12, 520, "RUN");
  band_key(b, 12, 538, "STOP");
  band_key(b, 265, 520, "RES");
  band_key(b, 265, 538, "TORE");
}

/* Zeichnet alle Formen, beschnitten auf das aktuelle Band */
static void build_band(void) {
  /* Oberer Bogen: Mittelpunkt (160,156), Radius 150, Breite 4 */
  for (int16_t y = band_y0 < 4 ? 4 : band_y0; y <= 156 && y < band_y1; y++) {
    uint16_t sy = SQ[156 - y];
    for (int16_t x = 8; x <= 312; x++) {
      int16_t ax = x - 160; if (ax < 0) ax = -ax;
      if (ax > 152) continue;
      uint16_t d2 = sy + SQ[ax];
      if (d2 >= 148u * 148u && d2 <= 152u * 152u) set_px(x, y, M_RAIL);
    }
  }
  for (uint8_t s = 0; s < NSEGS; s++) {
    int16_t ya = SEGS[s][1], yb = SEGS[s][3];
    int16_t lo = ya < yb ? ya : yb, hi = ya < yb ? yb : ya;
    if (hi + 2 < band_y0 || lo - 2 >= band_y1) continue;
    stamp_seg(SEGS[s][0], ya, SEGS[s][2], yb, (uint8_t)SEGS[s][4]);
  }
  for (uint8_t s = 0; s < 2; s++) {
    const int16_t *t = SLING_TRI[s];
    for (int16_t y = 488; y <= 554; y++) {
      if (y < band_y0 || y >= band_y1) continue;
      for (int16_t x = (s ? 206 : 62); x <= (s ? 238 : 94); x++)
        if (in_tri(t, x, y)) set_px(x, y, M_WALL);
    }
  }
  for (uint8_t i = 0; i < N_TARGETS; i++) {
    const int16_t *t = TARGETS[i];
    int16_t lo = t[1] < t[3] ? t[1] : t[3], hi = t[1] < t[3] ? t[3] : t[1];
    if (hi + 2 < band_y0 || lo - 2 >= band_y1) continue;
    stamp_seg(t[0], t[1], t[2], t[3], (uint8_t)(M_TARGET + i));
  }
}

void table_build_mask(void) {
  for (uint16_t i = 0; i <= 160; i++) SQ[i] = i * i;
  for (band_y0 = 0; band_y0 < TH; band_y0 += WIN_ROWS) {
    band_y1 = band_y0 + WIN_ROWS;
    if (band_y1 > TH) band_y1 = TH;
    for (uint16_t i = 0; i < WIN_ROWS * 40; i++) MASKWIN[i] = 0;
    for (uint16_t i = 0; i < CLS_ROWS * 40; i++) CLSWIN[i] = 0;
    cls_r0 = (uint8_t)(band_y0 >> 3);
    build_band();
    dma_copy(CPUADDR(MASKWIN), MASK_FAR + (uint32_t)(uint16_t)band_y0 * 40,
             (uint16_t)(band_y1 - band_y0) * 40);
    dma_copy(CPUADDR(CLSWIN), CLS_FAR + (uint16_t)cls_r0 * 40,
             (uint16_t)(band_y1 - band_y0) * 5);   /* /8 Zeilen * 40 Byte */
    build_progress(0, (uint8_t)(band_y0 / WIN_ROWS));
  }
  win_y0 = -1000;                       /* Fenster ungueltig, naechster Zugriff laedt neu */
}

/* ---------- Platzhaltergrafik ---------- */

/* Abstand eines Punktes zur Gummikante eines Slingshots <= 2.5 Pixel?
 * Beide Kanten sind 28 x 62 Pixel lang (Laenge^2 = 4628). Bedingung
 * cr^2 * 4 <= 4628 * 25 ist fuer ganze Zahlen gleichbedeutend mit |cr| <= 170;
 * so passt alles in 16 Bit. */
static uint8_t on_rubber(const int16_t *t, int16_t x, int16_t y) {
  int16_t ex = t[4] - t[0], ey = t[5] - t[1];
  int16_t px = x - t[0], py = y - t[1];
  int16_t dot = px * ex + py * ey;
  if (dot < 0 || dot > 4628) return 0;
  int16_t cr = px * ey - py * ex;
  return cr >= -170 && cr <= 170;
}

static uint8_t art_px(int16_t x, int16_t y) {
  if (mask_solid(x, y)) {
    if (y >= 486 && y <= 556) {
      if (x <= 96 && on_rubber(SLING_TRI[0], x, y)) return C_SLING0;
      if (x >= 204 && on_rubber(SLING_TRI[1], x, y)) return C_SLING0 + 1;
    }
    uint8_t k = mask_class(x, y);
    if (k >= M_TARGET) {                 /* v7.1: dunkle Fuge an den Enden jedes Targets */
      /* Das Klassenraster gilt je 8x8-Feld; fuer die Grafik zaehlt das naechste Target */
      uint8_t best = 0;
      int16_t bd = 0x7FFF;
      for (uint8_t i = 0; i < N_TARGETS; i++) {
        const int16_t *u = TARGETS[i];
        int16_t mx = (u[0] + u[2]) >> 1, my = (u[1] + u[3]) >> 1;
        int16_t ex = x - mx, ey = y - my;
        if (ex > 20 || ex < -20 || ey > 20 || ey < -20) continue;
        int16_t dd = ex * ex + ey * ey;
        if (dd < bd) { bd = dd; best = i; }
      }
      k = (uint8_t)(M_TARGET + best);
      const int16_t *t = TARGETS[k - M_TARGET];
      int16_t sx = t[2] - t[0], sy = t[3] - t[1];
      int16_t tp = (x - t[0]) * sx + (y - t[1]) * sy, l2 = sx * sx + sy * sy;
      int16_t l = sx < 0 ? -sx : sx, ly = sy < 0 ? -sy : sy;
      l = l > ly ? l + ly / 2 : ly + l / 2;     /* ungefaehre Laenge */
      if (tp < l || tp > l2 - l) return C_LANE;
      return (uint8_t)(C_TGT0 + (k - M_TARGET));
    }
    if (y > 0 && !mask_solid(x, y - 1)) return C_CHROME_HI;
    if (x > 0 && !mask_solid(x - 1, y)) return C_CHROME_MID;
    return C_CHROME_DK;
  }
  if (y >= 488 && y <= 554) {
    if (x >= 62 && x <= 94 && in_tri(SLING_TRI[0], x, y)) return C_SLING_BODY;
    if (x >= 206 && x <= 238 && in_tri(SLING_TRI[1], x, y)) return C_SLING_BODY;
  }
  if (x >= 293 && x <= 307 && y >= 100) return C_LANE;
  if (y >= 595) {                        /* Apron zwischen den schraegen Blechen */
    int16_t d = y - 595;
    if (x > 100 - (d * 17) / 45 && x < 200 + (d * 17) / 45) return C_LANE;
  }
  return art_bg(x, y);                    /* v7.1: gemalte Tischgrafik */
}

/* Kleine 3x5-Schrift fuer Beschriftungen auf dem Tisch */
static const char LBL_CHARS[] = "0123456789ABCDEFIKLNORSTVY-.*,\"P";
static const uint8_t LBL_GLYPHS[][5] = {
  {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
  {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 2, 2}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
  {2, 5, 7, 5, 5}, {6, 5, 6, 5, 6}, {3, 4, 4, 4, 3}, {6, 5, 5, 5, 6}, {7, 4, 6, 4, 7},
  {7, 4, 6, 4, 4}, {7, 2, 2, 2, 7}, {5, 5, 6, 5, 5}, {4, 4, 4, 4, 7}, {6, 5, 5, 5, 5},
  {2, 5, 5, 5, 2}, {6, 5, 6, 5, 5}, {3, 4, 2, 1, 6}, {7, 2, 2, 2, 2}, {5, 5, 5, 5, 2},
  {5, 5, 2, 2, 2}, {0, 0, 7, 0, 0}, {0, 0, 0, 0, 2}, {0, 5, 2, 5, 0}, {0, 0, 0, 2, 4},
  {5, 5, 0, 0, 0}, {6, 5, 6, 4, 4},
};

/* Zeichnet Text mit Vergroesserung s in das Band (nur Zeilen im Band) */
static void band_text(uint8_t *buf, int16_t x, int16_t y, const char *str, uint8_t s, uint8_t col) {
  for (; *str; str++, x += 4 * s) {
    uint8_t g = 0;
    while (LBL_CHARS[g] && LBL_CHARS[g] != *str) g++;
    if (!LBL_CHARS[g]) continue;            /* Leerzeichen und Unbekanntes */
    for (uint8_t r = 0; r < 5 * s; r++) {
      int16_t yy = y + r;
      if (yy < band_y0 || yy >= band_y1) continue;
      uint8_t bits = LBL_GLYPHS[g][r / s];
      for (uint8_t c = 0; c < 3 * s; c++) {
        if (!(bits & (4 >> (c / s)))) continue;
        int16_t xx = x + c;
        if (xx < 0 || xx >= TW) continue;
        buf[((uint16_t)(xx >> 3) << 6) + ((yy - band_y0) << 3) + (xx & 7)] = col;
      }
    }
  }
}

/* Gefuellter Kreis (r2 = Radius^2), nur Zeilen im Band */
static void band_disc(uint8_t *buf, int16_t cx, int16_t cy, int16_t r, uint8_t col) {
  for (int16_t yy = cy - r; yy <= cy + r; yy++) {
    if (yy < band_y0 || yy >= band_y1) continue;
    int16_t dy = yy - cy;
    for (int16_t xx = cx - r; xx <= cx + r; xx++) {
      int16_t dx = xx - cx;
      if (dx * dx + dy * dy > r * r + r / 2) continue;
      buf[((uint16_t)(xx >> 3) << 6) + ((yy - band_y0) << 3) + (xx & 7)] = col;
    }
  }
}

/* v6.8: Pfeil-Einsatz (Dreieck mit dunklem Rand), Spitze in Richtung ang (256er-Winkel) */
static const int16_t ARROWS[N_ARROWS][3] = {
  {128, 386, 192}, {179, 330, 192}, {72, 338, 171}, {277, 232, 192},
  {204, 258, 212}, {94, 432, 160}, {206, 432, 224}, {82, 146, 128},
};
static uint8_t in_arrow(int16_t u, int16_t v, int16_t tip, int16_t base, int16_t half) {
  if (v < 0) v = -v;
  if (u > tip || u < base) return 0;
  return v <= (int16_t)(((int32_t)(tip - u) * half) / (tip - base));
}
static void band_arrow(uint8_t *buf, int16_t cx, int16_t cy, uint8_t ang, uint8_t col) {
  int16_t c = (int16_t)(cosA(ang) >> 8), sn = (int16_t)(sinA(ang) >> 8);   /* +-256 */
  for (int16_t yy = cy - 10; yy <= cy + 10; yy++) {
    if (yy < band_y0 || yy >= band_y1) continue;
    int16_t dy = yy - cy;
    for (int16_t xx = cx - 10; xx <= cx + 10; xx++) {
      int16_t dx = xx - cx;
      int16_t u = dx * c + dy * sn, v = dy * c - dx * sn;      /* 8.8 */
      /* schlankes Dreieck: 16 Pixel lang, 11 breit, mit dunklem Rand */
      if (!in_arrow(u, v, 9 * 256, -7 * 256, 1408)) continue;
      uint8_t inner = in_arrow(u, v, 7 * 256, -6 * 256, 960);
      buf[((uint16_t)(xx >> 3) << 6) + ((yy - band_y0) << 3) + (xx & 7)] = inner ? col : C_LANE;
    }
  }
}

static void band_rect(uint8_t *buf, int16_t x0, int16_t y0, int16_t w, int16_t h, uint8_t col) {
  for (int16_t yy = y0; yy < y0 + h; yy++) {
    if (yy < band_y0 || yy >= band_y1) continue;
    for (int16_t xx = x0; xx < x0 + w; xx++)
      buf[((uint16_t)(xx >> 3) << 6) + ((yy - band_y0) << 3) + (xx & 7)] = col;
  }
}

/* Spinner: Strecke quer zur Einfahrt (Querrichtung P), 4 Streifen entlang
 * der Fahrtrichtung U; seit v5.4 waagerecht in der senkrechten Einfahrt */
static void band_spinner(uint8_t *b) {
  const fix16 PX = 65536L, PY = 0, UXS = 0, UYS = -65536L;
  const fix16 CXF = (fix16)128 << 16, CYF = (fix16)SPIN_Y << 16;
  for (int8_t t2 = -22; t2 <= 22; t2++)
    for (int8_t k2 = -3; k2 <= 3; k2++) {
      fix16 t = (fix16)t2 << 15, k = (fix16)k2 << 15;
      int16_t xx = (int16_t)((CXF + fmul(PX, t) + fmul(UXS, k)) >> 16);
      int16_t yy = (int16_t)((CYF + fmul(PY, t) + fmul(UYS, k)) >> 16);
      if (yy < band_y0 || yy >= band_y1) continue;
      uint8_t stripe = (uint8_t)((k2 + 3) * 4 / 7);
      b[((uint16_t)(xx >> 3) << 6) + ((yy - band_y0) << 3) + (xx & 7)] = (uint8_t)(C_SPIN0 + stripe);
    }
}

static void band_overlays(uint8_t *b) {
  band_art(b);
  /* Pop-Bumper: roter Ring, Kappe im Einschaltbild-Blau mit Cursor */
  for (uint8_t i = 0; i < N_BUMPERS; i++) {
    int16_t x = BUMPERS[i][0], y = BUMPERS[i][1];
    if (y + 14 < band_y0 || y - 14 >= band_y1) continue;
    band_disc(b, x, y, 13, (uint8_t)(C_BUMP0 + i));
    band_disc(b, x, y, 9, C_C64_DARK);
    band_rect(b, x - 6, y - 3, 3, 5, C_C64_LIGHT);   /* "READY."-Andeutung */
    band_rect(b, x - 2, y - 3, 3, 5, C_C64_LIGHT);
    band_rect(b, x + 3, y - 3, 3, 5, C_C64_LIGHT);   /* Cursor */
  }
  if (band_y0 < 112 && band_y1 > 94) band_text(b, 107, 94, "LOAD\"*\",8,1", 2, C_HOLE_RING);
  if (band_y0 < 64 && band_y1 > 50) {
    band_text(b, 125, 50, "C", 1, C_LABEL);
    band_text(b, 149, 50, "6", 1, C_LABEL);
    band_text(b, 173, 50, "4", 1, C_LABEL);
  }
  /* Lampen der oberen Lanes */
  if (band_y0 < 66 && band_y1 > 56)
    for (uint8_t i = 0; i < 3; i++) band_disc(b, 126 + 24 * i, 61, 4, (uint8_t)(C_TOP0 + i));
  for (uint8_t i = 0; i < N_ARROWS; i++)
    if (band_y0 < ARROWS[i][1] + 11 && band_y1 > ARROWS[i][1] - 11)
      band_arrow(b, ARROWS[i][0], ARROWS[i][1], (uint8_t)ARROWS[i][2], (uint8_t)(C_ARROW0 + i));
  /* Kick-Out-Hole "1541" */
  if (band_y0 < KICK_Y + 20 && band_y1 > KICK_Y - 12) {
    band_disc(b, KICK_X, KICK_Y, 10, C_KICK_RING);
    band_disc(b, KICK_X, KICK_Y, 8, C_HOLE);
  }
  /* Spinner quer in der schraegen Einfahrt: vier Streifen, deren Farben wandern */
  if (band_y0 < SPIN_Y + 14 && band_y1 > SPIN_Y - 14) band_spinner(b);
  /* Lampen L O A D: O und A an den Inlanes, L und D sind Roll-Over-Kontakte (v5.1) */
  {
    static const int16_t LX[4] = { ROLL_LX, 52, 248, ROLL_DX }, LY[4] = { ROLL_LY, 488, 488, ROLL_DY };
    static const char *LL[4] = { "L", "O", "A", "D" };
    for (uint8_t i = 0; i < 4; i++) {
      if (band_y0 >= LY[i] + 6 || band_y1 <= LY[i] - 6) continue;
      if (i == 0 || i == 3) band_disc(b, LX[i], LY[i], ROLL_R, C_CHROME_MID);   /* Chromring */
      band_disc(b, LX[i], LY[i], 5, (uint8_t)(C_LOAD0 + i));
      band_text(b, LX[i] - 1, LY[i] - 2, LL[i], 1, C_BG);
    }
  }
  /* Kickback "FIRE" in der linken Outlane */
  if (band_y0 < 632 && band_y1 > 616) { band_rect(b, 18, 618, 14, 10, C_KICKBACK); }
  /* Apron: Einschaltbild */
  if (band_y0 < 640 && band_y1 > 610) {   /* v6.1: tiefer, Reihenfolge wie beim Einschalten */
    band_text(b, 106, 615, "38911 BASIC BYTES FREE", 1, C_C64_LIGHT);
    band_text(b, 119, 626, "READY.", 2, C_C64_LIGHT);
    band_rect(b, 167, 626, 6, 10, C_C64_LIGHT);
  }
}

/* ---------- Rampe ----------
 * Entlang des Weges in halben Pixeln: Querschnitt von -7 bis +7 Pixel,
 * aussen Chromschienen, dazwischen ein durchsichtiger Boden als Raster
 * (jeder zweite Pixel). Dasselbe Bild liegt als Deckbild im Attic-RAM
 * (ATTIC_OVL), damit der Ball unter der Bruecke verschwindet. */
#define RAMP_Y_MIN 70
#define RAMP_Y_MAX 360
static void ramp_stamp(uint8_t *buf) {
  if (band_y1 <= RAMP_Y_MIN || band_y0 > RAMP_Y_MAX) return;
  uint8_t seg = 0;
  for (fix16 s = 0; s <= RAMP_LEN; s += 32768L) {
    while (seg + 1 < RAMP_NSEG && s >= RAMP_SEGS[seg + 1].s0) seg++;
    const ramp_seg_t *g = &RAMP_SEGS[seg];
    fix16 d = s - g->s0;
    fix16 y = RS_P(g->y0) + fmul(RS_U(g->uy), d);
    int16_t yi = (int16_t)(y >> 16);
    if (yi < band_y0 - 8 || yi > band_y1 + 8) continue;
    fix16 x = RS_P(g->x0) + fmul(RS_U(g->ux), d);
    for (int8_t o2 = -14; o2 <= 14; o2++) {
      fix16 o = (fix16)o2 << 15;
      int16_t xx = (int16_t)((x - fmul(RS_U(g->uy), o)) >> 16), yy = (int16_t)((y + fmul(RS_U(g->ux), o)) >> 16);
      if (yy < band_y0 || yy >= band_y1 || xx < 0 || xx >= TW) continue;
      uint8_t *p = &buf[((uint16_t)(xx >> 3) << 6) + ((yy - band_y0) << 3) + (xx & 7)];
      uint8_t ao = (uint8_t)(o2 < 0 ? -o2 : o2);
      if (ao >= 12) *p = C_RAMP_RAIL;
      else if (ao >= 11) { if (*p != C_RAMP_RAIL) *p = C_RAMP_RAIL_DK; }
      else if (((xx ^ yy) & 1) && *p != C_RAMP_RAIL && *p != C_RAMP_RAIL_DK) *p = C_RAMP_FLOOR;
    }
  }
}

/* Deckbild der Rampe: nur die Rampe, alles andere 0 (durchsichtig) */
void table_build_overlay(void) {
  uint8_t *b = ROWBUF;
  for (uint8_t k = 0; k < 4; k++) dma_fill(ATTIC_OVL + (uint32_t)k * 51200u, 0, 51200u);
  for (band_y0 = RAMP_Y_MIN & ~7; band_y0 <= RAMP_Y_MAX; band_y0 += 8) {
    band_y1 = band_y0 + 8;
    for (uint16_t i = 0; i < ROW_BYTES; i++) b[i] = 0;
    ramp_stamp(b);
    uint32_t dst = ATTIC_OVL + (uint32_t)(uint16_t)band_y0 * 8;
    for (uint8_t c = 0; c < 40; c++, dst += STRIP_BYTES)
      dma_copy(CPUADDR(b + (uint16_t)c * 64), dst, 64);
    build_progress(2, (uint8_t)((band_y0 - (RAMP_Y_MIN & ~7)) >> 3));
  }
}

void table_build_art(void) {
  uint8_t *b = ROWBUF;
  for (band_y0 = 0; band_y0 < TH; band_y0 += 8) {
    band_y1 = band_y0 + 8;
    mask_load(band_y0 - 1);
    for (uint8_t py = 0; py < 8; py++) {
      int16_t y = band_y0 + py;
      for (int16_t x = 0; x < TW; x++)
        b[((uint16_t)(x >> 3) << 6) + (py << 3) + (x & 7)] = art_px(x, y);
    }
    band_overlays(b);
    ramp_stamp(b);
    /* Jede Zeichenspalte in ihren Streifen im Attic-RAM */
    uint32_t dst = ATTIC_BG + (uint32_t)(uint16_t)band_y0 * 8;
    for (uint8_t c = 0; c < 40; c++, dst += STRIP_BYTES)
      dma_copy(CPUADDR(b + (uint16_t)c * 64), dst, 64);
    build_progress(1, (uint8_t)(band_y0 >> 3));
  }
  VICIV.bordercol = 0;
  win_y0 = -1000;
}
#endif /* PB_BUILDER */
