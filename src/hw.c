/*
 * hw.c – Hardwarezugriff fuer Pinball65
 *
 * Die VIC-IV-Einrichtung folgt dem FCM-Beispiel aus dem llvm-mos-SDK
 * (examples/mega65/mandelbrot_fcm.cc), das auf dem MEGA65 erprobt ist.
 */
#include <mega65.h>
#include "hw.h"

/* Enhanced-DMA-Job: Optionen, Ende-Markierung, F018B-Liste (12 Byte).
 * Optionen: $0B F018B-Format, $80/$81 Quell-/Ziel-MB, $85 Ziel-Schrittweite,
 * $06/$07 Transparenz aus/an, $86 Transparenzwert (Bytes mit diesem Wert
 * werden nicht geschrieben). Ob die Transparenz wirkt, prueft dma_trans_ok(). */
struct dmajob {
  uint8_t opt[10];
  uint8_t end;
  uint8_t cmd;
  uint16_t count;
  uint16_t src;
  uint8_t src_bank;
  uint16_t dst;
  uint8_t dst_bank;
  uint8_t cmd_msb;
  uint16_t modulo;
};
_Static_assert(sizeof(struct dmajob) == 23, "DMA-Job muss 23 Byte haben");

static struct dmajob job;

static void dma_run(uint8_t cmd, uint32_t src, uint32_t dst, uint16_t count, uint8_t trans) {
  job.opt[0] = 0x0B;                    /* F018B-Format */
  job.opt[1] = 0x80; job.opt[2] = (uint8_t)(src >> 20);   /* Quell-MB */
  job.opt[3] = 0x81; job.opt[4] = (uint8_t)(dst >> 20);   /* Ziel-MB */
  job.opt[5] = 0x85; job.opt[6] = 1;    /* Ziel-Schrittweite 1 */
  if (trans) { job.opt[7] = 0x07; job.opt[8] = 0x86; job.opt[9] = 0x00; }  /* 0 = durchsichtig */
  else       { job.opt[7] = 0x06; job.opt[8] = 0x06; job.opt[9] = 0x06; }  /* Transparenz aus */
  job.end = 0;
  job.cmd = cmd;
  job.count = count;
  job.src = (uint16_t)src;
  job.src_bank = (uint8_t)(src >> 16) & 0x0F;
  job.dst = (uint16_t)dst;
  job.dst_bank = (uint8_t)(dst >> 16) & 0x0F;
  job.cmd_msb = 0;
  job.modulo = 0;
  asm volatile("" ::: "memory");
  DMA.enable_f018b = 1;
  DMA.addr_bank = 0;
  DMA.addr_msb = (uint8_t)((uint16_t)&job >> 8);
  DMA.trigger_enhanced = (uint8_t)((uint16_t)&job & 0xFF);  /* startet, CPU wartet */
  asm volatile("" ::: "memory");
}

void dma_copy(uint32_t src, uint32_t dst, uint16_t count) { dma_run(0x00, src, dst, count, 0); }
void dma_copy_trans(uint32_t src, uint32_t dst, uint16_t count) { dma_run(0x00, src, dst, count, 1); }
void dma_fill(uint32_t dst, uint8_t value, uint16_t count) { dma_run(0x03, value, dst, count, 0); }

/* Selbsttest der Transparenz: Ziel mit $55 fuellen, Muster mit Nullen
 * transparent darueberkopieren, zuruecklesen. Nullen duerfen nichts
 * ueberschrieben haben. */
uint8_t dma_trans_ok(void) {
  uint8_t *a = COLBUF, *b = COLBUF + 64;
  for (uint8_t i = 0; i < 64; i++) a[i] = (i & 1) ? (uint8_t)(i + 1) : 0;
  dma_fill(TRANS_TEST, 0x55, 64);
  dma_copy(CPUADDR(a), ATTIC_SPR, 64);                 /* Quelle wie im Spiel: Attic */
  dma_copy_trans(ATTIC_SPR, TRANS_TEST, 64);
  dma_copy(TRANS_TEST, CPUADDR(b), 64);
  for (uint8_t i = 0; i < 64; i++)
    if (b[i] != ((i & 1) ? (uint8_t)(i + 1) : 0x55)) return 0;
  return 1;
}

void sid_poke(uint8_t reg, uint8_t value) {
  *(volatile uint8_t *)(0xD400u + reg) = value;
  *(volatile uint8_t *)(0xD440u + reg) = value;
}

/* Paletten-Register erwarten vertauschte Nybbles */
static uint8_t nyb_swap(uint8_t v) { return (uint8_t)((v << 4) | (v >> 4)); }
void set_color(uint8_t i, uint8_t r, uint8_t g, uint8_t b) {
  PALETTE.red[i] = nyb_swap(r);
  PALETTE.green[i] = nyb_swap(g);
  PALETTE.blue[i] = nyb_swap(b);
}

