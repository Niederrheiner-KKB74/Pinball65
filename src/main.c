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
  CPU_PORTDDR = 65;            /* 40 MHz (entspricht POKE 0,65) */

  printf("\n PINBALL65 - MEILENSTEIN 0\n");
  printf(" ------------------------\n");
  printf(" TOOLCHAIN OK, PROGRAMM LAEUFT.\n\n");
  printf(" TEST FMUL SOFTWARE VS HARDWARE ...\n");

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

  printf(" %u TESTS, %u ABWEICHUNGEN\n", (unsigned)TESTS, (unsigned)errors);
  if (errors) {
    printf(" ERSTE: A=%ld B=%ld\n", (long)first_a, (long)first_b);
    printf("        SW=%ld HW=%ld\n", (long)first_sw, (long)first_hw);
    VICII.bordercolor = 2;    /* rot */
  } else {
    printf(" ERGEBNIS: OK\n");
    VICII.bordercolor = 5;    /* gruen */
  }

  /* Bekannter Wert zum Vergleich mit Xemu: 1.5 * -2.25 = -3.375 */
  fix16 k = fmul_hw(98304L, -147456L);
  printf("\n 1.5 * -2.25 = %ld (ERWARTET -221184)\n", (long)k);
  printf("\n BITTE FOTO/NOTIZ DES ERGEBNISSES.\n");
  return 0;
}
