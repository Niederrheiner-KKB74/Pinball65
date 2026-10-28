/*
 * dmd.c – Dot-Matrix-Display im C64-Stil
 */
#include <mega65.h>
#include "dmd.h"
#include "hw.h"
#include "table.h"

static char line[2][DMD_COLS + 1];
static uint8_t dirty = 15;           /* Bit n: Zeichenzeile n (0-1 = Zeile 1, 2-3 = Zeile 2) */
/* v6.6 Tipp-Effekt: sichtbare Zeichen je Zeile (255 = alle) und Cursor-Block */
static uint8_t vis[2] = { 255, 255 }, typing, cur_on, cur_ln, cur_ph, tick;
static uint8_t fx, fx_t, fx_rnd = 1;
uint8_t dmd_stripes;

static uint8_t set_line(uint8_t n, const char *s) {
  char tmp[DMD_COLS + 1];
  uint8_t i = 0, ch = 0;
  if (s) for (; s[i] && i < DMD_COLS; i++) tmp[i] = s[i];
  tmp[i] = 0;
  for (uint8_t k = 0; k <= i; k++)
    if (line[n][k] != tmp[k]) { dirty |= (uint8_t)(3 << (n * 2)); ch = 1; break; }
  for (uint8_t k = 0; k <= i; k++) line[n][k] = tmp[k];
  return ch;
}

/* Neuer Text: mit type tippt er Zeichen fuer Zeichen und endet mit blinkendem
 * Cursor. Gleicher Text wie bisher aendert nichts (auch nicht das Tippen). */
void dmd_show(const char *l1, const char *l2, uint8_t type) {
  uint8_t ch = set_line(0, l1);
  ch |= set_line(1, l2);
  if (!ch) return;
  vis[0] = vis[1] = type ? 0 : 255;
  typing = type;
  cur_on = type;
  cur_ln = 0;
  cur_ph = 1;
}

void dmd_text(const char *l1, const char *l2) { dmd_show(l1, l2, 0); }

char *dmd_put(char *p, const char *s) { while (*s) *p++ = *s++; *p = 0; return p; }

/* Dezimalausgabe ohne 32-Bit-Division (spart die Bibliotheksroutine) */
static const uint32_t POW10[10] = {
  1000000000UL, 100000000UL, 10000000UL, 1000000UL, 100000UL,
  10000UL, 1000UL, 100UL, 10UL, 1UL
};
char *dmd_num(char *p, uint32_t v, uint8_t digits) {
  uint8_t started = 0;
  for (uint8_t i = 0; i < 10; i++) {
    char d = '0';
    while (v >= POW10[i]) { v -= POW10[i]; d++; }
    if (d != '0' || started || i == 9 || 10 - i <= digits) { *p++ = d; started = 1; }
  }
  *p = 0;
  return p;
}

void dmd_flush(void) { while (dirty) dmd_render(); }

/* Eine Zeichenzeile mit lauter ausgeschalteten Punkten; einmal berechnet,
 * danach per DMA als Grundlage jeder Neuzeichnung kopiert */
void dmd_init(void) {
  uint8_t *b = ROWBUF;
  for (uint8_t q = 0; q < 4; q++) {
    uint8_t *row = b + (q << 4);
    for (uint8_t dx = 0; dx < 160; dx++) {
      uint8_t *d = row + ((uint16_t)(dx >> 2) << 6) + ((dx & 3) << 1);
      d[0] = C_DMD_OFF; d[1] = C_DMD_BG; d[8] = C_DMD_BG; d[9] = C_DMD_BG;
    }
  }
  dma_copy(CPUADDR(b), DMD_TPL, ROW_BYTES);
}

/* Eine Zeichenzeile cr (0-3): Vorlage per DMA holen, dann nur die leuchtenden
 * Punkte setzen */
static void render_row(uint8_t cr) {
  uint8_t *b = ROWBUF;
  dma_copy(DMD_TPL, CPUADDR(b), ROW_BYTES);
  uint8_t ln = cr >> 1;                       /* Zeichenzeilen 0-1: Zeile 1, 2-3: Zeile 2 */
  uint8_t len = 0;
  while (line[ln][len]) len++;
  uint8_t cur = cur_on && cur_ln == ln;       /* Cursor-Block hinter dem Text */
  uint8_t x0 = (uint8_t)((160 - ((len + cur_on) * 6 - 1)) / 2);   /* Platz fuer den Cursor, damit nichts springt */
  uint8_t n = vis[ln] < len ? vis[ln] : len;
  uint8_t show_cur = cur && cur_ph;
  for (uint8_t q = 0; q < 4; q++) {           /* vier Punktreihen pro Zeichenzeile */
    uint8_t gy = (uint8_t)(((cr & 1) << 2) + q);   /* Glyphenzeile 0..7 (7 = Luecke) */
    if (gy == 7) continue;
    uint8_t *row = b + (q << 4);              /* Pixelzeile 2q im ersten Zeichen */
    uint8_t dx = x0;
    for (uint8_t ci = 0; ci <= n; ci++, dx += 6) {
      uint8_t bits;
      if (ci == n) {
        if (!show_cur) break;
        bits = 0x1F;                          /* Cursor: voller Block */
      } else {
        uint8_t ch = (uint8_t)line[ln][ci];
        if (ch >= 'a' && ch <= 'z') ch -= 32;
        if (ch < 32 || ch > 90) continue;
        bits = FONT5X7[ch - 32][gy];
      }
      for (uint8_t cx = 0; bits; cx++, bits = (uint8_t)(bits << 1) & 0x1F) {
        if (!(bits & 0x10)) continue;
        uint8_t px = dx + cx;                 /* Punkt px belegt Pixel 2px, 2px+1 */
        uint8_t *d = row + ((uint16_t)(px >> 2) << 6) + ((px & 3) << 1);
        d[0] = C_DMD_ON; d[1] = C_DMD_EDGE; d[8] = C_DMD_EDGE;
      }
    }
  }
  dma_copy(CPUADDR(b), DISP_ADDR + (uint32_t)cr * ROW_BYTES, ROW_BYTES);
}

