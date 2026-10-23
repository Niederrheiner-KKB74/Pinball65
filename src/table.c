/*
 * table.c – Kollisionsmaske und Platzhaltergrafik, beim Start berechnet
 *
 * Wandstaerke wie im Prototyp (Strichbreite 4): Scheibe mit Radius 2.
 */
#include <mega65.h>
#include "table.h"
#include "hw.h"

uint8_t mask_win[WIN_ROWS * 40];
int16_t win_y0 = -1000;
uint8_t cls_grid[(TH / 8) * 40];

/* Fenster ab Tischzeile y0 aus dem Farb-RAM laden */
void mask_load(int16_t y0) {
  if (y0 > TH - WIN_ROWS) y0 = TH - WIN_ROWS;
  if (y0 < 0) y0 = 0;
  win_y0 = y0;
  dma_copy(MASK_FAR + (uint32_t)(uint16_t)y0 * 40, CPUADDR(mask_win), WIN_ROWS * 40);
}

/* Beim Aufbau: Band, in das gerade gezeichnet wird */
static int16_t band_y0, band_y1;

/* Wandsegmente: x0, y0, x1, y1, Klasse */
static const int16_t SEGS[][5] = {
  {10, 160, 10, 576, M_WALL},
  {310, 160, 310, 576, M_WALL},
  {290, 132, 290, 576, M_WALL},           /* Innenwand Abschussbahn */
  {102, 40, 102, 68, M_WALL}, {134, 40, 134, 68, M_WALL},
  {166, 40, 166, 68, M_WALL}, {198, 40, 198, 68, M_WALL},
  {40, 380, 40, 455, M_WALL}, {40, 455, 101, 493, M_WALL},     /* Inlane links */
  {260, 380, 260, 455, M_WALL}, {260, 455, 199, 493, M_WALL},  /* Inlane rechts */
  {100, 503, 72, 576, M_WALL},
  {200, 503, 228, 576, M_WALL},
  {64, 398, 64, 440, M_WALL}, {64, 440, 92, 460, M_WALL},      /* Slingshot links */
  {236, 398, 236, 440, M_WALL}, {236, 440, 208, 460, M_WALL},  /* Slingshot rechts */
  {64, 398, 92, 460, M_SLING_L},
  {236, 398, 208, 460, M_SLING_R},
  {290, 90, 290, 132, M_GATE},            /* Einweg-Tor */
};
#define NSEGS (sizeof(SEGS) / sizeof(SEGS[0]))

static const int16_t SLING_TRI[2][6] = {
  {64, 398, 64, 440, 92, 460},
  {236, 398, 236, 440, 208, 460},
};
static const int16_t TARGETS[6][2] = {
  {13, 246}, {13, 274}, {13, 302}, {281, 246}, {281, 274}, {281, 302},
};
#define TGT_W 6
#define TGT_H 18

