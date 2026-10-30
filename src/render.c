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
#include "game.h"

/* ---------- Flipper: vorberechnete Spannen pro Winkelphase ---------- */
#define NPH   25            /* Phasen im Abstand von 2 Winkeleinheiten */
/* Zeichnen mit der CPU (ohne transparente DMA): im Lader fuer die vorgezeichneten
 * Bilder und in den Tests; im Spiel seit v6.4 nicht mehr (spart rund 1 KB fuer den
 * Multiball). Ein Core ohne transparente DMA meldet beim Start einen Fehler. */
#if defined(PB_BUILDER) && !defined(CPU_DRAW)
#define CPU_DRAW
#endif
#define SROWS 48            /* dy = -24 .. 23 */
#define EMPTY 127
/* Je Phase 192 Byte im Attic-RAM: Spanne x0[48], x1[48] und Innenbereich
 * i0[48], i1[48] (alles dazwischen ist Flipperflaeche, der Rest Rand;
 * i0 > i1 = kein Innenbereich). Zum Zeichnen wird die benoetigte Phase in
 * einen von zwei kleinen Puffern geholt (je Flipper einer), das spart 4,4 KB. */
#define SPAN_BYTES (4 * SROWS)
#define SPAN_ADDR(p) (ATTIC_SPR + 0x38000UL + (uint16_t)(p) * SPAN_BYTES)
#ifdef CPU_DRAW
static int8_t ph_row0[NPH], ph_row1[NPH];
static int8_t spb[N_FLIP][SPAN_BYTES];
static uint8_t spb_ph[N_FLIP];

static const int8_t *span_get(uint8_t slot, uint8_t p) {
  if (spb_ph[slot] != p) {
    dma_copy(SPAN_ADDR(p), CPUADDR(spb[slot]), SPAN_BYTES);
    spb_ph[slot] = p;
  }
  return spb[slot];
}
#endif

#ifdef PB_BUILDER
static void spans_init(void) {
  int8_t *span0 = spb[0], *span1 = spb[0] + SROWS, *in0 = spb[0] + 2 * SROWS, *in1 = spb[0] + 3 * SROWS;
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
      span0[j] = x0; span1[j] = x1;
      if (x0 != EMPTY) {
        if ((int8_t)j < ph_row0[p]) ph_row0[p] = (int8_t)j;
        if ((int8_t)j > ph_row1[p]) ph_row1[p] = (int8_t)j;
      }
    }
    /* Innenbereich: innerhalb der eigenen Spanne ohne die Enden und
     * innerhalb der Spannen der Zeilen darueber und darunter */
    for (uint8_t j = 0; j < SROWS; j++) {
      int8_t a = EMPTY, b = -128;
      if (span0[j] != EMPTY && j > 0 && j < SROWS - 1 &&
          span0[j - 1] != EMPTY && span0[j + 1] != EMPTY) {
        a = span0[j] + 1; b = span1[j] - 1;
        if (span0[j - 1] > a) a = span0[j - 1];
        if (span0[j + 1] > a) a = span0[j + 1];
        if (span1[j - 1] < b) b = span1[j - 1];
        if (span1[j + 1] < b) b = span1[j + 1];
      }
      in0[j] = a; in1[j] = b;
    }
    dma_copy(CPUADDR(spb[0]), SPAN_ADDR(p), SPAN_BYTES);
  }
  /* Zeilenbereiche je Phase hinter den Spannen ablegen, das Spiel liest sie */
  dma_copy(CPUADDR(ph_row0), SPAN_ADDR(NPH), NPH);
  dma_copy(CPUADDR(ph_row1), SPAN_ADDR(NPH) + NPH, NPH);
#ifdef CPU_DRAW
  for (uint8_t i = 0; i < N_FLIP; i++) spb_ph[i] = 0xFF;
#endif
}

#endif /* PB_BUILDER */

static uint8_t phase_of(const flipper_t *f) {
  int16_t v = (f->ang - F_UP * 256 + 256) >> 9;
  if (v < 0) v = 0; else if (v >= NPH) v = NPH - 1;
  return (uint8_t)v;
}

/* ---------- Ball ---------- */
#ifdef CPU_DRAW
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