/* v6.4: zeichnet hoechstens eine Textzeile (ihre beiden Zeichenzeilen) pro Aufruf.
 * Vorher kam je Bild nur eine Zeichenzeile dran; fehlte im Multiball die Zeit,
 * zeigten obere und untere Haelfte einer Zeile verschiedene Texte. */
void dmd_render(void) {
  static uint8_t next;                       /* reihum, damit keine Zeile verhungert */
  if (!dirty) return;
  uint8_t ln = next;
  if (!(dirty & (3 << (ln * 2)))) ln ^= 1;
  next = ln ^ 1;
  dirty &= (uint8_t)~(3 << (ln * 2));
  render_row((uint8_t)(ln * 2));
  render_row((uint8_t)(ln * 2 + 1));
}

/* C64-Farben (Index C_C64 + 0..15) fuer Rahmen und Streifen */
static const uint8_t C64RGB[16][3] = {
  {0, 0, 0}, {255, 255, 255}, {129, 51, 56}, {117, 206, 200}, {142, 60, 151}, {86, 172, 77},
  {46, 44, 155}, {237, 241, 113}, {142, 80, 41}, {85, 56, 0}, {196, 108, 113}, {74, 74, 74},
  {123, 123, 123}, {169, 255, 159}, {112, 109, 235}, {178, 178, 178},
};
void dmd_palette(void) {
  for (uint8_t k = 0; k < 16; k++) set_color((uint8_t)(C_C64 + k), C64RGB[k][0], C64RGB[k][1], C64RGB[k][2]);
}

void dmd_fx(uint8_t kind) { fx = kind; fx_t = kind == FX_BLINK ? 100 : kind == FX_PULSE ? 64 : 30; }

/* Leuchtfarbe der Punkte: Helligkeit 0..16 (16 = normal) */
static void dmd_level(uint8_t l) {
  set_color(C_DMD_ON, (uint8_t)(40 + ((215 * l) >> 4)), (uint8_t)(18 + ((142 * l) >> 4)), (uint8_t)(4 + ((36 * l) >> 4)));
  set_color(C_DMD_EDGE, (uint8_t)(40 + ((130 * l) >> 4)), (uint8_t)(18 + ((72 * l) >> 4)), (uint8_t)(4 + ((12 * l) >> 4)));
}

/* Einmal pro Bild: Tippen, Cursor blinken, Farbeffekte */
void dmd_tick(void) {
  tick++;
  if (typing) {
    uint8_t ln = vis[0] < 255 ? 0 : 1;
    if (!(dirty & (3 << (ln * 2))) && !(tick & 1)) {     /* vorheriger Schritt ist gezeichnet */
      uint8_t len = 0;
      while (line[ln][len]) len++;
      cur_ln = ln;
      if (vis[ln] < len) { vis[ln]++; dirty |= (uint8_t)(3 << (ln * 2)); }
      else {
        vis[ln] = 255;
        if (ln == 0 && line[1][0]) { cur_ln = 1; dirty |= 15; }   /* Cursor wandert in Zeile 2 */
        else if (ln == 1 || !line[1][0]) typing = 0;
      }
    }
  } else if (cur_on && !(tick & 15)) {
    cur_ph ^= 1;
    dirty |= (uint8_t)(3 << (cur_ln * 2));
  }
  if (!fx) return;
  fx_t--;
  fx_rnd = (uint8_t)((fx_rnd << 1) ^ ((fx_rnd & 0x80) ? 0x1D : 0));
  if (fx == FX_BLINK) {
    dmd_level((fx_t >> 2) & 1 ? 16 : 1);
    VICIV.bordercol = (uint8_t)(C_C64 + ((fx_t >> 1) & 15));
  } else if (fx == FX_PULSE) {
    uint8_t p = fx_t & 15;
    dmd_level(p < 8 ? (uint8_t)(16 - 2 * p) : (uint8_t)(2 * p - 14));
    dmd_stripes = 1;
  } else {
    dmd_level(fx_rnd & 1 ? 16 : 0);
  }
  if (!fx_t) { fx = 0; dmd_level(16); dmd_stripes = 0; VICIV.bordercol = 0; }
}