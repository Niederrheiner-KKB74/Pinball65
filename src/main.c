/*
 * Pinball65 – Meilenstein 0: Toolchain- und Hardware-Test
 *
 * Prueft, ob Compiler, D81-Erzeugung und Uebertragung funktionieren, und
 * vergleicht die Festkomma-Multiplikation in Software mit dem
 * Hardware-Multiplizierer. Das Ergebnis sollte auf echter Hardware und in
 * Xemu identisch sein: 0 Abweichungen.
 */
#include <mega65.h>
#include <stdio.h>
#include <stdint.h>
#include "fixmath.h"

#define TESTS 2000

static uint32_t rng = 1981;
static int32_t rnd32(void) {
  rng = rng * 1103515245UL + 12345UL;
  return (int32_t)rng;
}

int main(void) {
  /* Kein POKE 0,65: Im C65-Modus laeuft der MEGA65 bereits mit 40 MHz.
   * Ein Schreibzugriff auf $00 aendert die Speicherkonfiguration und
   * blendet das KERNAL aus (das war der Absturz in der ersten Version). */

  printf("\n Pinball65 - Meilenstein 0 (v2)\n");
  printf(" -----------------------------\n");
  printf(" Toolchain OK, Programm laeuft.\n\n");
  printf(" Test fmul Software vs. Hardware ...\n");

  uint16_t errors = 0;
  fix16 first_a = 0, first_b = 0, first_sw = 0, first_hw = 0;
  for (uint16_t i = 0; i < TESTS; i++) {
    /* Wertebereich wie in der Physik: Positionen bis ~±128 px, Faktoren bis ~±8 */
    fix16 a = rnd32() >> 8;
    fix16 b = rnd32() >> 12;
    fix16 sw = fmul_sw(a, b);
    fix16 hw = fmul_hw(a, b);
    if (sw != hw) {
      if (errors == 0) { first_a = a; first_b = b; first_sw = sw; first_hw = hw; }
      errors++;
    }
  }

  printf(" %u Tests, %u Abweichungen\n", (unsigned)TESTS, (unsigned)errors);
  if (errors) {
    printf(" Erste: a=%ld b=%ld\n", (long)first_a, (long)first_b);
    printf("        sw=%ld hw=%ld\n", (long)first_sw, (long)first_hw);
    VICII.bordercolor = 2;    /* rot */
  } else {
    printf(" Ergebnis: OK\n");
    VICII.bordercolor = 5;    /* gruen */
  }

  /* Bekannter Wert zum Vergleich mit Xemu: 1.5 * -2.25 = -3.375 */
  fix16 k = fmul_hw(98304L, -147456L);
  printf("\n 1.5 * -2.25 = %ld (erwartet -221184)\n", (long)k);
  printf("\n Fertig. Zum Beenden Reset druecken.\n");

  /* Nicht ins BASIC zurueckkehren: llvm-mos belegt dessen Zeropage. */
  for (;;) {}
}