static void draw_flipper_ph(const flipper_t *f, uint8_t p) {
  int16_t yb = f->py - 24;
  int16_t ya = yb + ph_row0[p], ye = yb + ph_row1[p];
  if (ya < ty0) ya = ty0;
  if (ye > ty1) ye = ty1;
  if (ya > ye) return;
  /* x-Bereich der Spalte relativ zum Drehpunkt (in Flipperrichtung gemessen) */
  int16_t c0 = cx0 - f->px, c7 = c0 + 7;
  const int8_t *s0 = span_get((uint8_t)(f - flip), p), *s1 = s0 + SROWS, *i0 = s0 + 2 * SROWS, *i1 = s0 + 3 * SROWS;
  uint8_t *row = COLBUF + ((uint16_t)(ya - ty0) << 3);
  for (int16_t y = ya; y <= ye; y++, row += 8) {
    uint8_t j = (uint8_t)(y - yb);
    int8_t a = s0[j];
    if (a == EMPTY) continue;
    int8_t b = s1[j], ia = i0[j], ib = i1[j];
    if (f->side > 0) {                       /* linker Flipper: x = px + xr */
      int16_t lo = a > c0 ? a : c0, hi = b < c7 ? b : c7;
      for (int16_t xr = lo; xr <= hi; xr++)
        row[(uint8_t)(xr - c0)] = (xr >= ia && xr <= ib) ? C_FLIP : C_FLIP_EDGE;
    } else {                                 /* rechter Flipper: x = px - xr */
      int16_t lo = a > -c7 ? a : -c7, hi = b < -c0 ? b : -c0;
      for (int16_t xr = lo; xr <= hi; xr++)
        row[(uint8_t)(-xr - c0)] = (xr >= ia && xr <= ib) ? C_FLIP : C_FLIP_EDGE;
    }
    /* Drehpunkt: 3 x 3 Pixel */
    if (y >= f->py - 1 && y <= f->py + 1)
      for (int8_t dx = -1; dx <= 1; dx++) {
        int16_t lx = f->px + dx - cx0;
        if (lx >= 0 && lx <= 7) row[(uint8_t)lx] = C_CHROME_MID;
      }
  }
}

static void draw_flipper_col(const flipper_t *f) { draw_flipper_ph(f, phase_of(f)); }

static void draw_ball_col(int16_t bx, int16_t by) {
  int16_t ya = by - 5, ye = by + 5;
  if (ya < ty0) ya = ty0;
  if (ye > ty1) ye = ty1;
  int16_t xa = bx - 5, xe = bx + 5;
  if (xa < cx0) xa = cx0;
  if (xe > cx0 + 7) xe = cx0 + 7;
  if (ya > ye || xa > xe) return;
  uint8_t *row = COLBUF + ((uint16_t)(ya - ty0) << 3);
  for (int16_t y = ya; y <= ye; y++, row += 8) {
    const uint8_t *img = ball_img[y - (by - 5)];
    for (int16_t x = xa; x <= xe; x++) {
      uint8_t c = img[x - (bx - 5)];
      if (c) row[(uint8_t)(x - cx0)] = c;
    }
  }
}
#endif /* CPU_DRAW */

/* ---------- Bild zusammensetzen ---------- */
#define NONE 0xFF
static int16_t buf_cam[2];
static uint8_t need_r0[40], need_r1[40];
/* Was steht in welchem Puffer? Damit muessen ruhende Flipper nicht in jedem
 * Bild neu gezeichnet werden. */
static uint8_t drawn_phase[2][N_FLIP];          /* gezeichnete Flipperphase, NONE = keine */
static int16_t drawn_plg[2];                /* v7.2: gezeichnete Plunger-Hoehe je Puffer */
static uint8_t drawn_ball_on[2][2];        /* [Puffer][Ball], Ball 1 = zweiter Ball im Multiball */
static int16_t drawn_ball[2][2][4];        /* x0, y0, x1, y1 in Tischkoordinaten */

static void need_clear(void) {
  for (uint8_t c = 0; c < 40; c++) need_r0[c] = need_r1[c] = NONE;
}

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

/* Umgebendes Rechteck eines Flippers ueber alle Phasen */
static int16_t fb_x0(const flipper_t *f) { return f->side > 0 ? f->px - 8 : f->px - 44; }

