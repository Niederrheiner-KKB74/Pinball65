/*
 * hw.h – direkter Hardwarezugriff: VIC-IV, DMA, Palette, Tastatur, Raster
 *
 * Nach hw_init() wird das KERNAL nicht mehr aufgerufen. Interrupts bleiben
 * gesperrt, das Timing laeuft ueber Abfrage der Rasterzeile.
 */
#ifndef HW_H
#define HW_H

#include <stdint.h>

/* Speicheraufteilung (Chip-RAM) */
#define FB_A        0x40000UL   /* Bildpuffer A: 1000 FCM-Zeichen, Zeichen $1000.. */
#define FB_B        0x50000UL   /* Bildpuffer B: 1000 FCM-Zeichen, Zeichen $1400.. */
#define BG_ADDR     0x10000UL   /* unveraenderter Hintergrund, 24 Zeichenzeilen */
#define COLRAM      0xFF80000UL
#define SCREEN_A    ((volatile uint16_t *)0x0800)
#define SCREEN_B    ((volatile uint16_t *)0x1000)

#define ROW_BYTES   2560u       /* 40 Zeichen x 64 Byte */
#define VIEW_ROWS   24          /* Tischausschnitt: 24 Zeichenzeilen = 192 Pixel */
#define HUD_ROW     24          /* Zeile 25: Statuszeile */

/* 28-Bit-Adresse eines CPU-Puffers (Bank 0) fuer DMA-Auftraege */
#ifndef CPUADDR
#define CPUADDR(p) ((uint32_t)(uint16_t)(p))
#endif

void hw_init(void);
void dma_copy(uint32_t src, uint32_t dst, uint16_t count);
void dma_fill(uint32_t dst, uint8_t value, uint16_t count);
void set_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void show_buffer(uint8_t b);
uint16_t raster_line(void);
void wait_vblank(void);

/* Tastaturmatrix (C64-Belegung): Spalte 0..7, liefert gedrueckte Zeilen als Bits */
uint8_t key_col(uint8_t col);
/* Joystick Port 2: Bit0 hoch, Bit1 runter, Bit2 links, Bit3 rechts, Bit4 Feuer */
uint8_t joy2(void);

#endif
