/*
 * table.h – Tischgeometrie, Kollisionsmaske und Platzhaltergrafik
 *
 * Meilenstein 2: ganzer Tisch 320 x 640 nach der Layout-Skizze im Entwurf
 * "LOAD"*",8,1" (ohne Rampe, ohne dritten Flipper). Unterhalb von y 440
 * ist die Geometrie die aus Meilenstein 1, um 64 Pixel nach unten verschoben.
 */
#ifndef TABLE_H
#define TABLE_H

#include <stdint.h>
#include "hw.h"

#define TW        320
#define TH        640
#define BALL_R    5

/* Klassen der Kollisionsmaske */
#define M_EMPTY   0
#define M_WALL    1
#define M_SLING_L 2
#define M_SLING_R 3
#define M_GATE    4           /* Tor an der Abschussbahn: offen, solange der Ball in der Bahn ist */
#define M_GATE_UP 5           /* Einwegtor im linken Orbit: nur nach oben durchlaessig */
#define M_TARGET  16          /* 16..25: zehn Targets, siehe TARGETS in table.c */

/* Zielgruppen: Index der ersten Target-Klasse und Anzahl */
#define TGT_VIC   0           /* 3 Targets V, I, C */
#define TGT_6510  3           /* 4 Targets 6, 5, 1, 0 */
#define TGT_SID   7           /* 3 Targets S, I, D */
#define N_TARGETS 10

/* Flipper */
#define F_LEN   36
#define F_R0    6
#define F_R1    3
#define F_REST  26
#define F_UP   (-22)
#define FLIP_L_X 106
#define FLIP_R_X 194
#define FLIP_Y   562
/* Dritter Flipper oben rechts (rechter Flipper, gleiche Form). Er muss in
 * derselben Pixelspalte innerhalb eines Zeichens liegen wie der rechte untere
 * Flipper (x mod 8 gleich), dann nutzt er dieselben vorgezeichneten Bilder. */
#define FLIP3_X  282
#define FLIP3_Y  300

#define PLUNGER_Y 612
#define LANE_X    300

/* Pop-Bumper: x, y, Radius */
#define N_BUMPERS 3
extern const int16_t BUMPERS[N_BUMPERS][3];

/* Palette (Index 0 bleibt schwarz) */
enum {
  C_BG = 1, C_GRID, C_CHROME_DK, C_CHROME_MID, C_CHROME_HI, C_SLING_BODY,
  C_RUBBER, C_LANE, C_FLIP, C_FLIP_EDGE, C_BALL_DK, C_BALL_MID, C_BALL_HI,
  C_TARGET_VIC, C_TARGET_6510, C_TARGET_SID, C_BUMPER_RING, C_C64_DARK,
  C_C64_LIGHT, C_HOLE, C_HOLE_RING, C_LAMP, C_LABEL, C_TRACE, C_TRACE2,
  C_KICKBACK, C_DMD_BG, C_DMD_OFF, C_DMD_ON, C_DMD_EDGE, C_LOGO_RED, C_LOGO_BLUE,
  /* Lampen: jede hat einen eigenen Paletteneintrag. Ein- und Ausschalten
   * aendert nur die Farbe, die Grafik muss nicht neu gezeichnet werden. */
  C_LAMP0,
  C_TGT0 = C_LAMP0,               /* 10 Targets */
  C_TOP0 = C_TGT0 + 10,           /* 3 obere Lanes C 6 4 */
  C_LOAD0 = C_TOP0 + 3,           /* 4 Lampen L O A D (Out- und Inlanes) */
  C_BUMP0 = C_LOAD0 + 4,          /* 3 Bumper-Ringe (Aufblitzen) */
  C_SLING0 = C_BUMP0 + 3,         /* 2 Slingshot-Gummis (Aufblitzen) */
  C_LAMP_END = C_SLING0 + 2
};
#define N_LAMPS (C_LAMP_END - C_LAMP0)

/* Rollover-Lanes */
#define TOPLANE_Y   52            /* Zaehllinie der oberen Lanes */
#define TOPLANE_X0  114           /* Waende bei 114, 138, 162, 186 */
#define LOWLANE_Y   470           /* Zaehllinie der Out- und Inlanes */


/*
 * Kollisionsmaske: 1 Bit pro Pixel (fest/frei), 40 Byte pro Tischzeile,
 * zusammen 25600 Byte im Attic-RAM ab MASK_FAR. Die CPU arbeitet mit einem
 * Fenster von WIN_ROWS Zeilen (MASKWIN), das bei Bedarf per DMA nachgeladen
 * wird.
 */
#define MASK_FAR  ATTIC_MASK
#define CLS_FAR   ATTIC_CLS
#define WIN_ROWS  48
extern int16_t win_y0;
void mask_load(int16_t y0);
/* Klasse je 8x8-Feld: 40 x 80 Byte im Attic-RAM (CLS_FAR). Mit dem
 * Maskenfenster werden die 7 Klassenzeilen ab cls_r0 nach CLSWIN geladen,
 * das spart 3,2 KB im Hauptspeicher. */
#define CLS_ROWS  7
extern uint8_t cls_r0;
/* Stellt sicher, dass die Tischzeilen ya..yb im Fenster liegen */
static inline void mask_ensure(int16_t ya, int16_t yb) {
  if (ya < win_y0 || yb >= win_y0 + WIN_ROWS) mask_load(((ya + yb) >> 1) - WIN_ROWS / 2);
}

/* Fortschrittsmeldung waehrend des Aufbaus (Phase 0 = Maske, 1 = Grafik) */
void build_progress(uint8_t phase, uint8_t step);

void table_build_mask(void);
void table_build_palette(void);
/* Zeichnet die Tischgrafik spaltenweise ins Attic-RAM (ATTIC_BG) */
void table_build_art(void);

extern const uint8_t MASK_BIT[8];         /* 0x80 >> i, ohne variable Verschiebung */

static inline uint8_t mask_solid(int16_t x, int16_t y) {
  uint16_t r = (uint16_t)(y - win_y0);
  if (r >= WIN_ROWS) { mask_load(y - WIN_ROWS / 2); r = (uint16_t)(y - win_y0); }
  return MASKWIN[(r << 5) + (r << 3) + ((uint16_t)x >> 3)] & MASK_BIT[x & 7];
}
/* Nur fuer Zeilen im Fenster (nach mask_solid oder mask_ensure) */
static inline uint8_t mask_class(int16_t x, int16_t y) {
  uint16_t r = (uint8_t)(((uint16_t)y >> 3) - cls_r0);
  return CLSWIN[(r << 5) + (r << 3) + ((uint16_t)x >> 3)];
}

#endif