static uint8_t overlaps(const int16_t *r, const flipper_t *f) {
  int16_t x0 = fb_x0(f), y0 = f->py - 24;
  return r[2] >= x0 && r[0] <= x0 + 52 && r[3] >= y0 && r[1] <= y0 + 47;
}

/* ---------- Vorgezeichnete Bilder fuer den DMA-Weg ----------
 * Flipper: je Flipper, Phase und Spalte (bis 8) ein Streifen von 48 Zeilen
 * (384 Byte); Ball: 8 seitliche Verschiebungen x 3 Spalten x 11 Zeilen.
 * Gezeichnet mit denselben Routinen wie der CPU-Weg, Wert 0 = durchsichtig. */
#define FSPR_BYTES 384u
#define BSPR_BASE  (ATTIC_SPR + 0x30000UL)
/* v7.2: Plunger, eine Spalte (x 296-303) mit 48 Zeilen */
#define PSPR_BASE  (ATTIC_SPR + 0x3C000UL)
#define PLG_X      296
#define PLG_ROWS   48
static uint8_t use_dma;

static uint32_t fspr(uint8_t i, uint8_t p, uint8_t k) {
  return ATTIC_SPR + (uint32_t)(((uint16_t)(i * NPH + p) << 3) + k) * FSPR_BYTES;
}

#ifdef PB_BUILDER
static void sprites_build(void) {
  for (uint8_t i = 0; i < 2; i++) {
    const flipper_t *f = &flip[i];
    int16_t xa = f->side > 0 ? f->px - 8 : f->px - 44;
    for (uint8_t p = 0; p < NPH; p++)
      for (uint8_t k = 0; k < 8; k++) {
        for (uint16_t n = 0; n < FSPR_BYTES; n++) COLBUF[n] = 0;
        cx0 = (int16_t)(((xa >> 3) + k) << 3);
        ty0 = f->py - 24;
        ty1 = ty0 + 47;
        draw_flipper_ph(f, p);
        dma_copy(CPUADDR(COLBUF), fspr(i, p, k), FSPR_BYTES);
      }
  }
  for (uint8_t s = 0; s < 8; s++)
    for (uint8_t k = 0; k < 3; k++) {
      for (uint8_t n = 0; n < 88; n++) COLBUF[n] = 0;
      cx0 = (int16_t)k * 8;
      ty0 = 0;
      ty1 = 10;
      draw_ball_col(5 + s, 5);
      dma_copy(CPUADDR(COLBUF), BSPR_BASE + (uint16_t)(s * 3 + k) * 88, 88);
    }
  /* Plunger: rote Kappe, Stange, Feder */
  for (uint16_t n = 0; n < PLG_ROWS * 8; n++) COLBUF[n] = 0;
  for (uint8_t r = 0; r < PLG_ROWS; r++) {
    uint8_t *row = COLBUF + r * 8;
    if (r < 4) { for (uint8_t x = 1; x < 8; x++) row[x] = r == 3 ? C_PLUNGER_DK : C_PLUNGER_TIP; continue; }
    row[3] = row[4] = C_PLUNGER;
    row[5] = C_PLUNGER_DK;
    if (r >= 8 && r < 40 && (r % 3) == 0) { row[1] = row[2] = C_PLUNGER_DK; row[6] = C_PLUNGER; }
  }
  dma_copy(CPUADDR(COLBUF), PSPR_BASE, PLG_ROWS * 8);
}

/* Lader: Flipperspannen und vorgezeichnete Bilder ins Attic-RAM */
void render_build(void) {
  spans_init();
  ball_init();
  sprites_build();
}
#endif /* PB_BUILDER */

/* Der dritte Flipper nutzt die Bilder des rechten unteren (gleiche Form,
 * gleiche Lage innerhalb der Zeichenspalte) */
