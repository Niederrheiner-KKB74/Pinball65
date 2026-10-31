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
#include "sound.h"
#ifndef GAME_HI_SIZE
#error "GAME_HI_SIZE (Groesse von PB65HI ohne Ladeadresse) fehlt, siehe Makefile"
#endif

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
/* v8.0: im Codebereich, damit es unter $A000 liegt (der Lader ist inzwischen so gross,
 * dass .noinit bei $C000 laege; dort ist beim Start noch ROM eingeblendet) */
__attribute__((section(".text.zp_save"), used)) uint8_t zp_save[256];
/* v8.0: zp_save[0] = Stapelzeiger beim Aufruf aus BASIC, zp_save[1] = $D030
 * (vor dem Startcode); damit kehrt das Spiel bei RUN/STOP zu BASIC zurueck */
asm(".pushsection .init.005,\"ax\",@progbits\n"
    "  tsx\n"
    "  stx zp_save\n"
    "  lda $d030\n"
    "  sta zp_save+1\n"
    "  ldx #2\n"
    "1:\n"
    "  lda $00,x\n"
    "  sta zp_save,x\n"
    "  inx\n"
    "  bne 1b\n"
    ".popsection\n");

/* Lade-Programm (6502), v7.4 mit Highscore-Datei, v8.1 mit PB65HI:
 *   Zeropage zuruecksichern / DMA auf das alte F018A-Format (das Diskettensystem
 *   des C65 benutzt es) / SETBNK 0,0 / ROMs wie unter BASIC / Rahmen gruen, CLI /
 *   PB65HS laden (Ladeadresse $0F00 aus der Datei; fehlt sie, egal) /
 *   PB65HI laden ($2001, Konstanten und Code fuer $E000) und per DMA ins
 *   Attic-RAM (ATTIC_RODATA) / PB65GAME laden / SEI / bei Fehler Rahmen blinken,
 *   sonst Spiel. v8.1: PB65HI ist eine eigene Datei, damit der Lader klein bleibt
 *   (vorher als Tabelle im Lader, der damit bis $C000 reichte). */
