/*
 * boot.c – Lader PINBALL65: baut die Tischdaten auf und startet das Spiel
 *
 * Kollisionsmaske, Tischgrafik und die vorgezeichneten Flipper brauchen
 * beim Aufbau rund 11 KB Programmcode, der im Spiel nie wieder laeuft. Darum
 * baut dieses eigene Programm alles im Attic-RAM auf, setzt eine Kennung und
 * laedt danach mit dem KERNAL das Spiel PB65GAME nach, das denselben
 * Speicher ab $2001 belegt.
 *
 * Das Nachladen uebernimmt ein kleines Maschinenprogramm bei $0E00 (im
 * 80-Zeichen-Bildschirmspeicher des C65-Modus, der waehrend eines laufenden
 * Programms nicht beschrieben wird). Es liegt ausserhalb des Bereichs, den das
 * Spiel belegt, und ausserhalb der ROM-Bereiche, die das KERNAL zum Laden
 * einblendet ($A000-$CFFF, $E000-$FFFF).
 */
#include <mega65.h>
#include "hw.h"
#include "table.h"
#include "physics.h"
#include "render.h"
#include "dmd.h"
#include "tables.h"

#ifndef GAME_ENTRY
#error "GAME_ENTRY (Startadresse von PB65GAME) fehlt, siehe Makefile"
#endif

#define STUB_ADDR 0x0E00u

void build_progress(uint8_t phase, uint8_t step) {
  char l2[DMD_COLS + 1], *p;
  p = dmd_put(l2, phase ? "GRAFIK " : "MASKE ");
  p = dmd_num(p, step + 1, 2);
  dmd_put(p, phase ? "/80" : "/14");
  dmd_text("SEARCHING FOR *", l2);
  dmd_flush();
  VICIV.bordercol = (uint8_t)(phase ? 3 : 4);
}

static void halt(const char *l1, const char *l2) {
  dmd_text(l1, l2);
  dmd_flush();
  VICIV.bordercol = 2;
  for (;;) {}
}

static const char GAME_NAME[] = "PB65GAME";

/* Lade-Programm (6502):
 *   DMA auf das alte F018A-Format zurueckstellen (das Diskettensystem des C65
 *   benutzt es; mit F018B hing v3.1 im KERNAL bei $E65A) /
 *   SETBNK 0,0 / SETLFS 0,8,1 / SETNAM / ROMs wie unter BASIC einblenden /
 *   Rahmen gruen, CLI, LOAD, SEI / bei Fehler Rahmen blinken, sonst Spiel */
static const uint8_t STUB[] = {
  0xA9, 0x00, 0x8D, 0x03, 0xD7,                         /*  0 $D703 = 0: DMA im F018A-Format */
  0xA9, 0x00, 0xA2, 0x00, 0x20, 0x6B, 0xFF,             /*  5 SETBNK: Daten Bank 0, Name Bank 0 */
  0xA9, 0x00, 0xA2, 0x08, 0xA0, 0x01, 0x20, 0xBA, 0xFF, /* 12 SETLFS: Datei 0, Geraet 8, Adresse aus Datei */
  0xA9, 0x00, 0xA2, 0x00, 0xA0, 0x00, 0x20, 0xBD, 0xFF, /* 21 SETNAM: Laenge, Adresse (wird eingesetzt) */
  0xA9, 0x3F, 0x85, 0x01,                               /* 30 $01 = $3F wie unter BASIC */
  0xA9, 0x64, 0x8D, 0x30, 0xD0,                         /* 34 $D030 = $64: ROM bei $C000 wie unter BASIC */
  0xA9, 0x05, 0x8D, 0x20, 0xD0,                         /* 39 Rahmen gruen: Laden laeuft */
  0xA9, 0x00, 0x58, 0x20, 0xD5, 0xFF, 0x78,             /* 44 LOAD (A = 0), mit Interrupts */
  0xB0, 0x03,                                           /* 51 BCS Fehler */
  0x4C, 0x00, 0x00,                                     /* 53 JMP Spiel (wird eingesetzt) */
  0xEE, 0x20, 0xD0, 0x4C, 0x00, 0x00,                   /* 56 Fehler: INC $D020, JMP 56 */
};
#define STUB_NAME (sizeof STUB)
_Static_assert(sizeof STUB == 62, "Sprungziele im Lade-Programm pruefen");

static void start_game(void) {
  uint8_t *s = (uint8_t *)STUB_ADDR;
  for (uint8_t i = 0; i < sizeof STUB; i++) s[i] = STUB[i];
  for (uint8_t i = 0; i < sizeof GAME_NAME - 1; i++) s[STUB_NAME + i] = (uint8_t)GAME_NAME[i];
  uint16_t name = STUB_ADDR + STUB_NAME, err = STUB_ADDR + 56;
  s[22] = (uint8_t)(sizeof GAME_NAME - 1);
  s[24] = (uint8_t)name;
  s[26] = (uint8_t)(name >> 8);
  s[54] = (uint8_t)GAME_ENTRY;
  s[55] = (uint8_t)(GAME_ENTRY >> 8);
  s[60] = (uint8_t)err;
  s[61] = (uint8_t)(err >> 8);
  asm volatile("jmp $0E00" ::: "memory");
}

int main(void) {
  tables_init();
  asm volatile("sei");
  hw_video_init();
  table_build_palette();
  dmd_init();
  dma_fill(FB_A, C_BG, 40 * COL_BYTES);
  dma_fill(FB_B, C_BG, 40 * COL_BYTES);
  dma_fill(DISP_ADDR, C_DMD_BG, DISP_ROWS * ROW_BYTES);
  show_buffer(0);

  if (!attic_ok()) halt("?ATTIC RAM  ERROR", "READY.");
  dmd_text("SEARCHING FOR *", "BUILDING  V3.2");
  dmd_flush();

  /* alte Kennung loeschen, falls der Aufbau abbricht */
  dma_fill(ATTIC_SIG, 0, 16);
  table_build_mask();
  table_build_art();
  phys_init();                   /* Flipperpositionen fuer die Bilder */
  render_build();

  for (uint8_t i = 0; i < PB_SIG_LEN; i++) COLBUF[i] = (uint8_t)PB_SIG[i];
  dma_copy(CPUADDR(COLBUF), ATTIC_SIG, PB_SIG_LEN);

  VICIV.bordercol = 0;
  dmd_text("SEARCHING FOR PB65GAME", "LOADING");
  dmd_flush();
  start_game();
  return 0;
}