_Static_assert(((FLIP_R_X - 44) & 7) == ((FLIP3_X - 44) & 7), "Flipper 3 braucht dieselbe x-Lage mod 8");
static void blit_flipper(uint8_t n, uint8_t p, uint32_t fb, int16_t cam) {
  const flipper_t *f = &flip[n];
  uint8_t i = f->side > 0 ? 0 : 1;
  int16_t xa = f->side > 0 ? f->px - 8 : f->px - 44;
  int16_t yt = f->py - 24, s0 = yt, s1 = yt + 47;
  if (s0 < cam) s0 = cam;
  if (s1 > cam + VIEW_H - 1) s1 = cam + VIEW_H - 1;
  if (s0 > s1) return;
  uint16_t count = (uint16_t)(s1 - s0 + 1) * 8;
  uint16_t soff = (uint16_t)(s0 - yt) * 8, doff = (uint16_t)(s0 - cam) * 8;
  uint8_t c0 = (uint8_t)(xa >> 3);
  for (uint8_t k = 0; k < 8 && c0 + k < 40; k++)
    dma_copy_trans(fspr(i, p, k) + soff, fb + (uint32_t)(c0 + k) * COL_BYTES + doff, count);
}

static void blit_plunger(int16_t py, uint32_t fb, int16_t cam) {
  int16_t s0 = py < cam ? cam : py, s1 = py + PLG_ROWS - 1;
  if (s1 > TH - 1) s1 = TH - 1;
  if (s1 > cam + VIEW_H - 1) s1 = cam + VIEW_H - 1;
  if (s0 > s1) return;
  dma_copy_trans(PSPR_BASE + (uint16_t)(s0 - py) * 8,
                 fb + (uint32_t)(PLG_X >> 3) * COL_BYTES + (uint16_t)(s0 - cam) * 8, (uint16_t)(s1 - s0 + 1) * 8);
}

static void blit_ball(int16_t bx, int16_t by, uint32_t fb, int16_t cam) {
  int16_t x0 = bx - 5, yt = by - 5, s0 = yt, s1 = yt + 10;
  if (x0 < 0) x0 = 0;
  if (s0 < cam) s0 = cam;
  if (s1 > cam + VIEW_H - 1) s1 = cam + VIEW_H - 1;
  if (s0 > s1) return;
  uint16_t count = (uint16_t)(s1 - s0 + 1) * 8;
  uint16_t soff = (uint16_t)(s0 - yt) * 8, doff = (uint16_t)(s0 - cam) * 8;
  uint8_t c0 = (uint8_t)(x0 >> 3), s = (uint8_t)(x0 & 7);
  for (uint8_t k = 0; k < 3 && c0 + k < 40; k++)
    dma_copy_trans(BSPR_BASE + (uint16_t)(s * 3 + k) * 88 + soff,
                   fb + (uint32_t)(c0 + k) * COL_BYTES + doff, count);
}

/* Rampe ueber den Ball legen (Ball unter der Bruecke): Deckbild aus dem
 * Attic-RAM transparent ueber die 3 Spalten des Balls kopieren */
static void blit_overlay(int16_t bx, int16_t by, uint32_t fb, int16_t cam) {
  int16_t x0 = bx - 5, s0 = by - 5, s1 = by + 5;
  if (x0 < 0) x0 = 0;
  if (s0 < cam) s0 = cam;
  if (s1 > cam + VIEW_H - 1) s1 = cam + VIEW_H - 1;
  if (s0 > s1) return;
  uint16_t count = (uint16_t)(s1 - s0 + 1) * 8, doff = (uint16_t)(s0 - cam) * 8;
  uint8_t c0 = (uint8_t)(x0 >> 3);
  uint32_t src = ATTIC_OVL + (uint32_t)c0 * STRIP_BYTES + (uint16_t)s0 * 8;
  for (uint8_t k = 0; k < 3 && c0 + k < 40; k++, src += STRIP_BYTES)
    dma_copy_trans(src, fb + (uint32_t)(c0 + k) * COL_BYTES + doff, count);
}

uint8_t render_uses_dma(void) { return use_dma; }

void render_init(void) {
#ifdef CPU_DRAW
  ball_init();
  dma_copy(SPAN_ADDR(NPH), CPUADDR(ph_row0), NPH);
  dma_copy(SPAN_ADDR(NPH) + NPH, CPUADDR(ph_row1), NPH);
  for (uint8_t i = 0; i < N_FLIP; i++) spb_ph[i] = 0xFF;
#endif
  use_dma = dma_trans_ok();
  buf_cam[0] = buf_cam[1] = -1;
  drawn_ball_on[0][0] = drawn_ball_on[0][1] = drawn_ball_on[1][0] = drawn_ball_on[1][1] = 0;
}

