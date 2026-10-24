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

int16_t win_y0 = -1000;
uint8_t cls_grid[(TH / 8) * 40];

const int16_t BUMPERS[N_BUMPERS][3] = { {124, 140, 13}, {196, 140, 13}, {160, 188, 13} };

/* Fenster ab Tischzeile y0 aus dem Farb-RAM laden */
void mask_load(int16_t y0) {
  if (y0 > TH - WIN_ROWS) y0 = TH - WIN_ROWS;
  if (y0 < 0) y0 = 0;
  win_y0 = y0;
  dma_copy(MASK_FAR + (uint32_t)(uint16_t)y0 * 40, CPUADDR(MASKWIN), WIN_ROWS * 40);
}

/* Wandsegmente: x0, y0, x1, y1, Klasse */
static const int16_t SEGS[][5] = {
  {10, 156, 10, 640, M_WALL},
  {310, 156, 310, 640, M_WALL},
  {290, 138, 290, 640, M_WALL},           /* Innenwand Abschussbahn */
  {114, 34, 114, 70, M_WALL}, {138, 34, 138, 70, M_WALL},      /* Lanes C 6 4 */
  {162, 34, 162, 70, M_WALL}, {186, 34, 186, 70, M_WALL},
  {36, 258, 36, 420, M_WALL},             /* Fuehrung linker Orbit */
  {40, 444, 40, 519, M_WALL}, {40, 519, 101, 557, M_WALL},     /* Inlane links */
  {260, 444, 260, 519, M_WALL}, {260, 519, 199, 557, M_WALL},  /* Inlane rechts */
  {100, 567, 72, 640, M_WALL},
  {200, 567, 228, 640, M_WALL},
  {64, 462, 64, 504, M_WALL}, {64, 504, 92, 524, M_WALL},      /* Slingshot links */
  {236, 462, 236, 504, M_WALL}, {236, 504, 208, 524, M_WALL},  /* Slingshot rechts */
  {64, 462, 92, 524, M_SLING_L},
  {236, 462, 208, 524, M_SLING_R},
  {290, 90, 290, 138, M_GATE},            /* Einweg-Tor */
  {12, 236, 34, 256, M_GATE_UP},          /* Einwegtor linker Orbit */
};
#define NSEGS (sizeof(SEGS) / sizeof(SEGS[0]))

static const int16_t SLING_TRI[2][6] = {
  {64, 462, 64, 504, 92, 524},
  {236, 462, 236, 504, 208, 524},
};
/*
 * Targets als kurze Wandstuecke (Breite 5 wie Waende): x0, y0, x1, y1.
 * Jede Gruppe bildet eine lueckenlose, schraege Linie. Waagerechte Flaechen
 * und Luecken in Ballbreite waeren Ruheplaetze, weil die Schwerkraft bei uns
 * senkrecht wirkt (im PC-Test blieb der Ball zwischen zwei Targets liegen).
 */
static const int16_t TARGETS[N_TARGETS][4] = {
  {88, 228, 96, 242}, {97, 244, 106, 259}, {107, 261, 116, 276},     /* VIC-II */
  {112, 306, 126, 302}, {128, 301, 142, 297}, {144, 296, 158, 292},  /* 6510 */
  {160, 291, 174, 287},
  {280, 328, 280, 344}, {280, 356, 280, 372}, {280, 384, 280, 400},  /* SID */
};

/* Beim Aufbau: Band, in das gerade gezeichnet wird */
static int16_t band_y0, band_y1;

