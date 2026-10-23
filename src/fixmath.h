/*
 * fixmath.h – Festkomma-Arithmetik 16.16 fuer Pinball65
 *
 * 65536 entspricht 1.0 (bzw. einem Pixel). Diese Definitionen entsprechen
 * exakt der Physik im Browser-Prototyp (prototype/pinball-prototyp.html).
 */
#ifndef FIXMATH_H
#define FIXMATH_H

#include <stdint.h>
#include <mega65.h>

typedef int32_t fix16;

#define FIX_ONE   65536L
#define INT2FIX(i) ((fix16)(i) << 16)
#define FIX2INT(f) ((int16_t)((f) >> 16))

/* Referenz in Software: (a*b) >> 16, rundet wie der Prototyp Richtung Null. */
static inline fix16 fmul_sw(fix16 a, fix16 b) {
  int64_t p = (int64_t)a * (int64_t)b;
  return (fix16)(p / 65536);
}

/*
 * Mit dem Hardware-Multiplizierer des MEGA65 ($D770-$D77F).
 * Die Einheit rechnet vorzeichenlos 32x32 -> 64 Bit. Das Vorzeichen
 * behandeln wir selbst, damit das Ergebnis exakt fmul_sw entspricht.
 * Die Bytes 2..5 des Produkts ($D77A) sind direkt das Ergebnis >> 16.
 */
static inline fix16 fmul_hw(fix16 a, fix16 b) {
  uint8_t neg = 0;
  uint32_t ua = (uint32_t)a, ub = (uint32_t)b;
  if (a < 0) { ua = (uint32_t)0 - ua; neg ^= 1; }
  if (b < 0) { ub = (uint32_t)0 - ub; neg ^= 1; }
  MATH.multina32 = ua;
  MATH.multinb32 = ub;
  uint32_t r = *(volatile uint32_t *)0xD77A;
  return neg ? -(fix16)r : (fix16)r;
}

#endif
