/*
 * LOADTEST – prueft, ob ein Programm, das ueber $A000 hinausreicht, beim
 * Laden vollstaendig und korrekt im Speicher ankommt.
 *
 * Das Programm enthaelt ein 38000 Byte grosses Testmuster und reicht damit
 * bis etwa $C800. Nach dem Start wird jedes Byte mit dem erwarteten Wert
 * verglichen und das Ergebnis ausgegeben.
 */
#include <stdio.h>
#include <stdint.h>

extern const uint8_t PATTERN[38000];

int main(void) {
  const volatile uint8_t *p = PATTERN;
  uint16_t start = (uint16_t)PATTERN, end = (uint16_t)(PATTERN + 38000) - 1;
  uint16_t bad = 0, first = 0, bad_a000 = 0;
  for (uint16_t i = 0; i < 38000; i++) {
    uint8_t want = (uint8_t)((i * 7) + (i >> 8) + 0x5A);
    if (p[i] != want) {
      if (!bad) first = start + i;
      bad++;
      if (start + i >= 0xA000 && start + i < 0xC000) bad_a000++;
    }
  }
  printf("\n LOADTEST PINBALL65\n");
  printf(" MUSTER $%04X-$%04X\n\n", start, end);
  if (!bad) printf(" ERGEBNIS: OK, ALLE BYTES RICHTIG.\n");
  else {
    printf(" ERGEBNIS: %u BYTES FALSCH\n", bad);
    printf(" ERSTES FALSCHES BYTE: $%04X\n", first);
    printf(" DAVON IM BEREICH $A000-$BFFF: %u\n", bad_a000);
  }
  for (;;) {}
}