static void set_px(int16_t x, int16_t y, uint8_t c) {
  if (x < 0 || x >= TW || y < band_y0 || y >= band_y1) return;
  uint16_t yy = (uint16_t)(y - band_y0);
  MASKWIN[(yy << 5) + (yy << 3) + ((uint16_t)x >> 3)] |= (uint8_t)(0x80 >> (x & 7));
  uint16_t r = (uint16_t)y >> 3;
  uint8_t *g = &cls_grid[(r << 5) + (r << 3) + ((uint16_t)x >> 3)];
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

/* Zeichnet alle Formen, beschnitten auf das aktuelle Band */
static void build_band(void) {
  /* Oberer Bogen: Mittelpunkt (160,156), Radius 150, Breite 4 */
  for (int16_t y = band_y0 < 4 ? 4 : band_y0; y <= 156 && y < band_y1; y++) {
    uint16_t sy = SQ[156 - y];
    for (int16_t x = 8; x <= 312; x++) {
      int16_t ax = x - 160; if (ax < 0) ax = -ax;
      if (ax > 152) continue;
      uint16_t d2 = sy + SQ[ax];
      if (d2 >= 148u * 148u && d2 <= 152u * 152u) set_px(x, y, M_WALL);
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
    for (int16_t y = 460; y <= 526; y++) {
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
  for (uint16_t i = 0; i < (TH / 8) * 40; i++) cls_grid[i] = 0;
  for (uint16_t i = 0; i <= 160; i++) SQ[i] = i * i;
  for (band_y0 = 0; band_y0 < TH; band_y0 += WIN_ROWS) {
    band_y1 = band_y0 + WIN_ROWS;
    if (band_y1 > TH) band_y1 = TH;
    for (uint16_t i = 0; i < WIN_ROWS * 40; i++) MASKWIN[i] = 0;
    build_band();
    dma_copy(CPUADDR(MASKWIN), MASK_FAR + (uint32_t)(uint16_t)band_y0 * 40,
             (uint16_t)(band_y1 - band_y0) * 40);
    VICIV.bordercol++;                  /* Fortschritt */
  }
  win_y0 = -1000;                       /* Fenster ungueltig, naechster Zugriff laedt neu */
}

void table_build_palette(void) {
  set_color(0, 0, 0, 0);
  set_color(C_BG, 16, 12, 56);
  set_color(C_GRID, 26, 21, 74);
  set_color(C_CHROME_DK, 100, 96, 150);
  set_color(C_CHROME_MID, 160, 156, 205);
  set_color(C_CHROME_HI, 236, 235, 255);
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
  set_color(C_C64_DARK, 64, 49, 141);       /* Einschaltbild: Hintergrund */
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
}

/* ---------- Platzhaltergrafik ---------- */

/* Abstand eines Punktes zur Gummikante eines Slingshots <= 2.5 Pixel? */
static uint8_t on_rubber(const int16_t *t, int16_t x, int16_t y) {
  int32_t ex = t[4] - t[0], ey = t[5] - t[1];
  int32_t px = x - t[0], py = y - t[1];
  int32_t dot = px * ex + py * ey, len2 = ex * ex + ey * ey;
  if (dot < 0 || dot > len2) return 0;
  int32_t cr = px * ey - py * ex;
  return cr * cr * 4 <= len2 * 25;
}

static uint8_t art_px(int16_t x, int16_t y) {
  if (mask_solid(x, y)) {
    if (y >= 458 && y <= 528) {
      if (x <= 96 && on_rubber(SLING_TRI[0], x, y)) return C_RUBBER;
      if (x >= 204 && on_rubber(SLING_TRI[1], x, y)) return C_RUBBER;
    }
    uint8_t k = mask_class(x, y);
    if (k >= M_TARGET + TGT_SID) return C_TARGET_SID;
    if (k >= M_TARGET + TGT_6510) return C_TARGET_6510;
    if (k >= M_TARGET) return C_TARGET_VIC;
    if (y > 0 && !mask_solid(x, y - 1)) return C_CHROME_HI;
    if (x > 0 && !mask_solid(x - 1, y)) return C_CHROME_MID;
    return C_CHROME_DK;
  }
  if (y >= 460 && y <= 526) {
    if (x >= 62 && x <= 94 && in_tri(SLING_TRI[0], x, y)) return C_SLING_BODY;
    if (x >= 206 && x <= 238 && in_tri(SLING_TRI[1], x, y)) return C_SLING_BODY;
  }
  if (x >= 293 && x <= 307 && y >= 100) return C_LANE;
  if (y >= 567) {                        /* Apron zwischen den schraegen Blechen */
    int16_t d = y - 567;
    if (x > 100 - (d * 28) / 73 && x < 200 + (d * 28) / 73) return C_LANE;
  }
  /* Leiterbahnen als Grundmuster: senkrechte und waagerechte Bahnen */
  if (((x + 4) & 31) == 0 && ((y >> 5) & 1)) return C_TRACE;
  if (((y + 12) & 31) == 0 && ((x >> 6) & 1)) return C_TRACE;
  if ((x & 15) == 0 || (y & 15) == 0) return C_GRID;
  return C_BG;
}

/* Kleine 3x5-Schrift fuer Beschriftungen auf dem Tisch */
static const char LBL_CHARS[] = "0123456789ABCDEFIKLNORSTVY-.*,\"";
static const uint8_t LBL_GLYPHS[][5] = {
  {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
  {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 2, 2}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
  {2, 5, 7, 5, 5}, {6, 5, 6, 5, 6}, {3, 4, 4, 4, 3}, {6, 5, 5, 5, 6}, {7, 4, 6, 4, 7},
  {7, 4, 6, 4, 4}, {7, 2, 2, 2, 7}, {5, 5, 6, 5, 5}, {4, 4, 4, 4, 7}, {6, 5, 5, 5, 5},
  {2, 5, 5, 5, 2}, {6, 5, 6, 5, 5}, {3, 4, 2, 1, 6}, {7, 2, 2, 2, 2}, {5, 5, 5, 5, 2},
  {5, 5, 2, 2, 2}, {0, 0, 7, 0, 0}, {0, 0, 0, 0, 2}, {0, 5, 2, 5, 0}, {0, 0, 0, 2, 4},
  {5, 5, 0, 0, 0},
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

static void band_rect(uint8_t *buf, int16_t x0, int16_t y0, int16_t w, int16_t h, uint8_t col) {
  for (int16_t yy = y0; yy < y0 + h; yy++) {
    if (yy < band_y0 || yy >= band_y1) continue;
    for (int16_t xx = x0; xx < x0 + w; xx++)
      buf[((uint16_t)(xx >> 3) << 6) + ((yy - band_y0) << 3) + (xx & 7)] = col;
  }
}

static void band_overlays(uint8_t *b) {
  /* Pop-Bumper: roter Ring, Kappe im Einschaltbild-Blau mit Cursor */
  for (uint8_t i = 0; i < N_BUMPERS; i++) {
    int16_t x = BUMPERS[i][0], y = BUMPERS[i][1];
    if (y + 14 < band_y0 || y - 14 >= band_y1) continue;
    band_disc(b, x, y, 13, C_BUMPER_RING);
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
  /* Kick-Out-Hole "1541" */
  if (band_y0 < 254 && band_y1 > 220) {
    band_disc(b, 226, 232, 10, C_HOLE_RING);
    band_disc(b, 226, 232, 8, C_HOLE);
    band_text(b, 219, 246, "1541", 1, C_LABEL);
  }
  if (band_y0 < 290 && band_y1 > 280) band_text(b, 96, 282, "VIC-II", 1, C_TARGET_VIC);
  if (band_y0 < 318 && band_y1 > 308) band_text(b, 136, 310, "6510", 1, C_TARGET_6510);
  if (band_y0 < 420 && band_y1 > 408) band_text(b, 272, 410, "SID", 1, C_TARGET_SID);
  /* Lampen L O A D */
  if (band_y0 < 470 && band_y1 > 450) {
    static const int16_t LX[4] = { 25, 52, 248, 275 };
    static const char *LL[4] = { "L", "O", "A", "D" };
    for (uint8_t i = 0; i < 4; i++) {
      band_disc(b, LX[i], 460, 5, C_LAMP);
      band_text(b, LX[i] - 1, 458, LL[i], 1, C_BG);
    }
  }
  /* Kickback "FIRE" in der linken Outlane */
  if (band_y0 < 604 && band_y1 > 588) { band_rect(b, 18, 590, 14, 10, C_KICKBACK); }
  /* Apron: Einschaltbild */
  if (band_y0 < 632 && band_y1 > 586) {
    band_text(b, 119, 588, "READY.", 2, C_C64_LIGHT);
    band_rect(b, 167, 588, 6, 10, C_C64_LIGHT);
    band_text(b, 106, 606, "38911 BASIC BYTES FREE", 1, C_C64_LIGHT);
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
    /* Jede Zeichenspalte in ihren Streifen im Attic-RAM */
    uint32_t dst = ATTIC_BG + (uint32_t)(uint16_t)band_y0 * 8;
    for (uint8_t c = 0; c < 40; c++, dst += STRIP_BYTES)
      dma_copy(CPUADDR(b + (uint16_t)c * 64), dst, 64);
    VICIV.bordercol = (uint8_t)(band_y0 >> 3);
  }
  VICIV.bordercol = 0;
  win_y0 = -1000;
}
