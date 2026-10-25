/*
 * dmd.c – Dot-Matrix-Display im C64-Stil
 */
#include "dmd.h"
#include "hw.h"
#include "table.h"

static char line[2][DMD_COLS + 1];
static uint8_t dirty = 15;           /* Bit n: Zeichenzeile n (0-1 = Zeile 1, 2-3 = Zeile 2) */

static void set_line(uint8_t n, const char *s) {
  char tmp[DMD_COLS + 1];
  uint8_t i = 0;
  if (s) for (; s[i] && i < DMD_COLS; i++) tmp[i] = s[i];
  tmp[i] = 0;
  for (uint8_t k = 0; k <= i; k++)
    if (line[n][k] != tmp[k]) { dirty |= (uint8_t)(3 << (n * 2)); break; }
  for (uint8_t k = 0; k <= i; k++) line[n][k] = tmp[k];
}

void dmd_text(const char *l1, const char *l2) {
  set_line(0, l1);
  set_line(1, l2);
}

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

/* Zeichnet hoechstens EINE Zeichenzeile pro Aufruf: Vorlage per DMA holen,
 * dann nur die leuchtenden Punkte setzen */
void dmd_render(void) {
  static uint8_t next;                       /* reihum, damit keine Zeile verhungert */
  if (!dirty) return;
  uint8_t cr = next;
  while (!(dirty & (1 << cr))) cr = (uint8_t)((cr + 1) & 3);
  next = (uint8_t)((cr + 1) & 3);
  dirty &= (uint8_t)~(1 << cr);
  uint8_t *b = ROWBUF;
  dma_copy(DMD_TPL, CPUADDR(b), ROW_BYTES);
  uint8_t ln = cr >> 1;                       /* Zeichenzeilen 0-1: Zeile 1, 2-3: Zeile 2 */
  uint8_t len = 0;
  while (line[ln][len]) len++;
  uint8_t x0 = (uint8_t)((160 - (len * 6 - 1)) / 2);
  for (uint8_t q = 0; q < 4; q++) {           /* vier Punktreihen pro Zeichenzeile */
    uint8_t gy = (uint8_t)(((cr & 1) << 2) + q);   /* Glyphenzeile 0..7 (7 = Luecke) */
    if (gy == 7) continue;
    uint8_t *row = b + (q << 4);              /* Pixelzeile 2q im ersten Zeichen */
    uint8_t dx = x0;
    for (uint8_t ci = 0; ci < len; ci++, dx += 6) {
      uint8_t ch = (uint8_t)line[ln][ci];
      if (ch >= 'a' && ch <= 'z') ch -= 32;
      if (ch < 32 || ch > 90) continue;
      uint8_t bits = FONT5X7[ch - 32][gy];
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