int16_t camera_update(void) {
  static int16_t cam16 = (TH - VIEW_H) * 16;
  /* Multiball: der tiefere Ball bestimmt die Kamera */
  fix16 fy = (multi && ball2.b.y > ball.y) ? ball2.b.y : ball.y;
  int16_t target = (int16_t)(fy >> 16) - 100;
  if (target < 0) target = 0;
  if (target > TH - VIEW_H) target = TH - VIEW_H;
  /* v7.5: Attract-Modus: Tisch langsam hoch und runter, an den Enden kurz Pause */
  static int16_t at = TH - VIEW_H;      /* Start unten bei der Abschussbahn */
  static int8_t adir = -1;
  static uint8_t await;
  if (game_over && !hs_entry) {
    if (await) await--;
    else {
      at += adir;                       /* v7.6.2: Richtung nach dem Rand, nicht umkehren (pendelte sonst am Rand) */
      if (at <= 0) { at = 0; adir = 1; await = 75; }
      else if (at >= TH - VIEW_H) { at = TH - VIEW_H; adir = -1; await = 75; }
    }
    target = at;
  } else {
    at = target;
    if (cam_peek && state == ST_PLUNGER) target = 0;   /* v7.8: oben zeigen */
  }
  if (cam_snap) { cam_snap = 0; cam16 = target * 16; }   /* v7.9: Vorschau abgebrochen */
  int16_t diff = target * 16 - cam16;
  if (diff > -4 && diff < 4) cam16 = target * 16;   /* Rest einrasten, sonst bleibt 1 Pixel stehen */
  else {
    int16_t d = diff >> 2;
    if (state == ST_PLUNGER) {                         /* v7.8: in der Abschussbahn ruhig fahren */
      if (d > 8 * 16) d = 8 * 16; else if (d < -8 * 16) d = -8 * 16;
    }
    cam16 += d;
  }
  return cam16 >> 4;
}

uint16_t render_bg_lines;

static uint16_t lines_since(uint16_t r0) {
  uint16_t r1 = raster_line();
  return r1 >= r0 ? r1 - r0 : r1 + 312 - r0;
}

