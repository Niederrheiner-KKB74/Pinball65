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

#ifdef PB_BUILDER
/* v7.4: Der Lader setzt die Palette und legt sie im Attic-RAM ab (ATTIC_PAL);
 * das Spiel uebernimmt sie von dort mit palette_load (spart dort rund 700 Byte) */
static uint8_t PAL_SH[C_ART_END][3];
static void pal_set(uint8_t i, uint8_t r, uint8_t g, uint8_t b) {
  if (i < C_ART_END) { PAL_SH[i][0] = r; PAL_SH[i][1] = g; PAL_SH[i][2] = b; }
  set_color(i, r, g, b);
}
#define set_color pal_set
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
  set_color(C_GATE, 172, 172, 184);
  set_color(C_PLUNGER, 200, 200, 210);
  set_color(C_PLUNGER_DK, 110, 110, 120);
  set_color(C_PLUNGER_TIP, 216, 48, 64);
  set_color(C_CIA0, 100, 80, 24);
  set_color(C_CIA1, 100, 80, 24);
  set_color(C_SAVE, 140, 112, 84);
  for (uint8_t k = 0; k < 4; k++) set_color((uint8_t)(C_MULT0 + k), MULT_RGB[k][0][0], MULT_RGB[k][0][1], MULT_RGB[k][0][2]);
  set_color(C_RAMP_RAIL, 220, 230, 255);    /* Rampe: Chromschienen ... */
  set_color(C_RAMP_RAIL_DK, 120, 130, 190);
  set_color(C_RAMP_FLOOR, 70, 150, 210);    /* ... und durchsichtiger Boden (Raster) */
  for (uint8_t k = 0; k < 4; k++) set_color((uint8_t)(C_SPIN0 + k), 90, 90, 110);
}


#undef set_color
#endif

void palette_load(void) {
  dma_copy(ATTIC_PAL, CPUADDR(COLBUF), C_ART_END * 3);
  const uint8_t *c = COLBUF + 3;
  for (uint8_t i = 1; i < C_ART_END; i++, c += 3) set_color(i, c[0], c[1], c[2]);
}

#ifdef PB_BUILDER
/* ===== Ab hier nur im Lader (PINBALL65): Aufbau von Maske und Grafik ===== */

