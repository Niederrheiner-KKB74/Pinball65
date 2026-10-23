/*
 * table.h – Tischgeometrie, Kollisionsmaske und Platzhaltergrafik
 *
 * Die Geometrie ist identisch mit prototype/pinball-prototyp.html.
 */
#ifndef TABLE_H
#define TABLE_H

#include <stdint.h>

#define TW        320
#define TH        576
#define VIEW_TOP  384          /* Meilenstein 1: fester Ausschnitt, untere 192 Zeilen */
#define BALL_R    5

/* Klassen der Kollisionsmaske */
#define M_EMPTY   0
#define M_WALL    1
#define M_SLING_L 2
#define M_SLING_R 3
#define M_GATE    4
#define M_TARGET  16

/* Flipper */
#define F_LEN   36
#define F_R0    6
#define F_R1    3
#define F_REST  26
#define F_UP   (-22)
#define FLIP_L_X 106
#define FLIP_R_X 194
#define FLIP_Y   498

#define PLUNGER_Y 548
#define LANE_X    300

/* Palette (Index 0 und 255 bleiben frei) */
enum {
  C_BG = 1, C_GRID, C_CHROME_DK, C_CHROME_MID, C_CHROME_HI, C_SLING_BODY,
  C_RUBBER, C_LANE, C_FLIP, C_FLIP_EDGE, C_BALL_DK, C_BALL_MID, C_BALL_HI,
  C_HUD_BG, C_HUD_TXT, C_IND, C_TARGET, C_HUD_DIM
};

/*
 * Kollisionsmaske: 1 Bit pro Pixel (fest/frei), 40 Byte pro Tischzeile,
 * zusammen 23040 Byte. Sie liegt im Farb-RAM hinter den 4 KB, die der
 * Bildschirm braucht. Die CPU arbeitet mit einem Fenster von WIN_ROWS
 * Zeilen, das bei Bedarf per DMA nachgeladen wird.
 */
#define MASK_FAR  0xFF81000UL
#define WIN_ROWS  128
extern uint8_t mask_win[WIN_ROWS * 40];
extern int16_t win_y0;
void mask_load(int16_t y0);
/* Klasse je 8x8-Feld: 40 x 72 */
extern uint8_t cls_grid[(TH / 8) * 40];

void table_build_mask(void);
void table_build_palette(void);
/* Zeichnet den Hintergrund in BG_ADDR und kopiert ihn in beide Bildpuffer */
void table_build_art(uint8_t *rowbuf);

static inline uint8_t mask_solid(int16_t x, int16_t y) {
  uint16_t r = (uint16_t)(y - win_y0);
  if (r >= WIN_ROWS) { mask_load(y - WIN_ROWS / 2); r = (uint16_t)(y - win_y0); }
  return mask_win[(r << 5) + (r << 3) + ((uint16_t)x >> 3)] & (uint8_t)(0x80 >> (x & 7));
}
static inline uint8_t mask_class(int16_t x, int16_t y) {
  uint16_t r = (uint16_t)y >> 3;
  return cls_grid[(r << 5) + (r << 3) + ((uint16_t)x >> 3)];
}

#endif
