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
  C_KICKBACK, C_DMD_BG, C_DMD_OFF, C_DMD_ON, C_DMD_EDGE, C_LOGO_RED, C_LOGO_BLUE
};

/*
 * Kollisionsmaske: 1 Bit pro Pixel (fest/frei), 40 Byte pro Tischzeile,
 * zusammen 25600 Byte im Farb-RAM ab MASK_FAR. Die CPU arbeitet mit einem
 * Fenster von WIN_ROWS Zeilen (MASKWIN), das bei Bedarf per DMA nachgeladen
 * wird.
 */
#define MASK_FAR  0xFF81000UL
#define WIN_ROWS  48
extern int16_t win_y0;
void mask_load(int16_t y0);
/* Klasse je 8x8-Feld: 40 x 80 */
extern uint8_t cls_grid[(TH / 8) * 40];

void table_build_mask(void);
void table_build_palette(void);
/* Zeichnet die Tischgrafik spaltenweise ins Attic-RAM (ATTIC_BG) */
void table_build_art(void);

static inline uint8_t mask_solid(int16_t x, int16_t y) {
  uint16_t r = (uint16_t)(y - win_y0);
  if (r >= WIN_ROWS) { mask_load(y - WIN_ROWS / 2); r = (uint16_t)(y - win_y0); }
  return MASKWIN[(r << 5) + (r << 3) + ((uint16_t)x >> 3)] & (uint8_t)(0x80 >> (x & 7));
}
static inline uint8_t mask_class(int16_t x, int16_t y) {
  uint16_t r = (uint16_t)y >> 3;
  return cls_grid[(r << 5) + (r << 3) + ((uint16_t)x >> 3)];
}

#endif