void show_buffer(uint8_t b) {
  uint32_t a = b ? MAP_B : MAP_A;
  VICIV.scrnptr_lsb = (uint8_t)a;
  VICIV.scrnptr_msb = (uint8_t)(a >> 8);
  VICIV.scrnptr_bnk = (uint8_t)(a >> 16);
  VICIV.scrnptr_mb = 0;
}

/* Attic-RAM pruefen: Muster hinschreiben, zuruecklesen, vergleichen */
uint8_t attic_ok(void) {
  uint8_t *a = COLBUF, *b = COLBUF + 256;
  for (uint16_t i = 0; i < 256; i++) { a[i] = (uint8_t)(i * 7 + 3); b[i] = 0; }
  dma_copy(CPUADDR(a), ATTIC_BG + 0x7F000UL, 256);
  dma_copy(ATTIC_BG + 0x7F000UL, CPUADDR(b), 256);
  for (uint16_t i = 0; i < 256; i++) if (a[i] != b[i]) return 0;
  return 1;
}

uint16_t raster_line(void) {
  uint8_t lo, hi;
  do {                                  /* konsistent lesen */
    hi = VICIV.ctrl1 & 0x80;
    lo = VICIV.rasterline;
  } while (hi != (VICIV.ctrl1 & 0x80));
  return ((uint16_t)hi << 1) | lo;
}

uint16_t raster_diff(uint16_t a, uint16_t b) { return b >= a ? b - a : b + 312 - a; }

/* Wartet auf den Beginn des unteren Rahmens (Zeile 251, PAL) */
void wait_vblank(void) {
  while (raster_line() >= 251) {}
  while (raster_line() < 251) {}
}


uint8_t key_col(uint8_t col) {
  CIA1.ddra = 0xFF;
  CIA1.pra = (uint8_t)~(1u << col);
  return (uint8_t)~CIA1.prb;
}

uint8_t joy2(void) {
  CIA1.ddra = 0x00;
  uint8_t v = (uint8_t)~CIA1.pra & 0x1F;
  CIA1.ddra = 0xFF;
  return v;
}

void hw_init(void) {
  asm volatile("sei");

  /* VIC-IV freischalten und VIC-II-"Hot-Register" abschalten */
  VICIV.key = 0x47;
  VICIV.key = 0x53;
  VICIV.sdbdrwd_msb &= ~VIC4_HOTREG_MASK;

  /* 40 MHz. Danach darf das KERNAL nicht mehr aufgerufen werden. */
  CPU_PORTDDR = 65;

  VICIV.ctrlb = (VICIV.ctrlb | VIC3_FAST_MASK | VIC3_ATTR_MASK) &
                ~(VIC3_H640_MASK | VIC3_V400_MASK);
  /* 16-Bit-Zeichennummern, Full Colour fuer Zeichen > $FF */
  VICIV.ctrlc = (VICIV.ctrlc & ~VIC4_FCLRLO_MASK) | VIC4_CHR16_MASK | VIC4_FCLRHI_MASK;

  VICIV.charptr_lsb = 0;
  VICIV.charptr_msb = 0;
  VICIV.charptr_bnk = 0;
  VICIV.linestep = 80;                  /* 2 Byte pro Zeichen */
  VICIV.chrcount = 40;
  VICIV.disp_rows = 25;
  VICIV.ctrl1 = (VICIV.ctrl1 & 0xC0) | 0x1B;
  VICIV.ctrl2 = (VICIV.ctrl2 & 0xE0) | 0x08;
  VICIV.bordercol = 0;
  VICIV.screencol = 0;
  VICIV.ctrla |= VIC3_PAL_MASK;

  /* Farb-RAM neutral (keine Spiegelung, kein Alpha, kein NCM) */
  dma_fill(COLRAM, 0, 4096);

  /* Zwei Bildschirmkarten im Chip-RAM: Display oben gemeinsam, darunter
   * die spaltenweise nummerierten Zeichen des jeweiligen Bildpuffers */
  uint16_t *m = (uint16_t *)ROWBUF;
  for (uint8_t buf = 0; buf < 2; buf++) {
    uint16_t base = buf ? FB_B_CHAR : FB_A_CHAR;
    for (uint8_t r = 0; r < 25; r++)
      for (uint8_t c = 0; c < 40; c++)
        m[r * 40 + c] = r < DISP_ROWS ? (uint16_t)(DISP_CHAR + r * 40 + c)
                                      : (uint16_t)(base + c * VIEW_ROWS + (r - DISP_ROWS));
    dma_copy(CPUADDR(ROWBUF), buf ? MAP_B : MAP_A, 2000);
  }

  CIA1.ddra = 0xFF;
  CIA1.ddrb = 0x00;
}
