/*
 * hw.h – direkter Hardwarezugriff: VIC-IV, DMA, Palette, Tastatur, Raster
 *
 * Nach hw_init() wird das KERNAL nicht mehr aufgerufen. Interrupts bleiben
 * gesperrt, das Timing laeuft ueber Abfrage der Rasterzeile.
 *
 * Bildschirmaufbau (Meilenstein 2): 40 x 25 Zeichen im Full-Colour-Modus.
 *   Zeilen 0-3  : Dot-Matrix-Display (gemeinsame Zeichen fuer beide Puffer)
 *   Zeilen 4-24 : Tischausschnitt, 21 Zeichenzeilen = 168 Pixel
 * Die Zeichen des Tischausschnitts sind SPALTENWEISE nummeriert: die 21
 * Zeichen einer Spalte liegen hintereinander im Speicher. Damit ist eine
 * Bildschirmspalte (1344 Byte) ein zusammenhaengender Block, den eine einzige
 * DMA-Kopie aus dem Hintergrund im Attic-RAM fuellt - auf jede Pixelzeile
 * genau. So scrollt der Tisch pixelgenau ohne Feinscroll-Register.
 */
#ifndef HW_H
#define HW_H

#include <stdint.h>

/* ---------- Speicheraufteilung ---------- */
#define VIEW_ROWS   21          /* Tischausschnitt in Zeichenzeilen */
#define VIEW_H      (VIEW_ROWS * 8)
#define DISP_ROWS   4           /* Display oben */
#define COL_BYTES   (VIEW_ROWS * 64u)   /* eine Bildschirmspalte: 1344 Byte */

#define FB_A        0x40000UL   /* Bildpuffer A: 40 Spalten x 1344 Byte */
#define FB_B        0x4D200UL   /* Bildpuffer B */
#define DISP_ADDR   0x5A400UL   /* Display: 4 x 40 Zeichen, zeilenweise */
#define MAP_A       0x5CC00UL   /* Bildschirmkarte A (1000 x 2 Byte) */
#define MAP_B       0x5D400UL   /* Bildschirmkarte B */
#define DMD_TPL     0x5DC00UL   /* Vorlage: eine Display-Zeichenzeile, alle Punkte aus */
#define TRANS_TEST  0x5E600UL   /* 64 Byte fuer den Transparenz-Selbsttest */
#define FB_A_CHAR   0x1000u     /* = FB_A / 64 */
#define FB_B_CHAR   0x1348u     /* = FB_B / 64 */
#define DISP_CHAR   0x1690u     /* = DISP_ADDR / 64 */

#define ATTIC_BG    0x8000000UL /* Tischgrafik, 40 Spalten-Streifen */
#define ATTIC_SPR   0x8040000UL /* vorgezeichnete Flipper und Ball (Spaltenstreifen) */
#define ATTIC_OVL   0x8080000UL /* Rampe als Deckbild ueber dem Ball, 40 Streifen wie ATTIC_BG */
#define ATTIC_MASK  0x8100000UL /* Kollisionsmaske, 640 x 40 Byte */
#define ATTIC_CLS   0x8110000UL /* Klassenraster der Maske, 80 x 40 Byte */
#define ATTIC_SIG   0x8120000UL /* Kennung: Tischdaten vom Lader aufgebaut */
/* Der Lader (PINBALL65) baut alles im Attic-RAM auf und laedt dann das Spiel
 * (PB65GAME). Das Spiel prueft die Kennung; die Nummer aendert sich, wenn
 * sich der Aufbau der Daten aendert. */
#define PB_SIG      "PB65DATAL"
#define PB_SIG_LEN  9
#define STRIP_BYTES 5120UL      /* 640 Pixelzeilen x 8 Byte */
#define COLRAM      0xFF80000UL
#define ROW_BYTES   2560u       /* 40 Zeichen x 64 Byte */

/* Feste CPU-Puffer im Bereich $0800-$1FFF (frei, seit die Bildschirm-
 * karten im Chip-RAM liegen). Der PC-Test ersetzt sie durch Arrays. */
#ifndef ROWBUF
#define ROWBUF      ((uint8_t *)0x0800)  /* 2560 Byte: eine Zeichenzeile */
#define COLBUF      ((uint8_t *)0x1200)  /* 1344 Byte: eine Bildschirmspalte */
#define MASKWIN     ((uint8_t *)0x1740)  /* 1920 Byte: 48 Maskenzeilen */
#define CLSWIN      ((uint8_t *)0x1EC0)  /* 280 Byte: 7 Zeilen Klassenraster */
#endif

/* 28-Bit-Adresse eines CPU-Puffers (Bank 0) fuer DMA-Auftraege */
#ifndef CPUADDR
#define CPUADDR(p) ((uint32_t)(uint16_t)(p))
#endif

void hw_init(void);
/* Nur Bildschirm und Farben einrichten (der Lader braucht das KERNAL noch) */
void hw_video_init(void);
uint8_t attic_ok(void);
void dma_copy(uint32_t src, uint32_t dst, uint16_t count);
void dma_fill(uint32_t dst, uint8_t value, uint16_t count);
/* Kopie, bei der Quellbytes mit Wert 0 das Ziel nicht ueberschreiben */
void dma_copy_trans(uint32_t src, uint32_t dst, uint16_t count);
uint8_t dma_trans_ok(void);
/* SID-Register reg (0..$18) in den linken ($D400) und rechten ($D440) SID */
void sid_poke(uint8_t reg, uint8_t value);
/* v6.9: dasselbe fuer das zweite SID-Paar ($D420 und $D460, Musik) */
void sid2_poke(uint8_t reg, uint8_t value);
void set_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void show_buffer(uint8_t b);
uint16_t raster_line(void);
/* Rasterzeilen von a bis b (innerhalb eines Bildes, mit Ueberlauf bei 312) */
uint16_t raster_diff(uint16_t a, uint16_t b);
void wait_vblank(void);

/* Tastaturmatrix (C64-Belegung): Spalte 0..7, liefert gedrueckte Zeilen als Bits */
uint8_t key_col(uint8_t col);
/* Joystick Port 2: Bit0 hoch, Bit1 runter, Bit2 links, Bit3 rechts, Bit4 Feuer */
uint8_t joy2(void);

#endif