static const uint8_t STUB[] = {
  0xA2, 0x02, 0xBD, 0x00, 0x00, 0x95, 0x00, 0xE8, 0xD0, 0xF8, /*   0 Zeropage $02-$FF aus zp_save */
  0xA9, 0x00, 0x8D, 0x03, 0xD7,                         /*  10 $D703 = 0: DMA im F018A-Format */
  0xA3, 0x00,                                           /*  15 LDZ #0: KERNAL erwartet Z = 0 */
  0xA9, 0x00, 0xA2, 0x00, 0x20, 0x6B, 0xFF,             /*  17 SETBNK: Daten Bank 0, Name Bank 0 */
  0xA9, 0x3F, 0x85, 0x01,                               /*  24 $01 = $3F wie unter BASIC */
  0xA9, 0x64, 0x8D, 0x30, 0xD0,                         /*  28 $D030 = $64: ROM bei $C000 wie unter BASIC */
  0xA9, C_TARGET_6510, 0x8D, 0x20, 0xD0,                /*  33 Rahmen gruen (Farbe aus unserer Palette) */
  0x58,                                                 /*  38 CLI */
  0xA9, 0x00, 0xA2, 0x08, 0xA0, 0x01, 0x20, 0xBA, 0xFF, /*  39 SETLFS 0,8,1 */
  0xA9, 0x00, 0xA2, 0x00, 0xA0, 0x00, 0x20, 0xBD, 0xFF, /*  48 SETNAM PB65HS (wird eingesetzt) */
  0xA9, 0x00, 0x20, 0xD5, 0xFF,                         /*  57 LOAD, Fehler egal */
  0xA9, 0x00, 0xA2, 0x08, 0xA0, 0x01, 0x20, 0xBA, 0xFF, /*  62 SETLFS 0,8,1 */
  0xA9, 0x00, 0xA2, 0x00, 0xA0, 0x00, 0x20, 0xBD, 0xFF, /*  71 SETNAM PB65HI (wird eingesetzt) */
  0xA9, 0x00, 0x20, 0xD5, 0xFF,                         /*  80 LOAD */
  0xB0, 0x2C,                                           /*  85 BCS Fehler (131) */
  0xA9, 0x00, 0x8D, 0x02, 0xD7,                         /*  87 DMA-Liste in Bank 0 */
  0xA9, 0x00, 0x8D, 0x01, 0xD7,                         /*  92 Adresse der Liste, hoch (wird eingesetzt) */
  0xA9, 0x00, 0x8D, 0x05, 0xD7,                         /*  97 ... tief, startet die Kopie nach ATTIC_RODATA */
  0xA9, 0x00, 0xA2, 0x08, 0xA0, 0x01, 0x20, 0xBA, 0xFF, /* 102 SETLFS 0,8,1 */
  0xA9, 0x00, 0xA2, 0x00, 0xA0, 0x00, 0x20, 0xBD, 0xFF, /* 111 SETNAM PB65GAME (wird eingesetzt) */
  0xA9, 0x00, 0x20, 0xD5, 0xFF,                         /* 120 LOAD */
  0x78,                                                 /* 125 SEI */
  0xB0, 0x03,                                           /* 126 BCS Fehler */
  0x4C, 0x00, 0x00,                                     /* 128 JMP Spiel (wird eingesetzt) */
  0xEE, 0x20, 0xD0, 0x4C, 0x00, 0x00,                   /* 131 Fehler: INC $D020, JMP 131 */
  /* 137 DMA-Liste (F018B, mit Optionen): Quelle MB 0, Ziel MB von ATTIC_RODATA */
  0x0B, 0x80, 0x00, 0x81, (uint8_t)(ATTIC_RODATA >> 20), 0x00,
  0x00, 0x00, 0x00,                                     /* 143 Kopie, Laenge (wird eingesetzt) */
  0x01, 0x20, 0x00,                                     /* 146 Quelle $2001, Bank 0 */
  (uint8_t)ATTIC_RODATA, (uint8_t)(ATTIC_RODATA >> 8), (uint8_t)((ATTIC_RODATA >> 16) & 0x0F),   /* 149 Ziel */
  0x00, 0x00, 0x00,                                     /* 152 */
};
#define STUB_NAME (sizeof STUB)
_Static_assert(sizeof STUB == 155, "Sprungziele im Lade-Programm pruefen");
static const char HS_NAME[] = "PB65HS";
static const char HI_NAME[] = "PB65HI";

/* $0800-$1FFF: Bildschirm des C65-Modus und Arbeitsspeicher des KERNAL
 * ($1000-$15FF, Hinweis aus dem Vyper-Projekt). Der Lader braucht den Bereich
 * fuer seine Puffer, deshalb wird er beim Start gesichert und vor dem LOAD
 * zurueckgeschrieben (v3.3 hing sonst weiter im KERNAL). v7.4: Das Spiel nutzt
 * dieselbe Sicherung und die Zeropage (ATTIC_KZP) zum Speichern des Highscores. */
#define LOWRAM_FROM 0x0800u
#define LOWRAM_LEN  0x1800u

/* v7.4: Speicher-Programm fuer den Highscore (6502, laeuft bei $0E00, siehe hiscore.c).
 * Der Lader legt es im Attic-RAM ab, damit es im Spiel keinen Platz kostet.
 * v7.7: Das Spiel blendet das KERNAL bei $E000 aus (MAP leer, $01 = $35); das
 * Programm blendet es fuer SAVE wieder ein (MAP wie beim Laden, siehe kernal_map)
 * und danach wieder aus. */
