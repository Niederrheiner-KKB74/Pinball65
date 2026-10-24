/*
 * dmd.c – Dot-Matrix-Display im C64-Stil
 */
#include "dmd.h"
#include "hw.h"
#include "table.h"

static char line[2][DMD_COLS + 1];
static uint8_t dirty = 3;            /* Bit 0: Zeile 1, Bit 1: Zeile 2 */

static void set_line(uint8_t n, const char *s) {
  char tmp[DMD_COLS + 1];
  uint8_t i = 0;
  if (s) for (; s[i] && i < DMD_COLS; i++) tmp[i] = s[i];
  tmp[i] = 0;
  for (uint8_t k = 0; k <= i; k++)
    if (line[n][k] != tmp[k]) { dirty |= (uint8_t)(1 << n); break; }
  for (uint8_t k = 0; k <= i; k++) line[n][k] = tmp[k];
}

void dmd_text(const char *l1, const char *l2) {
  set_line(0, l1);
  set_line(1, l2);
}

char *dmd_put(char *p, const char *s) { while (*s) *p++ = *s++; *p = 0; return p; }

char *dmd_num(char *p, uint32_t v, uint8_t digits) {
  char buf[11];
  uint8_t n = 0;
  do { buf[n++] = (char)('0' + v % 10); v /= 10; } while (v && n < 10);
  while (n < digits) buf[n++] = '0';
  while (n) *p++ = buf[--n];
  *p = 0;
  return p;
}

void dmd_render(void) {
  if (!dirty) return;
  uint8_t *b = ROWBUF;
  for (uint8_t cr = 0; cr < DISP_ROWS; cr++) {
    uint8_t ln = cr >> 1;                       /* Zeichenzeilen 0-1: Zeile 1, 2-3: Zeile 2 */
    if (!(dirty & (1 << ln))) continue;
    uint8_t len = 0;
    while (line[ln][len]) len++;
    uint8_t x0 = (uint8_t)((160 - (len * 6 - 1)) / 2);
    for (uint8_t q = 0; q < 4; q++) {           /* vier Punktreihen pro Zeichenzeile */
      uint8_t gy = (uint8_t)(((cr & 1) << 2) + q);   /* Glyphenzeile 0..7 (7 = Luecke) */
      uint8_t py = q * 2;
      uint8_t ci = 0, cx = 0;
      for (uint8_t dx = 0; dx < 160; dx++) {
        uint8_t on = 0;
        if (dx >= x0 && ci < len && gy < 7) {
          if (cx < 5) {
            uint8_t ch = (uint8_t)line[ln][ci];
            if (ch >= 'a' && ch <= 'z') ch -= 32;
            if (ch >= 32 && ch <= 90) on = FONT5X7[ch - 32][gy] & (0x10 >> cx);
          }
          if (++cx == 6) { cx = 0; ci++; }
        }
        uint16_t x = (uint16_t)dx * 2;
        uint16_t i = ((x >> 3) << 6) + (py << 3) + (x & 7);
        b[i] = on ? C_DMD_ON : C_DMD_OFF;
        b[i + 1] = on ? C_DMD_EDGE : C_DMD_BG;
        b[i + 8] = on ? C_DMD_EDGE : C_DMD_BG;
        b[i + 9] = C_DMD_BG;
      }
    }
    dma_copy(CPUADDR(b), DISP_ADDR + (uint32_t)cr * ROW_BYTES, ROW_BYTES);
  }
  dirty = 0;
}