static void set_px(int16_t x, int16_t y, uint8_t c) {
  if (x < 0 || x >= TW || y < band_y0 || y >= band_y1) return;
  uint16_t yy = (uint16_t)(y - band_y0);
  mask_win[(yy << 5) + (yy << 3) + ((uint16_t)x >> 3)] |= (uint8_t)(0x80 >> (x & 7));
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
  /* Oberer Bogen: Mittelpunkt (160,160), Radius 150, Breite 4 */
  for (int16_t y = band_y0 < 8 ? 8 : band_y0; y <= 160 && y < band_y1; y++) {
    uint16_t sy = SQ[160 - y];
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
  /* Slingshot-Koerper fuellen */
  for (uint8_t s = 0; s < 2; s++) {
    const int16_t *t = SLING_TRI[s];
    for (int16_t y = 396; y <= 462; y++) {
      if (y < band_y0 || y >= band_y1) continue;
      for (int16_t x = (s ? 206 : 62); x <= (s ? 238 : 94); x++)
        if (in_tri(t, x, y)) set_px(x, y, M_WALL);
    }
  }
  for (uint8_t i = 0; i < 6; i++)
    for (int16_t y = TARGETS[i][1]; y < TARGETS[i][1] + TGT_H; y++)
      for (int16_t x = TARGETS[i][0]; x < TARGETS[i][0] + TGT_W; x++)
        set_px(x, y, (uint8_t)(M_TARGET + i));
}

void table_build_mask(void) {
  for (uint16_t i = 0; i < (TH / 8) * 40; i++) cls_grid[i] = 0;
  for (uint16_t i = 0; i <= 160; i++) SQ[i] = i * i;
  for (band_y0 = 0; band_y0 < TH; band_y0 += WIN_ROWS) {
    band_y1 = band_y0 + WIN_ROWS;
    if (band_y1 > TH) band_y1 = TH;
    for (uint16_t i = 0; i < WIN_ROWS * 40; i++) mask_win[i] = 0;
    build_band();
    dma_copy(CPUADDR(mask_win), MASK_FAR + (uint32_t)(uint16_t)band_y0 * 40,
             (uint16_t)(band_y1 - band_y0) * 40);
    VICIV.bordercol++;                  /* Fortschritt */
  }
  win_y0 = -1000;                       /* Fenster ungueltig, naechster Zugriff laedt neu */
}

void table_build_palette(void) {
  set_color(0, 0, 0, 0);
  set_color(C_BG, 16, 12, 56);
  set_color(C_GRID, 30, 24, 84);
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
  set_color(C_HUD_BG, 10, 8, 24);
  set_color(C_HUD_TXT, 255, 174, 61);
  set_color(C_IND, 255, 210, 58);
  set_color(C_TARGET, 90, 79, 168);
  set_color(C_HUD_DIM, 120, 105, 196);
}

/* Abstand eines Punktes zur Gummikante eines Slingshots <= 2.5 Pixel? */
static uint8_t on_rubber(const int16_t *t, int16_t x, int16_t y) {
  int32_t ex = t[4] - t[0], ey = t[5] - t[1];
  int32_t px = x - t[0], py = y - t[1];
  int32_t dot = px * ex + py * ey, len2 = ex * ex + ey * ey;
  if (dot < 0 || dot > len2) return 0;
  int32_t cr = px * ey - py * ex;
  return cr * cr * 4 <= len2 * 25;
}

static uint8_t art_px(int16_t x, int16_t y, int16_t apron_l, int16_t apron_r) {
  if (mask_solid(x, y)) {
    if (x >= 60 && x <= 96 && y >= 394 && y <= 464 && on_rubber(SLING_TRI[0], x, y)) return C_RUBBER;
    if (x >= 204 && x <= 240 && y >= 394 && y <= 464 && on_rubber(SLING_TRI[1], x, y)) return C_RUBBER;
    if (mask_class(x, y) >= M_TARGET) return C_TARGET;
    if (y > 0 && !mask_solid(x, y - 1)) return C_CHROME_HI;
    if (x > 0 && !mask_solid(x - 1, y)) return C_CHROME_MID;
    return C_CHROME_DK;
  }
  if (x >= 62 && x <= 94 && y >= 396 && y <= 462 && in_tri(SLING_TRI[0], x, y)) return C_SLING_BODY;
  if (x >= 206 && x <= 238 && y >= 396 && y <= 462 && in_tri(SLING_TRI[1], x, y)) return C_SLING_BODY;
  if (x >= 293 && x <= 307 && y >= 100) return C_LANE;
  if (y >= 503 && x > apron_l && x < apron_r) return C_LANE;
  if ((x & 15) == 0 || (y & 15) == 0) return C_GRID;
  return C_BG;
}

void table_build_art(uint8_t *rowbuf) {
  uint32_t dst = BG_ADDR;
  for (uint8_t r = 0; r < VIEW_ROWS; r++) {
    mask_load(VIEW_TOP + r * 8 - 1);
    for (uint8_t py = 0; py < 8; py++) {
      int16_t y = VIEW_TOP + r * 8 + py;
      /* Schraege Abschlussbleche: x-Grenzen fuer diese Zeile */
      int16_t d = y - 503;
      int16_t apron_l = 100 - (d * 28) / 73, apron_r = 200 + (d * 28) / 73;
      for (int16_t x = 0; x < TW; x++)
        rowbuf[((uint16_t)(x >> 3) << 6) + (py << 3) + (x & 7)] = art_px(x, y, apron_l, apron_r);
    }
    dma_copy(CPUADDR(rowbuf), dst, ROW_BYTES);
    dst += ROW_BYTES;
    VICIV.bordercol = r;
  }
  dma_copy(BG_ADDR, FB_A, VIEW_ROWS * ROW_BYTES);
  dma_copy(BG_ADDR, FB_B, VIEW_ROWS * ROW_BYTES);
  dma_fill(FB_A + (uint32_t)HUD_ROW * ROW_BYTES, C_HUD_BG, ROW_BYTES);
  dma_fill(FB_B + (uint32_t)HUD_ROW * ROW_BYTES, C_HUD_BG, ROW_BYTES);
  VICIV.bordercol = 0;
}
