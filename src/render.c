/*
 * render.c – Scrolling und Zeichnen in spaltenweise organisierte Bildpuffer
 *
 * Jede Bildschirmspalte (8 Pixel breit, 168 Pixel hoch) ist ein
 * zusammenhaengender Block von 1344 Byte. Ablauf pro Bild:
 *  1. Hat sich die Kamera seit dem letzten Bild in diesem Puffer bewegt,
 *     werden alle 40 Spalten per DMA aus dem Hintergrund im Attic-RAM
 *     kopiert (je ein DMA-Auftrag). Sonst werden nur die Stellen erneuert,
 *     an denen vorher Flipper oder Ball gezeichnet waren.
 *  2. Fuer jede Spalte, die Flipper oder Ball beruehren, wird der betroffene
 *     Ausschnitt in COLBUF geholt, bemalt und zurueckgeschrieben.
 */
#include "render.h"
#include "hw.h"
#include "table.h"
#include "physics.h"

/* ---------- Flipper: vorberechnete Spannen pro Winkelphase ---------- */
#define NPH   25            /* Phasen im Abstand von 2 Winkeleinheiten */
#define SROWS 48            /* dy = -24 .. 23 */
#define EMPTY 127
static int8_t span0[NPH][SROWS], span1[NPH][SROWS];
static int8_t ph_row0[NPH], ph_row1[NPH];

static void spans_init(void) {
  for (uint8_t p = 0; p < NPH; p++) {
    uint8_t d = (uint8_t)(F_UP + p * 2);
    fix16 ux = cosA(d), uy = sinA(d);
    ph_row0[p] = EMPTY; ph_row1[p] = -128;
    for (uint8_t j = 0; j < SROWS; j++) {
      int8_t x0 = EMPTY, x1 = -128;
      fix16 ey = ((fix16)(j - 24) << 16) + 32768L;
      for (int8_t dx = -8; dx <= 44; dx++) {
        fix16 ex = ((fix16)dx << 16) + 32768L;
        fix16 t = fmul(ex, ux) + fmul(ey, uy);
        if (t < 0) t = 0; else if (t > ((fix16)F_LEN << 16)) t = (fix16)F_LEN << 16;
        int32_t ddx = (ex - fmul(ux, t)) >> 8, ddy = (ey - fmul(uy, t)) >> 8;
        int32_t r8 = (((fix16)F_R0 << 16) + fmul(t, -5461L)) >> 8;
        if (mul32(ddx, ddx) + mul32(ddy, ddy) <= mul32(r8, r8)) {
          if (dx < x0) x0 = dx;
          if (dx > x1) x1 = dx;
        }
      }
      span0[p][j] = x0; span1[p][j] = x1;
      if (x0 != EMPTY) {
        if ((int8_t)j < ph_row0[p]) ph_row0[p] = (int8_t)j;
        if ((int8_t)j > ph_row1[p]) ph_row1[p] = (int8_t)j;
      }
    }
  }
}

static uint8_t phase_of(const flipper_t *f) {
  int16_t v = (f->ang - F_UP * 256 + 256) >> 9;
  if (v < 0) v = 0; else if (v >= NPH) v = NPH - 1;
  return (uint8_t)v;
}

static uint8_t in_span(uint8_t p, int8_t j, int8_t xr) {
  if (j < 0 || j >= SROWS) return 0;
  return span0[p][j] != EMPTY && xr >= span0[p][j] && xr <= span1[p][j];
}

/* ---------- Ball ---------- */
static uint8_t ball_img[11][11];

static void ball_init(void) {
  for (int8_t j = 0; j < 11; j++)
    for (int8_t i = 0; i < 11; i++) {
      int8_t dx = i - 5, dy = j - 5;
      int8_t d2 = dx * dx + dy * dy;
      int8_t h2 = (dx + 2) * (dx + 2) + (dy + 2) * (dy + 2);
      uint8_t c = 0;
      if (d2 <= 28) c = h2 <= 3 ? C_BALL_HI : (d2 <= 13 ? C_BALL_MID : C_BALL_DK);
      ball_img[j][i] = c;
    }
}

/* ---------- Zeichnen in eine Spalte ---------- */
/* Aktuelle Spalte: x-Bereich cx0..cx0+7, Tischzeilen ty0..ty1 in COLBUF */
static int16_t cx0, ty0, ty1;

static inline void col_px(int16_t x, int16_t y, uint8_t c) {
  COLBUF[((uint16_t)(y - ty0) << 3) + (uint8_t)(x - cx0)] = c;
}

static void draw_flipper_col(const flipper_t *f) {
  uint8_t p = phase_of(f);
  int16_t yb = f->py - 24;
  int16_t ya = yb + ph_row0[p], ye = yb + ph_row1[p];
  if (ya < ty0) ya = ty0;
  if (ye > ty1) ye = ty1;
  for (int16_t y = ya; y <= ye; y++) {
    int8_t j = (int8_t)(y - yb);
    if (span0[p][j] == EMPTY) continue;
    int8_t a = span0[p][j], b = span1[p][j];
    /* x-Bereich der Spanne, auf die Spalte beschnitten */
    int16_t xa = f->side > 0 ? f->px + a : f->px - b;
    int16_t xe = f->side > 0 ? f->px + b : f->px - a;
    if (xa < cx0) xa = cx0;
    if (xe > cx0 + 7) xe = cx0 + 7;
    for (int16_t x = xa; x <= xe; x++) {
      int8_t xr = (int8_t)(f->side > 0 ? x - f->px : f->px - x);
      uint8_t edge = xr == a || xr == b || !in_span(p, j - 1, xr) || !in_span(p, j + 1, xr);
      uint8_t c = edge ? C_FLIP_EDGE : C_FLIP;
      int16_t ddx = x - f->px, ddy = y - f->py;
      if (ddx >= -1 && ddx <= 1 && ddy >= -1 && ddy <= 1) c = C_CHROME_MID;
      col_px(x, y, c);
    }
  }
}

