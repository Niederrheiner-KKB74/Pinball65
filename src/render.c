/*
 * render.c – Software-Zeichnen in FCM-Zeichenzeilen
 *
 * Jede Zeichenzeile (40 Zeichen x 64 Byte = 2560 Byte) wird per DMA aus dem
 * Hintergrund in einen CPU-Puffer geholt, dort bemalt und per DMA in den
 * hinteren Bildpuffer geschrieben. Zeilen, die im vorletzten Bild bemalt
 * wurden, aber jetzt nicht mehr, werden direkt aus dem Hintergrund
 * wiederhergestellt.
 */
#include "render.h"
#include "hw.h"
#include "table.h"
#include "physics.h"

uint8_t rowbuf[ROW_BYTES];

#define PX(x, py) rowbuf[((uint16_t)((x) >> 3) << 6) + ((py) << 3) + ((x) & 7)]

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

/* liegt die relative x-Position in der Spanne von Zeile j? */
static uint8_t in_span(uint8_t p, int8_t j, int8_t xr) {
  if (j < 0 || j >= SROWS) return 0;
  return span0[p][j] != EMPTY && xr >= span0[p][j] && xr <= span1[p][j];
}

static void draw_flipper(const flipper_t *f, uint8_t row) {
  uint8_t p = phase_of(f);
  int16_t ty0 = VIEW_TOP + row * 8;
  for (uint8_t py = 0; py < 8; py++) {
    int8_t j = (int8_t)(ty0 + py - f->py + 24);
    if (j < 0 || j >= SROWS || span0[p][j] == EMPTY) continue;
    int8_t a = span0[p][j], b = span1[p][j];
    for (int8_t xr = a; xr <= b; xr++) {
      int16_t x = f->side > 0 ? f->px + xr : f->px - xr;
      uint8_t edge = xr == a || xr == b || !in_span(p, j - 1, xr) || !in_span(p, j + 1, xr);
      uint8_t c = edge ? C_FLIP_EDGE : C_FLIP;
      int16_t ddx = x - f->px, ddy = ty0 + py - f->py;
      if (ddx >= -1 && ddx <= 1 && ddy >= -1 && ddy <= 1) c = C_CHROME_MID;
      PX(x, py) = c;
    }
  }
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

static void draw_ball(int16_t bx, int16_t by, uint8_t row) {
  int16_t sy0 = by - VIEW_TOP - 5 - row * 8;   /* Bildzeile der obersten Ballzeile relativ zur Zeichenzeile */
  for (int8_t j = 0; j < 11; j++) {
    int16_t py = sy0 + j;
    if (py < 0 || py > 7) continue;
    for (int8_t i = 0; i < 11; i++) {
      uint8_t c = ball_img[j][i];
      int16_t x = bx - 5 + i;
      if (c && x >= 0 && x < TW) PX(x, py) = c;
    }
  }
}

static void draw_indicator(int16_t bx) {
  for (uint8_t py = 0; py < 5; py++)
    for (int16_t x = bx - py; x <= bx + py; x++)
      if (x >= 0 && x < TW) PX(x, py) = C_IND;
}

/* ---------- Statuszeile: 3x5-Schrift ---------- */
static const char GLYPH_CHARS[] = "0123456789ABCDEFGHIKLMNOPRSTUVWXY:->/.";
static const uint8_t GLYPHS[][5] = {
  {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
  {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 2, 2}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
  {2, 5, 7, 5, 5}, {6, 5, 6, 5, 6}, {3, 4, 4, 4, 3}, {6, 5, 5, 5, 6}, {7, 4, 6, 4, 7},
  {7, 4, 6, 4, 4}, {3, 4, 5, 5, 3}, {5, 5, 7, 5, 5}, {7, 2, 2, 2, 7}, {5, 5, 6, 5, 5},
  {4, 4, 4, 4, 7}, {5, 7, 7, 5, 5}, {6, 5, 5, 5, 5}, {2, 5, 5, 5, 2}, {6, 5, 6, 4, 4},
  {6, 5, 6, 5, 5}, {3, 4, 2, 1, 6}, {7, 2, 2, 2, 2}, {5, 5, 5, 5, 7}, {5, 5, 5, 5, 2},
  {5, 5, 7, 7, 5}, {5, 5, 2, 5, 5}, {5, 5, 2, 2, 2}, {0, 2, 0, 2, 0}, {0, 0, 7, 0, 0},
  {4, 2, 1, 2, 4}, {1, 1, 2, 4, 4},
  {0, 0, 0, 0, 2},
};

static int16_t hud_x;
static void hud_char(char ch, uint8_t col) {
  for (uint8_t g = 0; GLYPH_CHARS[g]; g++) {
    if (GLYPH_CHARS[g] != ch) continue;
    for (uint8_t r = 0; r < 5; r++)
      for (uint8_t c = 0; c < 3; c++)
        if (GLYPHS[g][r] & (4 >> c)) PX(hud_x + c, r + 1) = col;
    break;
  }
  hud_x += 4;
}
static void hud_str(const char *s, uint8_t col) { while (*s) hud_char(*s++, col); }
static void hud_num(uint32_t v, uint8_t digits, uint8_t col) {
  char buf[10];
  for (int8_t i = (int8_t)digits - 1; i >= 0; i--) { buf[i] = (char)('0' + v % 10); v /= 10; }
  for (uint8_t i = 0; i < digits; i++) hud_char(buf[i], col);
}

static void draw_hud(uint16_t frame, uint16_t load) {
  for (uint16_t i = 0; i < ROW_BYTES; i++) rowbuf[i] = C_HUD_BG;
  hud_x = 4;
  hud_str("M1.2 ", C_HUD_DIM);  /* Meilenstein und Version */
  hud_str("F", C_HUD_DIM); hud_num(frame, 5, C_HUD_TXT);
  hud_str(" L", C_HUD_DIM); hud_num(load, 3, C_HUD_TXT);
  hud_str(" X", C_HUD_DIM); hud_num((uint32_t)(ball.x >> 16) & 0x3FF, 3, C_HUD_TXT);
  hud_str(" Y", C_HUD_DIM); hud_num((uint32_t)(ball.y >> 16) & 0x3FF, 3, C_HUD_TXT);
  hud_str(" B", C_HUD_DIM); hud_num(bumper_hits, 3, C_HUD_TXT);
  hud_str(" S", C_HUD_DIM); hud_num(sling_hits, 3, C_HUD_TXT);
  if (tune_on) {
    hud_str("  >", C_IND);
    hud_str(TUNE_INFO[tune_sel].name, C_HUD_TXT);
    hud_str(" ", C_HUD_DIM);
    hud_num((uint32_t)tune[tune_sel], 6, C_IND);
    return;
  }
  hud_str("  TEST", C_HUD_DIM);
  if (test_active) { hud_str(" ", C_HUD_DIM); hud_num(test_count, 3, C_HUD_TXT); }
  else if (snap_valid) {
    hud_str(" X:", C_HUD_DIM); hud_num((uint32_t)snap_x, 8, C_HUD_TXT);
    hud_str(" Y:", C_HUD_DIM); hud_num((uint32_t)snap_y, 8, C_HUD_TXT);
  } else hud_str(" -", C_HUD_DIM);
}

/* ---------- Bild zusammensetzen ---------- */
static uint32_t dirty[2];

static void mark_rows(uint32_t *m, int16_t sy0, int16_t sy1) {
  if (sy1 < 0 || sy0 > VIEW_ROWS * 8 - 1) return;
  if (sy0 < 0) sy0 = 0;
  if (sy1 > VIEW_ROWS * 8 - 1) sy1 = VIEW_ROWS * 8 - 1;
  for (int16_t r = sy0 >> 3; r <= (sy1 >> 3); r++) *m |= 1UL << r;
}

void render_init(void) {
  spans_init();
  ball_init();
  dirty[0] = dirty[1] = 0;
}

void render_frame(uint8_t b, uint16_t frame, uint16_t load) {
  uint32_t need = 0;
  for (uint8_t i = 0; i < 2; i++) {
    uint8_t p = phase_of(&flip[i]);
    int16_t y0 = flip[i].py - 24 - VIEW_TOP;
    mark_rows(&need, y0 + ph_row0[p], y0 + ph_row1[p]);
  }
  int16_t bx = (int16_t)(ball.x >> 16), by = (int16_t)(ball.y >> 16);
  uint8_t ball_on = state != ST_DRAIN;
  uint8_t ind = ball_on && by < VIEW_TOP + 3;
  if (ball_on) mark_rows(&need, by - VIEW_TOP - 5, by - VIEW_TOP + 5);
  if (ind) need |= 1;

  uint32_t fb = b ? FB_B : FB_A;
  uint32_t restore = dirty[b] & ~need;
  uint32_t bit = 1, off = 0;
  for (uint8_t r = 0; r < VIEW_ROWS; r++, bit <<= 1, off += ROW_BYTES) {
    if (restore & bit) {
      dma_copy(BG_ADDR + off, fb + off, ROW_BYTES);
    } else if (need & bit) {
      dma_copy(BG_ADDR + off, CPUADDR(rowbuf), ROW_BYTES);
      draw_flipper(&flip[0], r);
      draw_flipper(&flip[1], r);
      if (ball_on) draw_ball(bx, by, r);
      if (ind && r == 0) draw_indicator(bx);
      dma_copy(CPUADDR(rowbuf), fb + off, ROW_BYTES);
    }
  }
  dirty[b] = need;

  draw_hud(frame, load);
  dma_copy(CPUADDR(rowbuf), fb + (uint32_t)HUD_ROW * ROW_BYTES, ROW_BYTES);
}
