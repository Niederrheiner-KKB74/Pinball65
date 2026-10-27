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
  set_color(C_TARGET_PLA, 255, 150, 40);
  set_color(C_RAMP_RAIL, 220, 230, 255);    /* Rampe: Chromschienen ... */
  set_color(C_RAMP_RAIL_DK, 120, 130, 190);
  set_color(C_RAMP_FLOOR, 70, 150, 210);    /* ... und durchsichtiger Boden (Raster) */
  for (uint8_t k = 0; k < 4; k++) set_color((uint8_t)(C_SPIN0 + k), 90, 90, 110);
}


#ifdef PB_BUILDER
/* ===== Ab hier nur im Lader (PINBALL65): Aufbau von Maske und Grafik ===== */

/* Wandsegmente: x0, y0, x1, y1, Klasse */
static const int16_t SEGS[][5] = {
  {10, 156, 10, 640, M_WALL},
  {310, 156, 310, 640, M_WALL},
  {290, 138, 290, 640, M_WALL},           /* Innenwand Abschussbahn */
  {114, 34, 114, 70, M_WALL}, {138, 34, 138, 70, M_WALL},      /* Lanes C 6 4 */
  {162, 34, 162, 70, M_WALL}, {186, 34, 186, 70, M_WALL},
  {36, 258, 36, 400, M_WALL},             /* Fuehrung linker Orbit, endet mit den VIC-II-Targets (v5.2) */
  {10, 390, 36, 400, M_WALL},             /* v5.3: Out-Lane links oben zu (Ball flog ueber den Orbit zurueck) */
  {290, 390, 264, 400, M_WALL},           /* v5.3: Out-Lane rechts oben zu ... */
  {290, 318, 264, 336, M_WALL},           /* ... und der Gang hinter den SID-Targets auch oben */
  /* Inlanes (v4.2): Fuehrung endet knapp hinter dem Drehpunkt, sonst blieb der
   * Ball oben auf dem Drehpunkt liegen und der Flipper traf ihn kaum */
  /* v5.4: Trennwaende Inlane/Out-Lane 16 Pixel hoeher (vorher ab y 444): kleinerer Zugang */
  {40, 428, 40, 519, M_WALL}, {40, 519, 109, 560, M_WALL},     /* Inlane links */
  {260, 428, 260, 519, M_WALL}, {260, 519, 191, 560, M_WALL},  /* Inlane rechts */
  {100, 567, 72, 640, M_WALL},
  {200, 567, 228, 640, M_WALL},
  {64, 462, 64, 504, M_WALL}, {64, 504, 92, 524, M_WALL},      /* Slingshot links */
  {236, 462, 236, 504, M_WALL}, {236, 504, 208, 524, M_WALL},  /* Slingshot rechts */
  {64, 462, 92, 524, M_SLING_L},
  {236, 462, 208, 524, M_SLING_R},
  {290, 90, 290, 138, M_GATE},            /* Einweg-Tor */
  /* Einfahrt der Rampe (v5.4): senkrecht bei x 128, Muendung mit kleinem Trichter */
  {115, 300, 115, 330, M_WALL}, {141, 300, 141, 330, M_WALL},
  {115, 330, 106, 344, M_WALL}, {141, 330, 150, 344, M_WALL},
  {38, 316, 46, 326, M_WALL},             /* Dach ueber den VIC-II-Targets (wie bei SID) */
  {290, 268, 279, 294, M_WALL},           /* v4.3: Fuehrung ueber dem Drehpunkt des dritten Flippers,
                                             sonst blieb der Ball oben auf dem Drehpunkt liegen */
  /* Rechte Seite gespiegelt zur linken (v5.2): Begrenzung hinter den SID-Targets
   * bei x 264 (ueber der Trennwand Inlane/Out-Lane bei x 260), endet wie links bei
   * y 400; schraeges Dach von ihr zum obersten SID-Target */
  {264, 336, 264, 400, M_WALL},
  {264, 336, 254, 346, M_WALL},           /* v5.3 tiefer: sonst Mulde zwischen Spitze des dritten Flippers und Dach */
  {12, 236, 34, 256, M_WALL},             /* Orbit-Abweiser (v5.4 feste Wand: der Kanal darunter ist seit v5.3 geschlossen) */
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
  /* VIC-II (v4.2): links, genau gegenueber den SID-Targets */
  {46, 328, 46, 344}, {46, 356, 46, 372}, {46, 384, 46, 400},
  /* 6510 (v4.2): schraeg links vom geraden Abstieg der Rampe, zeigt zum linken Flipper */
  {214, 196, 220, 205}, {221, 206, 227, 215}, {227, 217, 233, 226}, {234, 227, 240, 236},
  {254, 348, 254, 362}, {254, 367, 254, 381}, {254, 386, 254, 400},  /* SID: gegenueber VIC-II */
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
    if (y >= 458 && y <= 528) {
      if (x <= 96 && on_rubber(SLING_TRI[0], x, y)) return C_SLING0;
      if (x >= 204 && on_rubber(SLING_TRI[1], x, y)) return C_SLING0 + 1;
    }
    uint8_t k = mask_class(x, y);
    if (k >= M_TARGET) return (uint8_t)(C_TGT0 + (k - M_TARGET));
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
  /* Kick-Out-Hole "1541" */
  if (band_y0 < KICK_Y + 20 && band_y1 > KICK_Y - 12) {
    band_disc(b, KICK_X, KICK_Y, 10, C_HOLE_RING);
    band_disc(b, KICK_X, KICK_Y, 8, C_HOLE);
    band_text(b, KICK_X - 7, KICK_Y + 13, "1541", 1, C_LABEL);
  }
  /* Spinner quer in der schraegen Einfahrt: vier Streifen, deren Farben wandern */
  if (band_y0 < SPIN_Y + 14 && band_y1 > SPIN_Y - 14) band_spinner(b);
  if (band_y0 < 412 && band_y1 > 402) band_text(b, 50, 404, "VIC-II", 1, C_TARGET_VIC);
  if (band_y0 < 222 && band_y1 > 212) band_text(b, 194, 214, "6510", 1, C_TARGET_6510);
  if (band_y0 < 412 && band_y1 > 402) band_text(b, 238, 404, "SID", 1, C_TARGET_SID);
  if (band_y0 < 184 && band_y1 > 174) band_text(b, 66, 176, "PLA", 1, C_TARGET_PLA);
  /* Lampen L O A D: O und A an den Inlanes, L und D sind Roll-Over-Kontakte (v5.1) */
  {
    static const int16_t LX[4] = { ROLL_LX, 52, 248, ROLL_DX }, LY[4] = { ROLL_Y, 460, 460, ROLL_Y };
    static const char *LL[4] = { "L", "O", "A", "D" };
    for (uint8_t i = 0; i < 4; i++) {
      if (band_y0 >= LY[i] + 6 || band_y1 <= LY[i] - 6) continue;
      if (i == 0 || i == 3) band_disc(b, LX[i], LY[i], ROLL_R, C_CHROME_MID);   /* Chromring */
      band_disc(b, LX[i], LY[i], 5, (uint8_t)(C_LOAD0 + i));
      band_text(b, LX[i] - 1, LY[i] - 2, LL[i], 1, C_BG);
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
    fix16 y = g->y0 + fmul(g->uy, d);
    int16_t yi = (int16_t)(y >> 16);
    if (yi < band_y0 - 8 || yi > band_y1 + 8) continue;
    fix16 x = g->x0 + fmul(g->ux, d);
    for (int8_t o2 = -14; o2 <= 14; o2++) {
      fix16 o = (fix16)o2 << 15;
      int16_t xx = (int16_t)((x - fmul(g->uy, o)) >> 16), yy = (int16_t)((y + fmul(g->ux, o)) >> 16);
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
