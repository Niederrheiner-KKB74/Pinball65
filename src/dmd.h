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
#include "hw.h"

#define DMD_COLS 26              /* Zeichen pro Zeile (6 Punkte breit) */

extern LOWDATA uint8_t FONT5X7[59][7];   /* v7.6.2: im Spiel aus dem Attic-RAM */

/* Legt die leere Punktzeile im Chip-RAM an (einmal nach hw_init) */
void dmd_init(void);
/* Setzt beide Zeilen (NULL = leer); Text wird zentriert */
void dmd_text(const char *l1, const char *l2);
/* v6.6: wie dmd_text; type = 1 tippt den Text Zeichen fuer Zeichen mit Cursor */
void dmd_show(const char *l1, const char *l2, uint8_t type);
/* Einmal pro Bild aufrufen: Tippen, Cursor, Farbeffekte */
void dmd_tick(void);
/* Farbeffekte im Display (nur Palette): schnelles Invertieren (Multiball),
 * mehrmaliges Invertieren (Jackpot, Mission, ...), Flackern (Ball verloren) */
enum { FX_NONE, FX_BLINK, FX_INVERT, FX_FLICKER };
void dmd_fx(uint8_t kind);
/* Zeichnet hoechstens eine geaenderte Textzeile (beide Haelften) */
void dmd_render(void);
/* Zeichnet alle geaenderten Zeilen sofort (Start, Fehlermeldungen) */
void dmd_flush(void);

/* Hilfen zum Zusammensetzen von Text */
char *dmd_put(char *p, const char *s);
char *dmd_num(char *p, uint32_t v, uint8_t digits);   /* digits 0 = ohne fuehrende Nullen */

#endif
