/*
 * dmd.h – Dot-Matrix-Display oben: 160 x 16 Punkte, zwei Textzeilen
 *
 * Jeder Punkt ist 2 x 2 Pixel gross, das Display belegt die oberen vier
 * Zeichenzeilen (320 x 32 Pixel). Gezeichnet wird nur, wenn sich der Text
 * aendert.
 */
#ifndef DMD_H
#define DMD_H

#include <stdint.h>

#define DMD_COLS 26              /* Zeichen pro Zeile (6 Punkte breit) */

extern const uint8_t FONT5X7[59][7];

/* Legt die leere Punktzeile im Chip-RAM an (einmal nach hw_init) */
void dmd_init(void);
/* Setzt beide Zeilen (NULL = leer); Text wird zentriert */
void dmd_text(const char *l1, const char *l2);
/* Zeichnet hoechstens eine geaenderte Textzeile (beide Haelften) */
void dmd_render(void);
/* Zeichnet alle geaenderten Zeilen sofort (Start, Fehlermeldungen) */
void dmd_flush(void);

/* Hilfen zum Zusammensetzen von Text */
char *dmd_put(char *p, const char *s);
char *dmd_num(char *p, uint32_t v, uint8_t digits);   /* digits 0 = ohne fuehrende Nullen */

#endif