static void draw_ball_col(int16_t bx, int16_t by) {
  for (int8_t j = 0; j < 11; j++) {
    int16_t y = by - 5 + j;
    if (y < ty0 || y > ty1) continue;
    for (int8_t i = 0; i < 11; i++) {
      int16_t x = bx - 5 + i;
      if (x < cx0 || x > cx0 + 7) continue;
      uint8_t c = ball_img[j][i];
      if (c) col_px(x, y, c);
    }
  }
}

/* ---------- Bild zusammensetzen ---------- */
#define NONE 0xFF
static int16_t buf_cam[2];
static uint8_t dirty_r0[2][40], dirty_r1[2][40];   /* Zeichenzeilen 0..20, r0 = NONE: leer */
static uint8_t need_r0[40], need_r1[40];

/* Rechteck (Tischkoordinaten) als benoetigte Zeichenzeilen je Spalte vormerken */
static void need_rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t cam) {
  int16_t s0 = y0 - cam, s1 = y1 - cam;
  if (s1 < 0 || s0 > VIEW_H - 1 || x1 < 0 || x0 > TW - 1) return;
  if (s0 < 0) s0 = 0;
  if (s1 > VIEW_H - 1) s1 = VIEW_H - 1;
  if (x0 < 0) x0 = 0;
  if (x1 > TW - 1) x1 = TW - 1;
  uint8_t r0 = (uint8_t)(s0 >> 3), r1 = (uint8_t)(s1 >> 3);
  for (uint8_t c = (uint8_t)(x0 >> 3); c <= (uint8_t)(x1 >> 3); c++) {
    if (need_r0[c] == NONE || r0 < need_r0[c]) need_r0[c] = r0;
    if (need_r1[c] == NONE || r1 > need_r1[c]) need_r1[c] = r1;
  }
}

void render_init(void) {
  spans_init();
  ball_init();
  buf_cam[0] = buf_cam[1] = -1;
  for (uint8_t c = 0; c < 40; c++) dirty_r0[0][c] = dirty_r0[1][c] = NONE;
}

int16_t camera_update(void) {
  static int16_t cam16 = (TH - VIEW_H) * 16;
  int16_t target = (int16_t)(ball.y >> 16) - 100;
  if (target < 0) target = 0;
  if (target > TH - VIEW_H) target = TH - VIEW_H;
  int16_t diff = target * 16 - cam16;
  if (diff > -4 && diff < 4) cam16 = target * 16;   /* Rest einrasten, sonst bleibt 1 Pixel stehen */
  else cam16 += diff >> 2;
  return cam16 >> 4;
}

void render_frame(uint8_t b, int16_t cam) {
  uint32_t fb = b ? FB_B : FB_A;
  uint32_t bg = ATTIC_BG + (uint32_t)(uint16_t)cam * 8;

  /* 1. Hintergrund */
  if (cam != buf_cam[b]) {
    for (uint8_t c = 0; c < 40; c++)
      dma_copy(bg + c * STRIP_BYTES, fb + (uint32_t)c * COL_BYTES, COL_BYTES);
    buf_cam[b] = cam;
  } else {
    for (uint8_t c = 0; c < 40; c++) {
      uint8_t r0 = dirty_r0[b][c];
      if (r0 == NONE) continue;
      uint16_t off = (uint16_t)r0 * 64, len = (uint16_t)(dirty_r1[b][c] - r0 + 1) * 64;
      dma_copy(bg + c * STRIP_BYTES + off, fb + (uint32_t)c * COL_BYTES + off, len);
    }
  }

  /* 2. Was muss gezeichnet werden? */
  for (uint8_t c = 0; c < 40; c++) need_r0[c] = need_r1[c] = NONE;
  for (uint8_t i = 0; i < 2; i++) {
    const flipper_t *f = &flip[i];
    uint8_t p = phase_of(f);
    int16_t yb = f->py - 24;
    int16_t xa = f->side > 0 ? f->px - 8 : f->px - 44;
    need_rect(xa, yb + ph_row0[p], xa + 52, yb + ph_row1[p], cam);
  }
  int16_t bx = (int16_t)(ball.x >> 16), by = (int16_t)(ball.y >> 16);
  uint8_t ball_on = state != ST_DRAIN;
  if (ball_on) need_rect(bx - 5, by - 5, bx + 5, by + 5, cam);

  /* 3. Spaltenweise holen, bemalen, zurueckschreiben */
  for (uint8_t c = 0; c < 40; c++) {
    uint8_t r0 = need_r0[c], r1 = need_r1[c];
    dirty_r0[b][c] = r0;
    dirty_r1[b][c] = r1;
    if (r0 == NONE) continue;
    uint16_t off = (uint16_t)r0 * 64, len = (uint16_t)(r1 - r0 + 1) * 64;
    uint32_t addr = fb + (uint32_t)c * COL_BYTES + off;
    dma_copy(addr, CPUADDR(COLBUF), len);
    cx0 = (int16_t)c * 8;
    ty0 = cam + (int16_t)r0 * 8;
    ty1 = cam + (int16_t)r1 * 8 + 7;
    draw_flipper_col(&flip[0]);
    draw_flipper_col(&flip[1]);
    if (ball_on) draw_ball_col(bx, by);
    dma_copy(CPUADDR(COLBUF), addr, len);
  }
}
