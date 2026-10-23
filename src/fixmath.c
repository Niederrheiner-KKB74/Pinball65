/*
 * fixmath.c – Multiplikation mit dem Hardware-Multiplizierer
 *
 * Die Einheit rechnet vorzeichenlos 32x32 -> 64 Bit. Fuer fmul behandeln wir
 * das Vorzeichen selbst; die Bytes 2..5 des Produkts ($D77A) sind direkt das
 * Ergebnis >> 16. In Meilenstein 0 auf MEGA65 und Xemu bitgenau geprueft.
 */
#include "fixmath.h"

fix16 fmul(fix16 a, fix16 b) {
  uint8_t neg = 0;
  uint32_t ua = (uint32_t)a, ub = (uint32_t)b;
  if (a < 0) { ua = (uint32_t)0 - ua; neg ^= 1; }
  if (b < 0) { ub = (uint32_t)0 - ub; neg ^= 1; }
  MATH.multina32 = ua;
  MATH.multinb32 = ub;
  uint32_t r = *(volatile uint32_t *)0xD77A;
  return neg ? -(fix16)r : (fix16)r;
}

int32_t mul32(int32_t a, int32_t b) {
  MATH.multina32 = (uint32_t)a;
  MATH.multinb32 = (uint32_t)b;
  return (int32_t)MATH.multout32;
}