void render_frame(uint8_t b, int16_t cam) {
  uint16_t t0 = raster_line();
  uint32_t fb = b ? FB_B : FB_A;
  uint32_t bg = ATTIC_BG + (uint32_t)(uint16_t)cam * 8;
  uint8_t full = cam != buf_cam[b];

  int16_t bxs[2], bys[2], nb[2][4];
  uint8_t on[2], onr[2];
  bxs[0] = (int16_t)(ball.x >> 16); bys[0] = (int16_t)(ball.y >> 16);
  bxs[1] = (int16_t)(ball2.b.x >> 16); bys[1] = (int16_t)(ball2.b.y >> 16);
  on[0] = state != ST_DRAIN; on[1] = multi;
  onr[0] = on_ramp; onr[1] = ball2.onr;
  for (uint8_t k = 0; k < 2; k++) {
    nb[k][0] = bxs[k] - 5; nb[k][1] = bys[k] - 5; nb[k][2] = bxs[k] + 5; nb[k][3] = bys[k] + 5;
  }
  int16_t plg_y = PLUNGER_Y + (int16_t)((pull * 7) / 20);   /* Oberkante der Plunger-Kappe */
  uint8_t redraw[N_FLIP], phase[N_FLIP];
  for (uint8_t i = 0; i < N_FLIP; i++) {
    phase[i] = phase_of(&flip[i]);
    uint8_t r = full || phase[i] != drawn_phase[b][i];
    for (uint8_t k = 0; k < 2; k++)
      r |= (drawn_ball_on[b][k] && overlaps(drawn_ball[b][k], &flip[i])) || (on[k] && overlaps(nb[k], &flip[i]));
    redraw[i] = r;
  }

  /* 1. Hintergrund: ganz (Kamera bewegt) oder nur die Stellen, die neu
   *    gezeichnet werden (alter Ball, Flipper, die sich geaendert haben) */
  if (full) {
    for (uint8_t c = 0; c < 40; c++)
      dma_copy(bg + c * STRIP_BYTES, fb + (uint32_t)c * COL_BYTES, COL_BYTES);
    buf_cam[b] = cam;
  } else {
    need_clear();
    for (uint8_t k = 0; k < 2; k++)
      if (drawn_ball_on[b][k])
        need_rect(drawn_ball[b][k][0], drawn_ball[b][k][1], drawn_ball[b][k][2], drawn_ball[b][k][3], cam);
    if (plg_y != drawn_plg[b])
      need_rect(PLG_X, plg_y < drawn_plg[b] ? plg_y : drawn_plg[b], PLG_X + 7, TH - 1, cam);
    for (uint8_t i = 0; i < N_FLIP; i++)
      if (redraw[i]) { int16_t x0 = fb_x0(&flip[i]), y0 = flip[i].py - 24; need_rect(x0, y0, x0 + 52, y0 + 47, cam); }
    for (uint8_t c = 0; c < 40; c++) {
      uint8_t r0 = need_r0[c];
      if (r0 == NONE) continue;
      uint16_t off = (uint16_t)r0 * 64, len = (uint16_t)(need_r1[c] - r0 + 1) * 64;
      dma_copy(bg + c * STRIP_BYTES + off, fb + (uint32_t)c * COL_BYTES + off, len);
    }
  }

  render_bg_lines = lines_since(t0);

  /* 2./3. DMA-Weg: vorgezeichnete Bilder transparent einsetzen */
  if (use_dma) {
    for (uint8_t i = 0; i < N_FLIP; i++)
      if (redraw[i]) blit_flipper(i, phase[i], fb, cam);
    blit_plunger(plg_y, fb, cam);
    for (uint8_t k = 0; k < 2; k++)
      if (on[k]) {
        blit_ball(bxs[k], bys[k], fb, cam);
        if (!onr[k] && bys[k] < 370) blit_overlay(bxs[k], bys[k], fb, cam);   /* nur wo die Rampe ist */
      }
  } else {
#ifdef CPU_DRAW
  /* 2. Was muss gezeichnet werden? */
  need_clear();
  for (uint8_t i = 0; i < N_FLIP; i++) {
    if (!redraw[i]) continue;
    int16_t yb = flip[i].py - 24, xa = fb_x0(&flip[i]);
    need_rect(xa, yb + ph_row0[phase[i]], xa + 52, yb + ph_row1[phase[i]], cam);
  }
  for (uint8_t k = 0; k < 2; k++) if (on[k]) need_rect(nb[k][0], nb[k][1], nb[k][2], nb[k][3], cam);

  /* 3. Spaltenweise holen, bemalen, zurueckschreiben */
  for (uint8_t c = 0; c < 40; c++) {
    uint8_t r0 = need_r0[c], r1 = need_r1[c];
    if (r0 == NONE) continue;
    uint16_t off = (uint16_t)r0 * 64, len = (uint16_t)(r1 - r0 + 1) * 64;
    uint32_t addr = fb + (uint32_t)c * COL_BYTES + off;
    dma_copy(addr, CPUADDR(COLBUF), len);
    cx0 = (int16_t)c * 8;
    ty0 = cam + (int16_t)r0 * 8;
    ty1 = cam + (int16_t)r1 * 8 + 7;
    for (uint8_t i = 0; i < N_FLIP; i++)
      if (redraw[i]) draw_flipper_col(&flip[i]);
    for (uint8_t k = 0; k < 2; k++) if (on[k]) draw_ball_col(bxs[k], bys[k]);
    dma_copy(CPUADDR(COLBUF), addr, len);
  }
#endif /* CPU_DRAW */
  }

  /* 4. Merken, was jetzt in diesem Puffer steht */
  for (uint8_t i = 0; i < N_FLIP; i++) drawn_phase[b][i] = phase[i];
  drawn_plg[b] = plg_y;
  for (uint8_t k = 0; k < 2; k++) {
    drawn_ball_on[b][k] = on[k];
    for (uint8_t j = 0; j < 4; j++) drawn_ball[b][k][j] = nb[k][j];
  }
}
