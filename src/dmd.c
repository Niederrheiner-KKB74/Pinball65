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

/* Zeichnet hoechstens EINE Zeichenzeile pro Aufruf (verteilt die Arbeit
 * auf mehrere Bilder; eine ganze Neuzeichnung dauert vier Bilder) */
void dmd_render(void) {
  static uint8_t next;                       /* reihum, damit keine Zeile verhungert */
  if (!dirty) return;
  uint8_t cr = next;
  while (!(dirty & (1 << cr))) cr = (uint8_t)((cr + 1) & 3);
  next = (uint8_t)((cr + 1) & 3);
  dirty &= (uint8_t)~(1 << cr);
  uint8_t *b = ROWBUF;
  uint8_t ln = cr >> 1;                       /* Zeichenzeilen 0-1: Zeile 1, 2-3: Zeile 2 */
  uint8_t len = 0;
  while (line[ln][len]) len++;
  uint8_t x0 = (uint8_t)((160 - (len * 6 - 1)) / 2);
  for (uint8_t q = 0; q < 4; q++) {           /* vier Punktreihen pro Zeichenzeile */
    uint8_t gy = (uint8_t)(((cr & 1) << 2) + q);   /* Glyphenzeile 0..7 (7 = Luecke) */
    uint8_t *row = b + (q << 4);              /* Pixelzeile 2q im ersten Zeichen */
    uint8_t ci = 0, cx = 0, bits = 0;
    for (uint8_t dx = 0; dx < 160; dx++) {
      uint8_t on = 0;
      if (dx >= x0 && ci < len && gy < 7) {
        if (cx == 0) {
          uint8_t ch = (uint8_t)line[ln][ci];
          if (ch >= 'a' && ch <= 'z') ch -= 32;
          bits = (ch >= 32 && ch <= 90) ? FONT5X7[ch - 32][gy] : 0;
        }
        on = bits & (0x10 >> cx);
        if (++cx == 6) { cx = 0; ci++; }
      }
      /* Punkt dx belegt Pixel 2dx, 2dx+1 in zwei Pixelzeilen */
      uint8_t *d = row + ((uint16_t)(dx >> 2) << 6) + ((dx & 3) << 1);
      if (on) { d[0] = C_DMD_ON; d[1] = C_DMD_EDGE; d[8] = C_DMD_EDGE; }
      else    { d[0] = C_DMD_OFF; d[1] = C_DMD_BG; d[8] = C_DMD_BG; }
      d[9] = C_DMD_BG;
    }
  }
  dma_copy(CPUADDR(b), DISP_ADDR + (uint32_t)cr * ROW_BYTES, ROW_BYTES);
}