static const uint8_t SAVE_STUB[] = {
  0xA2, 0x02, 0xB5, 0x00, 0x9D, 0x00, 0x0C, 0xE8, 0xD0, 0xF8, /*   0 Zeropage des Spiels nach $0C00 */
  0xA5, 0x01, 0x8D, 0x85, 0x0E,                         /*  10 $01 merken ($0E85) */
  0xAD, 0x30, 0xD0, 0x8D, 0x86, 0x0E,                   /*  15 $D030 merken ($0E86) */
  0xA2, 0x02, 0xBD, 0x00, 0x0D, 0x95, 0x00, 0xE8, 0xD0, 0xF8, /*  21 Zeropage des KERNAL aus $0D00 */
  0xA9, 0x00, 0x8D, 0x03, 0xD7,                         /*  31 DMA im F018A-Format */
  0xA9, 0x00, 0xA2, 0x00, 0xA0, 0x00, 0xA3, 0x83, 0x5C, 0xEA, /*  36 MAP: KERNAL bei $E000 einblenden (Z bei 44 eingesetzt) */
  0xA3, 0x00,                                           /*  46 LDZ #0 */
  0xA9, 0x00, 0xA2, 0x00, 0x20, 0x6B, 0xFF,             /*  48 SETBNK 0,0 */
  0xA9, 0x3F, 0x85, 0x01,                               /*  55 $01 = $3F */
  0xA9, 0x64, 0x8D, 0x30, 0xD0,                         /*  59 $D030 = $64 */
  0x58,                                                 /*  64 CLI */
  0xA9, 0x00, 0xA2, 0x08, 0xA0, 0x01, 0x20, 0xBA, 0xFF, /*  65 SETLFS 0,8,1 */
  0xA9, 0x09, 0xA2, 0x87, 0xA0, 0x0E, 0x20, 0xBD, 0xFF, /*  74 SETNAM "@0:PB65HS" ($0E87) */
  0xA9, 0x00, 0x85, 0xFB, 0xA9, 0x0F, 0x85, 0xFC,       /*  83 $FB/$FC = $0F00 */
  0xA9, 0xFB, 0xA2, 0x00, 0xA0, 0x0F, 0x20, 0xD8, 0xFF, /*  91 SAVE bis $0F00 + HS_BYTES (X bei 94 eingesetzt) */
  0x78,                                                 /* 100 SEI */
  0xA9, 0x00, 0xA2, 0x00, 0xA0, 0x00, 0xA3, 0x00, 0x5C, 0xEA, /* 101 MAP leer: RAM bei $E000 */
  0xA2, 0x02, 0xBD, 0x00, 0x0C, 0x95, 0x00, 0xE8, 0xD0, 0xF8, /* 111 Zeropage des Spiels zurueck */
  0xAD, 0x85, 0x0E, 0x85, 0x01,                         /* 121 $01 zurueck */
  0xAD, 0x86, 0x0E, 0x8D, 0x30, 0xD0,                   /* 126 $D030 zurueck */
  0x60,                                                 /* 132 RTS */
  0x00, 0x00,                                           /* 133 gemerkte Werte */
  '@', '0', ':', 'P', 'B', '6', '5', 'H', 'S',          /* 135 Dateiname */
};
_Static_assert(sizeof SAVE_STUB == 144, "Adressen im Speicher-Programm pruefen");
_Static_assert(sizeof SAVE_STUB == SAVE_STUB_LEN, "Laenge in hw.h");

/* v7.7.1: Wie ist das KERNAL bei $E000 eingeblendet? Erst pruefen, dass dort
 * das C65- oder C64-KERNAL ($3E000, $2E000) zu sehen ist; dann, ob es mit
 * $01 = $35 verschwindet (dann blendet $01 es ein, MAP bleibt unangetastet)
 * oder nicht (dann MAP, Z = $83: $E000 -> $3E000). 0xFF = unbekannt */
static uint8_t page_is(uint32_t phys) {
  dma_copy(phys, CPUADDR(COLBUF), 256);
  for (uint16_t i = 0; i < 256; i++) if (COLBUF[i] != ((const volatile uint8_t *)0xFF00u)[i]) return 0;
  return 1;
}
static uint8_t kernal_map(void) {
  if (!page_is(0x3FF00UL) && !page_is(0x2FF00UL)) return 0xFF;
  volatile uint8_t *port = (volatile uint8_t *)1;
  uint8_t p = *port;
  *port = 0x35;
  uint8_t still = page_is(0x3FF00UL) || page_is(0x2FF00UL);
  *port = p;
  return still ? 0x83 : 0x00;
}