/* Wandsegmente: x0, y0, x1, y1, Klasse */
static const int16_t SEGS[][5] = {
  {10, 156, 10, 640, M_WALL},
  {310, 156, 310, 640, M_WALL},
  {290, 140, 290, 640, M_WALL},           /* Innenwand Abschussbahn */
  {114, 34, 114, 70, M_WALL}, {138, 34, 138, 70, M_WALL},      /* Lanes C 6 4 */
  {162, 34, 162, 70, M_WALL}, {186, 34, 186, 70, M_WALL},
  /* v6.1: Orbit rundherum. Innenwaende parallel zum oberen Bogen, sie enden an
   * den aeusseren Lane-Waenden: links vom Orbit-Aufstieg, rechts von der Abschussbahn */
  {31, 170, 31, 146, M_WALL}, {31, 146, 35, 122, M_WALL}, {35, 122, 43, 99, M_WALL}, {43, 99, 56, 78, M_WALL}, {56, 78, 72, 60, M_WALL}, {72, 60, 92, 45, M_WALL}, {92, 45, 114, 34, M_WALL},
  /* v6.3: rechts keine innere Wand mehr; der obere Bogen ist eine Fuehrungsschiene
   * (M_RAIL, ohne Abprall), an der der Ball entlanglaeuft */
  /* v7.2: Einwegtor oben quer ueber der Abschussbahn: der Abschuss geht nach oben
   * hindurch, danach ist es zu. Ein Ball aus dem Orbit rollt darauf nach links
   * ueber die Wand der Bahn zum dritten Flipper. */
  {290, 140, 301, 132, M_GATE_UP},        /* endet vor dem Feld des Bogens (Klassen je 8x8) */
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
   * Rampe und beruehrt diese Waende nicht. v8.2: Trichter ab y 318 statt 330 und weiter
   * (100/156 statt 106/150): Rampe vom rechten Flipper fast dreimal so oft. */
  {115, 282, 115, 318, M_WALL}, {141, 282, 141, 318, M_WALL},
  {115, 282, 129, 272, M_WALL}, {129, 272, 141, 282, M_WALL},
  {115, 318, 100, 346, M_WALL}, {141, 318, 156, 346, M_WALL},   /* v8.2: Trichter weiter */
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
  {255, 252, 240}, {226, 218, 196}, {160, 150, 128}, {60, 50, 150}, {8, 6, 18},
  {66, 66, 76}, {8, 8, 10}, {120, 120, 130}, {140, 140, 150},
  {222, 214, 192}, {250, 246, 232}, {150, 140, 118}, {176, 166, 142},
  {206, 164, 64}, {252, 216, 116}, {140, 100, 30}, {84, 56, 16},
};
_Static_assert(sizeof ART_RGB / 3 == N_ART_COLORS, "Grafikfarben passen nicht");
void table_store_art_palette(void) {
  for (uint8_t i = 0; i < N_ART_COLORS; i++) pal_set((uint8_t)(C_ART + i), ART_RGB[i][0], ART_RGB[i][1], ART_RGB[i][2]);
  dma_copy(CPUADDR(PAL_SH), ATTIC_PAL, C_ART_END * 3);
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
/* v8.6: Leiterbahnen im unteren Tisch (waagerecht, senkrecht oder im 45-Grad-Winkel),
 * von den Chips zu Durchkontaktierungen; bewusst unregelmaessig und nicht zu dicht */
static const int16_t TRACES[][4] = {
  {97, 458, 72, 458}, {72, 458, 52, 478}, {52, 478, 52, 520},           /* CIA 1 nach links */
  {97, 470, 84, 470}, {84, 470, 84, 488},
  {108, 496, 108, 512}, {108, 512, 120, 524}, {120, 524, 120, 562},     /* CIA 1 nach unten */
  {223, 462, 236, 462}, {236, 462, 250, 476}, {250, 476, 250, 534},     /* CIA 2 nach rechts */
  {212, 496, 212, 508}, {212, 508, 196, 524}, {196, 524, 196, 548},     /* CIA 2 nach unten */
  {58, 442, 30, 442}, {30, 442, 30, 500},                               /* VIC-II nach aussen */
  {244, 442, 270, 442}, {270, 442, 286, 458}, {286, 458, 286, 516},     /* SID nach aussen */
  {24, 560, 64, 560}, {64, 560, 80, 576}, {80, 576, 100, 576},          /* unten links */
  {296, 552, 262, 552}, {262, 552, 248, 566},                           /* unten rechts */
};
#define N_TRACES (sizeof TRACES / sizeof TRACES[0])
static const int16_t PADS[][2] = {
  {52, 520}, {84, 488}, {120, 562}, {250, 534}, {196, 548}, {30, 500}, {286, 516},
  {24, 560}, {100, 576}, {296, 552}, {248, 566}, {142, 584}, {176, 470},
};
#define N_PADS (sizeof PADS / sizeof PADS[0])
static uint8_t on_trace(const int16_t *t, int16_t x, int16_t y) {
  int16_t dx = t[2] - t[0], dy = t[3] - t[1];
  int16_t lo = t[0] < t[2] ? t[0] : t[2], hi = t[0] < t[2] ? t[2] : t[0];
  int16_t ly = t[1] < t[3] ? t[1] : t[3], hy = t[1] < t[3] ? t[3] : t[1];
  if (x < lo || x > hi || y < ly || y > hy) return 0;
  if (!dy) return y == t[1];
  if (!dx) return x == t[0];
  return (x - t[0]) * (dy > 0 ? 1 : -1) * (dx > 0 ? 1 : -1) == (y - t[1]);
}

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
    if (y >= 440 && y < 600 && x >= 12 && x <= 308) {   /* v8.6: Leiterbahnen statt Raster */
      for (uint8_t i = 0; i < N_PADS; i++) {
        int16_t dx = x - PADS[i][0], dy = y - PADS[i][1];
        if (dx * dx + dy * dy <= 5) return C_PAD;
      }
      for (uint8_t i = 0; i < N_TRACES; i++) if (on_trace(TRACES[i], x, y)) return C_COPPER;
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
/* v8.1: Chip im DIL-Gehaeuse mit dicht stehenden Beinchen (pins je Seite, Abstand 2),
 * plastischem Gehaeuse, Kerbe, Pin-1-Markierung, Name und Teilenummer.
 * cer: Keramik (cremefarben) mit goldenem Deckel, Beschriftung auf dem Deckel */
static void band_text(uint8_t *buf, int16_t x, int16_t y, const char *str, uint8_t s, uint8_t col);
static void band_chip(uint8_t *b, int16_t x, int16_t y, uint8_t pins, const char *name, const char *part, uint8_t cer) {
  int16_t w = pins * 2 + 2, h = 17;
  if (band_y0 > y + h + 3 || band_y1 < y - 4) return;
  for (uint8_t k = 0; k < pins; k++) {
    int16_t px = x + 1 + 2 * k;
    band_put_bg(b, px, y - 3, C_CHIP_TEXT); band_put_bg(b, px, y - 2, C_PIN); band_put_bg(b, px, y - 1, C_PIN_DK);
    band_put_bg(b, px, y + h, C_PIN_DK); band_put_bg(b, px, y + h + 1, C_PIN); band_put_bg(b, px, y + h + 2, C_CHIP_PART);
  }
  uint8_t body = cer ? C_CER : C_CHIP, hi = cer ? C_CER_HI : C_CHIP_HI, lo = cer ? C_CER_LO : C_CHIP_LO,
          edge = cer ? C_CER_EDGE : C_CHIP_EDGE;
  band_box(b, x, y, x + w - 1, y + h - 1, body, edge);
  band_box(b, x + 1, y, x + w - 2, y, hi, hi);
  band_box(b, x + 1, y + h - 1, x + w - 2, y + h - 1, lo, lo);
  for (int16_t dy = -3; dy <= 3; dy++)                      /* Kerbe an der Stirnseite */
    for (int16_t dx = 0; dx < 4; dx++)
      if (dx * dx + dy * dy <= 9) band_put_bg(b, x + dx, y + h / 2 + dy, lo);
  band_box(b, x + 5, y + h - 4, x + 6, y + h - 4, lo, lo); /* Pin-1-Markierung */
  int16_t n = 0, m = 0;
  while (name[n]) n++;
  while (part[m]) m++;
  if (cer) {                                                 /* goldener Deckel mit Beschriftung */
    int16_t lx = x + 6, ly = y + 2, lw = w - 12;
    band_box(b, lx, ly, lx + lw - 1, ly + 12, C_GOLD, C_GOLD);
    band_box(b, lx, ly, lx + lw - 1, ly, C_GOLD_HI, C_GOLD_HI);
    band_box(b, lx, ly, lx, ly + 12, C_GOLD_HI, C_GOLD_HI);
    band_box(b, lx + 1, ly + 12, lx + lw - 1, ly + 12, C_GOLD_DK, C_GOLD_DK);
    band_box(b, lx + lw - 1, ly + 1, lx + lw - 1, ly + 12, C_GOLD_DK, C_GOLD_DK);
    band_text(b, x + (w - (n * 4 - 1)) / 2, y + 4, name, 1, C_GOLD_TEXT);
    band_text(b, x + (w - (m * 4 - 1)) / 2, y + 10, part, 1, C_GOLD_DK);
  } else {
    band_text5(b, x + (w - (n * 6 - 1)) / 2 + 1, y + 2, name, C_CHIP_TEXT);
    band_text(b, x + (w - (m * 4 - 1)) / 2 + 1, y + 11, part, 1, C_CHIP_PART);
  }
}
/* v8.4: CIA 6526 hochkant (40 Beinchen), Beschriftung CIA / Nummer / 6526 untereinander */
static void band_chip_v(uint8_t *b, int16_t x, int16_t y, const char *num) {
  const int16_t w = 17, h = 42;
  if (band_y0 > y + h || band_y1 < y) return;
  for (uint8_t k = 0; k < 20; k++) {
    int16_t py = y + 1 + 2 * k;
    band_put_bg(b, x - 3, py, C_CHIP_TEXT); band_put_bg(b, x - 2, py, C_PIN); band_put_bg(b, x - 1, py, C_PIN_DK);
    band_put_bg(b, x + w, py, C_PIN_DK); band_put_bg(b, x + w + 1, py, C_PIN); band_put_bg(b, x + w + 2, py, C_CHIP_PART);
  }
  band_box(b, x, y, x + w - 1, y + h - 1, C_CHIP, C_CHIP_EDGE);
  band_box(b, x, y + 1, x, y + h - 2, C_CHIP_HI, C_CHIP_HI);
  band_box(b, x + w - 1, y + 1, x + w - 1, y + h - 2, C_CHIP_LO, C_CHIP_LO);
  for (int16_t dx = -3; dx <= 3; dx++)                      /* Kerbe oben */
    for (int16_t dy = 0; dy < 4; dy++)
      if (dx * dx + dy * dy <= 9) band_put_bg(b, x + w / 2 + dx, y + dy, C_CHIP_LO);
  band_box(b, x + 3, y + 5, x + 3, y + 6, C_CHIP_LO, C_CHIP_LO);   /* Pin-1-Markierung */
  band_text5(b, x, y + 8, "CIA", C_CHIP_TEXT);
  band_text5(b, x + 6, y + 18, num, C_CHIP_TEXT);
  band_text(b, x + 1, y + 29, "6526", 1, C_CHIP_PART);
}
/* v7.3: quadratische Taste wie beim C64 (26 x 26), Oberseite leuchtet bei Ball-Save */
static void band_key(uint8_t *b, int16_t x, int16_t y, const char *l1, const char *l2) {
  if (band_y0 > y + 26 || band_y1 < y) return;
  band_box(b, x, y, x + 25, y + 25, C_KEY, C_KEY_EDGE);
  band_box(b, x + 3, y + 2, x + 22, y + 19, C_SAVE, C_SAVE);
  int16_t n1 = 0, n2 = 0;
  while (l1[n1]) n1++;
  while (l2[n2]) n2++;
  band_text(b, x + 13 - n1 * 2, y + 4, l1, 1, C_KEY_TEXT);
  band_text(b, x + 13 - n2 * 2, y + 11, l2, 1, C_KEY_TEXT);
}
/* v8.6: Widerstand senkrecht: Draht, beiges Gehaeuse, drei Farbringe und Toleranzring */
static void band_resistor(uint8_t *b, int16_t x, int16_t y, uint8_t c1, uint8_t c2, uint8_t c3) {
  if (band_y0 > y + 18 || band_y1 < y) return;
  band_box(b, x + 2, y, x + 2, y + 17, C_PIN, C_PIN);               /* Anschlussdraht */
  band_box(b, x + 1, y, x + 3, y + 1, C_PAD, C_PAD);                /* Loetaugen */
  band_box(b, x + 1, y + 16, x + 3, y + 17, C_PAD, C_PAD);
  band_box(b, x, y + 4, x + 4, y + 13, C_DRV, C_DRV_DK);            /* Gehaeuse */
  band_box(b, x + 1, y + 4, x + 1, y + 13, C_KEY_TEXT, C_KEY_TEXT); /* Glanz */
  band_box(b, x, y + 6, x + 4, y + 6, c1, c1);
  band_box(b, x, y + 8, x + 4, y + 8, c2, c2);
  band_box(b, x, y + 10, x + 4, y + 10, c3, c3);
  band_box(b, x, y + 12, x + 4, y + 12, C_GOLD, C_GOLD);
}
/* v8.6: Elko: blauer Becher mit hellem Streifen und Deckel */
static void band_cap(uint8_t *b, int16_t x, int16_t y) {
  if (band_y0 > y + 14 || band_y1 < y) return;
  band_box(b, x, y, x + 7, y + 11, C_RB0 + 4, C_RB_EDGE);
  band_box(b, x + 1, y + 1, x + 6, y + 2, C_CHIP_TEXT, C_CHIP_TEXT); /* Deckel */
  band_box(b, x + 5, y + 3, x + 5, y + 10, C_SCR_EDGE, C_SCR_EDGE);  /* Minus-Streifen */
  band_box(b, x + 2, y + 12, x + 2, y + 13, C_PIN, C_PIN);
  band_box(b, x + 5, y + 12, x + 5, y + 13, C_PIN, C_PIN);
}
/* v8.6: Ecken links (x 12-34, y 302-376, oberhalb des Regenbogens) und rechts
 * (x 266-288, y 338-426): kleine Platine mit Leiterbahnen, Widerstaenden, Elko */
static void band_parts(uint8_t *b, uint8_t right) {
  int16_t x0 = right ? 267 : 13, y0 = right ? 340 : 304, y1 = right ? 424 : 374;
  if (band_y0 > y1 || band_y1 < y0) return;
  if (!right) band_box(b, x0, y0, x0 + 20, y1, C_PCB, C_PCB_DK);   /* links: eigene Platine auf Violett */
  band_box(b, x0 + 17, y0 + 4, x0 + 17, y1 - 4, C_COPPER, C_COPPER);   /* Leiterbahnen */
  band_box(b, x0 + 9, y0 + 4, x0 + 17, y0 + 4, C_COPPER, C_COPPER);
  band_box(b, x0 + 9, y1 - 4, x0 + 17, y1 - 4, C_COPPER, C_COPPER);
  band_resistor(b, x0 + 3, y0 + 3, C_RB0 + 0, C_RB0 + 0, C_RB0 + 1);   /* rot rot orange */
  band_resistor(b, x0 + 10, y0 + 22, C_RB0 + 2, C_RB0 + 4, C_RB0 + 0); /* gelb blau rot */
  if (right) {
    band_resistor(b, x0 + 3, y0 + 44, C_WOOD0, C_CHIP, C_RB0 + 0);     /* braun schwarz rot */
    band_cap(b, x0 + 8, y0 + 66);
  } else band_cap(b, x0 + 3, y0 + 50);
}
/* Motive: Einschalttext, Chips neben den Targets, Laufwerk 1541, Tasten */
static void band_art(uint8_t *b) {
  if (band_y0 < 122 && band_y1 > 108) band_text5(b, 73, 109, "**** PINBALL 65 BASIC V2 ****", C_SCR_TEXT);
  if (band_y0 < 131 && band_y1 > 117) band_text5(b, 94, 118, "38911 BASIC BYTES FREE", C_SCR_TEXT);
  if (band_y0 < 224 && band_y1 > 208) {
    band_text5(b, 134, 209, "READY.", C_SCR_TEXT);
    band_box(b, 170, 209, 174, 215, C_SCR_TEXT, C_SCR_TEXT);
  }
  band_chip(b, 40, 422, 20, "VIC-II", "6569R3", 1);   /* v8.1: echte Teilenummern */
  band_chip(b, 223, 422, 14, "SID", "6581", 0);
  band_chip(b, 221, 241, 20, "6510", "CPU", 0);
  band_chip(b, 70, 176, 14, "PLA", "906114", 0);
  band_chip_v(b, 100, 452, "1");                    /* v8.4: CIA 1 und CIA 2 neben der Bonus-Leiter */
  band_chip_v(b, 203, 452, "2");
  if (band_y0 <= 298 && band_y1 > 232) {        /* v7.2: 5,25-Zoll-Diskette um das Kick-Out-Hole */
    const int16_t X0 = KICK_X - 30, Y0 = KICK_Y - 34, X1 = KICK_X + 30, Y1 = KICK_Y + 30;
    band_box(b, X0, Y0, X1, Y1, C_CHIP, C_CHIP_EDGE);                 /* schwarze Huelle */
    band_box(b, X1 - 2, Y0 + 8, X1, Y0 + 15, C_GRAD0 + 4, C_GRAD0 + 4); /* Schreibschutzkerbe */
    band_box(b, X0 + 8, Y0 + 3, X1 - 8, Y0 + 14, C_KEY_TEXT, C_DRV_DK); /* Etikett */
    band_text5(b, KICK_X - 11, Y0 + 5, "1541", C_DRV_TEXT);
    for (int16_t y = KICK_Y - 15; y <= KICK_Y + 15; y++)              /* Mittelloch: Ring der Scheibe */
      for (int16_t x = KICK_X - 15; x <= KICK_X + 15; x++) {
        int16_t dx = x - KICK_X, dy = y - KICK_Y, d = dx * dx + dy * dy;
        if (d <= 225 && d >= 144) band_put_bg(b, x, y, C_KEY);
      }
    /* Schreib-/Leseoeffnung, oben und unten abgerundet wie bei einer echten Diskette (v8.3) */
    for (int16_t y = KICK_Y + 15; y <= Y1 - 2; y++)
      for (int16_t x = KICK_X - 3; x <= KICK_X + 3; x++) {
        int16_t cy = y < KICK_Y + 18 ? KICK_Y + 18 : y > Y1 - 5 ? Y1 - 5 : y, dx = x - KICK_X, dy = y - cy;
        int16_t d = dx * dx + dy * dy;
        if (d <= 10) band_put_bg(b, x, y, d >= 5 ? C_DRV_DK : C_KEY);
      }
    band_box(b, KICK_X + 18, KICK_Y - 3, KICK_X + 21, KICK_Y + 0, C_KEY, C_KEY); /* Indexloch */
  }
  /* v8.6: Bauteile in den abgeschlossenen Ecken am Rand (dort rollt keine Kugel) */
  band_parts(b, 0);
  band_parts(b, 1);
  band_key(b, 12, 526, "RUN", "STOP");
  band_key(b, 263, 526, "CLR", "HOME");
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

/* v8.6: Slingshot-Koerper als kleine Platine (u, v: Abstand von der senkrechten
 * Kante und von oben; rechts gespiegelt): Leiterbahnen, Loetaugen, ein SMD-Bauteil */
static uint8_t sling_pcb(int16_t u, int16_t v) {
  if ((u >= 1 && u <= 3 && v >= 13 && v <= 15) || (u >= 17 && u <= 19 && v >= 33 && v <= 35) ||
      (u >= 3 && u <= 5 && v >= 19 && v <= 21) || (u >= 19 && u <= 21 && v >= 43 && v <= 45)) return C_PAD;
  if (v >= 49 && v <= 51 && u >= 6 && u <= 13) return (u == 6 || u == 13) ? C_PIN : C_CHIP;
  if ((v == 14 && u <= 10) || (u == 10 && v >= 14 && v <= 34) || (v == 34 && u >= 10 && u <= 18) ||
      (u == 4 && v >= 20 && v <= 44) || (v == 44 && u >= 4 && u <= 20) ||
      (u == 9 && v >= 44 && v <= 50)) return C_COPPER;
  return ((u + v) & 3) == 0 && (v & 1) ? C_PCB : C_PCB_DK;   /* dunklere Platine als die Spielflaeche */
}

static uint8_t art_px(int16_t x, int16_t y) {
  if (mask_solid(x, y)) {
    if (y >= 486 && y <= 556) {
      if (x <= 96 && on_rubber(SLING_TRI[0], x, y)) return C_SLING0;
      if (x >= 204 && on_rubber(SLING_TRI[1], x, y)) return C_SLING0 + 1;
      /* v8.6: Inneres der Slingshots als Platine, Rand bleibt Chrom */
      if (mask_solid(x - 1, y) && mask_solid(x + 1, y) && mask_solid(x, y - 1) && mask_solid(x, y + 1)) {
        if (x <= 96 && in_tri(SLING_TRI[0], x, y)) return sling_pcb(x - 64, y - 490);
        if (x >= 204 && in_tri(SLING_TRI[1], x, y)) return sling_pcb(236 - x, y - 490);
      }
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
    if (k == M_GATE_UP && !(x <= 292 && y > 139)) return C_GATE;   /* v7.2: Tor (Farbe zeigt offen/zu) */
    /* v8.3: Fuehrungen an der Rampeneinfahrt und am 1541-Loch als Drahtfuehrung:
     * nur die Kanten in Chrom, innen scheint die Tischgrafik durch */
    if (((x >= 96 && x <= 160 && y >= 278 && y <= 350) || (x >= 158 && x <= 200 && y >= 250 && y <= 310)) &&
        mask_solid(x, y - 1) && mask_solid(x, y + 1) && mask_solid(x - 1, y) && mask_solid(x + 1, y))
      return art_bg(x, y);
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
static const char LBL_CHARS[] = "0123456789ABCDEFIKLNORSTVY-.*,\"PHMU";
static const uint8_t LBL_GLYPHS[][5] = {
  {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
  {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 2, 2}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
  {2, 5, 7, 5, 5}, {6, 5, 6, 5, 6}, {3, 4, 4, 4, 3}, {6, 5, 5, 5, 6}, {7, 4, 6, 4, 7},
  {7, 4, 6, 4, 4}, {7, 2, 2, 2, 7}, {5, 5, 6, 5, 5}, {4, 4, 4, 4, 7}, {6, 5, 5, 5, 5},
  {2, 5, 5, 5, 2}, {6, 5, 6, 5, 5}, {3, 4, 2, 1, 6}, {7, 2, 2, 2, 2}, {5, 5, 5, 5, 2},
  {5, 5, 2, 2, 2}, {0, 0, 7, 0, 0}, {0, 0, 0, 0, 2}, {0, 5, 2, 5, 0}, {0, 0, 0, 2, 4},
  {5, 5, 0, 0, 0}, {6, 5, 6, 4, 4},
  {5, 5, 7, 5, 5}, {5, 7, 7, 5, 5}, {5, 5, 5, 5, 7},   /* H M U (v7.3) */
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

/* Gewoelbte Scheibe: Licht von links oben (hell, mittel, dunkel) */
static void band_shade(uint8_t *buf, int16_t cx, int16_t cy, int16_t r, uint8_t hi, uint8_t mid, uint8_t dk) {
  for (int16_t yy = cy - r; yy <= cy + r; yy++) {
    if (yy < band_y0 || yy >= band_y1) continue;
    int16_t dy = yy - cy;
    for (int16_t xx = cx - r; xx <= cx + r; xx++) {
      int16_t dx = xx - cx;
      if (dx * dx + dy * dy > r * r + r / 2) continue;
      int16_t l = -dx - dy;
      buf[((uint16_t)(xx >> 3) << 6) + ((yy - band_y0) << 3) + (xx & 7)] = l > r / 2 ? hi : l < -r / 2 ? dk : mid;
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
    /* v7.3: Schatten, leuchtender Kragen (Lampe), Chromring, gewoelbte Kappe mit Glanzpunkt */
    static const char *const BNAME[N_BUMPERS] = { "PEEK", "POKE", "SYS" };
    band_disc(b, x + 2, y + 3, 14, C_SHADOW);
    band_disc(b, x, y, 13, (uint8_t)(C_BUMP0 + i));
    band_shade(b, x, y, 10, C_CHROME_HI, C_CHROME_MID, C_CHROME_DK);
    band_shade(b, x, y, 8, C_CAP_HI, C_CAP, C_CAP_DK);
    band_rect(b, x - 4, y - 6, 2, 2, C_BALL_HI);
    int16_t n = 0;
    while (BNAME[i][n]) n++;
    band_text(b, x - n * 2 + 1, y - 2, BNAME[i], 1, C_CAP_TEXT);
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
  /* v7.3: Roll-Over CIA 1 und CIA 2 */
  if (band_y0 < CIA_Y + 8 && band_y1 > CIA_Y - 8)
    for (uint8_t i = 0; i < 2; i++) {
      int16_t cx = i ? CIA_RX : CIA_LX;
      band_disc(b, cx, CIA_Y, ROLL_R, C_CHROME_MID);
      band_disc(b, cx, CIA_Y, 5, (uint8_t)(C_CIA0 + i));
      band_text(b, cx - 1, CIA_Y - 2, i ? "2" : "1", 1, C_BG);
    }
  /* v7.6: Bonus-Leiter: vier Pfeile zwischen zwei Chromholmen, rechts 2X-5X, unten BONUS */
  if (band_y0 < LADDER_Y + 34 && band_y1 > LADDER_Y - 72) {
    for (int16_t x = LADDER_X - 19; x <= LADDER_X + 19; x += 38) {
      band_box(b, x, LADDER_Y - 72, x, LADDER_Y + 18, C_CHROME_MID, C_CHROME_MID);
      band_box(b, x + 1, LADDER_Y - 72, x + 1, LADDER_Y + 18, C_CHROME_HI, C_CHROME_HI);
    }
    static const char *const ML[4] = { "2X", "3X", "4X", "5X" };
    for (uint8_t k = 0; k < 4; k++) {
      int16_t y = LADDER_Y - 22 * k;
      uint8_t col = (uint8_t)(C_MULT0 + k);
      for (int16_t dx = -16; dx <= 16; dx++) {
        int16_t a = (dx < 0 ? -dx : dx) * 12 / 17, t = y - 4 + a;
        for (int16_t yy = t; yy <= t + 8; yy++) {
          uint8_t edge = yy == t || yy == t + 8 || dx == -16 || dx == 16;
          band_put_bg(b, LADDER_X + dx + 1, yy + 1, C_PCB_DK);           /* Schatten */
          band_put_bg(b, LADDER_X + dx, yy, edge ? C_CHIP_EDGE : col);
        }
      }
      band_box(b, LADDER_X + 22, y + 1, LADDER_X + 37, y + 11, C_PCB_DK, C_PCB_DK);
      band_text5(b, LADDER_X + 24, y + 3, ML[k], col);
    }
    band_box(b, LADDER_X - 16, LADDER_Y + 20, LADDER_X + 16, LADDER_Y + 30, C_PCB_DK, C_PCB_DK);
    band_text5(b, LADDER_X - 14, LADDER_Y + 22, "BONUS", C_KEY_TEXT);
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
/* v8.2: Drahtrampe wie beim echten Flipper: vier Metallschienen (zwei obere aussen
 * bei +-7 Pixel, zwei untere, auf denen die Kugel laeuft, bei +-3), alle 20 Pixel
 * eine Querstrebe, dazwischen durchsichtig. Im Hintergrundbild (shadow = 1) werfen
 * die Schienen zusaetzlich einen Schatten auf die Spielflaeche. */
static void ramp_px(uint8_t *buf, int16_t xx, int16_t yy, uint8_t col, uint8_t weak) {
  if (yy < band_y0 || yy >= band_y1 || xx < 0 || xx >= TW) return;
  uint8_t *p = &buf[((uint16_t)(xx >> 3) << 6) + ((yy - band_y0) << 3) + (xx & 7)];
  if (weak && (*p == C_RAMP_RAIL || *p == C_RAMP_RAIL_DK)) return;   /* Schienen bleiben vorn */
  *p = col;
}
static void ramp_stamp(uint8_t *buf, uint8_t shadow) {
  if (band_y1 <= RAMP_Y_MIN || band_y0 > RAMP_Y_MAX + 4) return;
  for (uint8_t pass = shadow ? 0 : 1; pass < 2; pass++) {
  uint8_t seg = 0, k = 0;
  for (fix16 s = 0; s <= RAMP_LEN; s += 32768L, k = (uint8_t)(k == 39 ? 0 : k + 1)) {
    while (seg + 1 < RAMP_NSEG && s >= RAMP_SEGS[seg + 1].s0) seg++;
    const ramp_seg_t *g = &RAMP_SEGS[seg];
    fix16 d = s - g->s0;
    fix16 y = RS_P(g->y0) + fmul(RS_U(g->uy), d);
    int16_t yi = (int16_t)(y >> 16);
    if (yi < band_y0 - 12 || yi > band_y1 + 8) continue;
    fix16 x = RS_P(g->x0) + fmul(RS_U(g->ux), d);
    for (int8_t o2 = -14; o2 <= 14; o2++) {
      fix16 o = (fix16)o2 << 15;
      int16_t xx = (int16_t)((x - fmul(RS_U(g->uy), o)) >> 16), yy = (int16_t)((y + fmul(RS_U(g->ux), o)) >> 16);
      uint8_t ao = (uint8_t)(o2 < 0 ? -o2 : o2);
      uint8_t rail = ao >= 13 || ao == 6 || ao == 5;
      uint8_t brace = k < 2 && ao <= 13;
      if (!rail && !brace) continue;
      if (pass == 0) { ramp_px(buf, xx + 2, yy + 3, C_SHADOW, 1); continue; }
      if (rail) ramp_px(buf, xx, yy, (ao == 14 || ao == 6) ? C_RAMP_RAIL : C_RAMP_RAIL_DK, 0);
      else ramp_px(buf, xx, yy, C_RAMP_RAIL_DK, 1);
    }
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
    ramp_stamp(b, 0);
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
    ramp_stamp(b, 1);
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
