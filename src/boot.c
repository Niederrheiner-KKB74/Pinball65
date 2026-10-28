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
  static const char *const NAME[3] = { "MASKE ", "GRAFIK ", "RAMPE " };
  static const char *const OF[3] = { "/14", "/80", "/38" };
  p = dmd_put(l2, NAME[phase]);
  p = dmd_num(p, step + 1, 2);
  dmd_put(p, OF[phase]);
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

/* Zeropage beim Programmstart sichern, bevor der C-Startcode sie belegt.
 * Das MEGA65-KERNAL nutzt die ganze Zeropage; v3.1 und v3.2 hingen beim
 * Nachladen im KERNAL bei $E65A, weil llvm-mos $02-$8F ueberschrieben hatte.
 * Das Lade-Programm stellt sie vor dem LOAD wieder her. */
__attribute__((section(".noinit"), used)) uint8_t zp_save[256];
asm(".pushsection .init.005,\"ax\",@progbits\n"
    "  ldx #2\n"
    "1:\n"
    "  lda $00,x\n"
    "  sta zp_save,x\n"
    "  inx\n"
    "  bne 1b\n"
    ".popsection\n");

/* Lade-Programm (6502):
 *   Zeropage zuruecksichern / DMA auf das alte F018A-Format (das Diskettensystem
 *   des C65 benutzt es) / SETBNK 0,0 / SETLFS 0,8,1 / SETNAM / ROMs wie unter
 *   BASIC / Rahmen gruen, CLI, LOAD, SEI / bei Fehler Rahmen blinken, sonst Spiel */
static const uint8_t STUB[] = {
  0xA2, 0x02, 0xBD, 0x00, 0x00, 0x95, 0x00, 0xE8, 0xD0, 0xF8, /*  0 Zeropage $02-$FF aus zp_save */
  0xA9, 0x00, 0x8D, 0x03, 0xD7,                         /* 10 $D703 = 0: DMA im F018A-Format */
  0xA3, 0x00,                                           /* 15 LDZ #0: KERNAL erwartet Z = 0 */
  0xA9, 0x00, 0xA2, 0x00, 0x20, 0x6B, 0xFF,             /* 17 SETBNK: Daten Bank 0, Name Bank 0 */
  0xA9, 0x00, 0xA2, 0x08, 0xA0, 0x01, 0x20, 0xBA, 0xFF, /* 24 SETLFS: Datei 0, Geraet 8, Adresse aus Datei */
  0xA9, 0x00, 0xA2, 0x00, 0xA0, 0x00, 0x20, 0xBD, 0xFF, /* 33 SETNAM: Laenge, Adresse (wird eingesetzt) */
  0xA9, 0x3F, 0x85, 0x01,                               /* 42 $01 = $3F wie unter BASIC */
  0xA9, 0x64, 0x8D, 0x30, 0xD0,                         /* 46 $D030 = $64: ROM bei $C000 wie unter BASIC */
  0xA9, C_TARGET_6510, 0x8D, 0x20, 0xD0,                /* 51 Rahmen gruen (Farbe aus unserer Palette) */
  0xA9, 0x00, 0x58, 0x20, 0xD5, 0xFF, 0x78,             /* 56 LOAD (A = 0), mit Interrupts */
  0xB0, 0x03,                                           /* 63 BCS Fehler */
  0x4C, 0x00, 0x00,                                     /* 65 JMP Spiel (wird eingesetzt) */
  0xEE, 0x20, 0xD0, 0x4C, 0x00, 0x00,                   /* 68 Fehler: INC $D020, JMP 68 */
};
#define STUB_NAME (sizeof STUB)
_Static_assert(sizeof STUB == 74, "Sprungziele im Lade-Programm pruefen");

/* $0800-$1FFF: Bildschirm des C65-Modus und Arbeitsspeicher des KERNAL
 * ($1000-$15FF, Hinweis aus dem Vyper-Projekt). Der Lader braucht den Bereich
 * fuer seine Puffer, deshalb wird er beim Start gesichert und vor dem LOAD
 * zurueckgeschrieben (v3.3 hing sonst weiter im KERNAL). */
#define LOWRAM_SAVE (ATTIC_SIG + 0x1000UL)
#define LOWRAM_FROM 0x0800u
#define LOWRAM_LEN  0x1800u

static void start_game(void) {
  dma_copy(LOWRAM_SAVE, LOWRAM_FROM, LOWRAM_LEN);
  uint8_t *s = (uint8_t *)STUB_ADDR;
  for (uint8_t i = 0; i < sizeof STUB; i++) s[i] = STUB[i];
  for (uint8_t i = 0; i < sizeof GAME_NAME - 1; i++) s[STUB_NAME + i] = (uint8_t)GAME_NAME[i];
  uint16_t name = STUB_ADDR + STUB_NAME, err = STUB_ADDR + 68, save = (uint16_t)zp_save;
  s[3] = (uint8_t)save;
  s[4] = (uint8_t)(save >> 8);
  s[34] = (uint8_t)(sizeof GAME_NAME - 1);
  s[36] = (uint8_t)name;
  s[38] = (uint8_t)(name >> 8);
  s[66] = (uint8_t)GAME_ENTRY;
  s[67] = (uint8_t)(GAME_ENTRY >> 8);
  s[72] = (uint8_t)err;
  s[73] = (uint8_t)(err >> 8);
  asm volatile("jmp $0E00" ::: "memory");
}

int main(void) {
  dma_copy(LOWRAM_FROM, LOWRAM_SAVE, LOWRAM_LEN);   /* vor jeder Benutzung der Puffer */
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
  dmd_text("SEARCHING FOR *", "BUILDING  V6.7");
  dmd_flush();

  /* alte Kennung loeschen, falls der Aufbau abbricht */
  dma_fill(ATTIC_SIG, 0, 16);
  table_build_mask();
  table_build_art();
  table_build_overlay();
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