static void start_game(void) {
  uint8_t kz = kernal_map();
  if (kz == 0xFF) halt("?KERNAL MAP  ERROR", "READY.");
  dma_copy(CPUADDR(SAVE_STUB), ATTIC_SAVESTUB, SAVE_STUB_LEN);
  dma_fill(ATTIC_SAVESTUB + SAVE_STUB_KZ, kz, 1);
  if (!kz) {                                   /* KERNAL ueber $01: MAP nicht anfassen */
    dma_fill(ATTIC_SAVESTUB + SAVE_STUB_MAP1, 0xEA, 1);
    dma_fill(ATTIC_SAVESTUB + SAVE_STUB_MAP2, 0xEA, 1);
  }
  dma_fill(ATTIC_KMODE, kz, 1);
  dma_copy(CPUADDR(zp_save), ATTIC_KZP, 256);
  dma_copy(LOWRAM_SAVE, LOWRAM_FROM, LOWRAM_LEN);
  uint8_t *s = (uint8_t *)STUB_ADDR;
  for (uint8_t i = 0; i < sizeof STUB; i++) s[i] = STUB[i];
  for (uint8_t i = 0; i < sizeof GAME_NAME - 1; i++) s[STUB_NAME + i] = (uint8_t)GAME_NAME[i];
  for (uint8_t i = 0; i < sizeof HS_NAME - 1; i++) s[STUB_NAME + 8 + i] = (uint8_t)HS_NAME[i];
  for (uint8_t i = 0; i < sizeof HI_NAME - 1; i++) s[STUB_NAME + 16 + i] = (uint8_t)HI_NAME[i];
  for (uint8_t i = 0; i < 40; i++) ((uint8_t *)HS_LOAD)[i] = 0;   /* keine Datei: keine Kennung */
  uint16_t name = STUB_ADDR + STUB_NAME, err = STUB_ADDR + 131, save = (uint16_t)zp_save, job = STUB_ADDR + 137;
  s[3] = (uint8_t)save;
  s[4] = (uint8_t)(save >> 8);
  s[49] = (uint8_t)(sizeof HS_NAME - 1);
  s[51] = (uint8_t)(name + 8);
  s[53] = (uint8_t)((name + 8) >> 8);
  s[72] = (uint8_t)(sizeof HI_NAME - 1);
  s[74] = (uint8_t)(name + 16);
  s[76] = (uint8_t)((name + 16) >> 8);
  s[93] = (uint8_t)(job >> 8);
  s[98] = (uint8_t)job;
  s[144] = (uint8_t)GAME_HI_SIZE;
  s[145] = (uint8_t)(GAME_HI_SIZE >> 8);
  s[112] = (uint8_t)(sizeof GAME_NAME - 1);
  s[114] = (uint8_t)name;
  s[116] = (uint8_t)(name >> 8);
  s[129] = (uint8_t)GAME_ENTRY;
  s[130] = (uint8_t)(GAME_ENTRY >> 8);
  s[135] = (uint8_t)err;
  s[136] = (uint8_t)(err >> 8);
  asm volatile("jmp $0E00" ::: "memory");
}

int main(void) {
  dma_copy(LOWRAM_FROM, LOWRAM_SAVE, LOWRAM_LEN);   /* vor jeder Benutzung der Puffer */
  /* v8.0: Zustand von BASIC fuer den Ausstieg sichern: Register $D000-$D07F,
   * Palette, Farb-RAM, BASIC-Zeile des Laders */
  for (uint8_t i = 0; i < 0x80; i++) COLBUF[i] = ((const volatile uint8_t *)0xD000u)[i];
  for (uint16_t i = 0; i < 0x300; i++) COLBUF[0x80 + i] = ((const volatile uint8_t *)0xD100u)[i];
  dma_copy(CPUADDR(COLBUF), ATTIC_BVIC, BVIC_LEN);
  dma_copy(COLRAM, ATTIC_BCOL, 4096);
  dma_copy(0x2001u, ATTIC_BHDR, BHDR_LEN);
  tables_init();
  asm volatile("sei");
  hw_video_init();
  table_build_palette();
  dmd_init();
  dma_fill(FB_A, C_BG, 40 * COL_BYTES);
  dma_fill(FB_B, C_BG, 40 * COL_BYTES);
  dma_fill(DISP_ADDR, C_DMD_BG, DISP_ROWS * ROW_BYTES);
  show_buffer(0);

  if (!attic_ok()) halt("?ATTIC RAM  ERROR", "IN LOADER");
  dmd_text("SEARCHING FOR *", "BUILDING V8.5");
  dmd_flush();

  /* alte Kennung loeschen, falls der Aufbau abbricht */
  dma_fill(ATTIC_SIG, 0, 16);
  table_build_mask();
  table_store_art_palette();
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
