/*
 * fixmath.h – Festkomma-Arithmetik 16.16 fuer Pinball65
 *
 * 65536 entspricht 1.0 (bzw. einem Pixel). Diese Definitionen entsprechen
 * exakt der Physik im Browser-Prototyp (prototype/pinball-prototyp.html).
 * In Meilenstein 0 auf MEGA65 und Xemu bitgenau geprueft.
 */
#ifndef FIXMATH_H
#define FIXMATH_H

#include <stdint.h>
#include <mega65.h>
#include "tables.h"

typedef int32_t fix16;

#define FIX_ONE   65536L

/* Referenz in Software: (a*b) >> 16, rundet wie der Prototyp Richtung Null. */
static inline fix16 fmul_sw(fix16 a, fix16 b) {
  int64_t p = (int64_t)a * (int64_t)b;
  return (fix16)(p / 65536);
}

/* Mit dem Hardware-Multiplizierer des MEGA65 ($D770), Ergebnis wie fmul_sw */
fix16 fmul(fix16 a, fix16 b);
/* Ganzzahliges 32x32-Produkt (untere 32 Bit), vorzeichenrichtig */
int32_t mul32(int32_t a, int32_t b);

/* Winkel in 256 Schritten: 0 = rechts, 64 = unten (Bildschirm-y waechst nach unten) */
static inline fix16 cosA(uint8_t a) { return sin_tab((uint8_t)(a + 64)); }
static inline fix16 sinA(uint8_t a) { return sin_tab(a); }

uint8_t angle256(int32_t dx, int32_t dy);

#endif
