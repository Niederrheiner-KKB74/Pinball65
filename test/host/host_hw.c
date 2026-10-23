/*
 * host_hw.c – Simulation von Chip-RAM, Farb-RAM und DMA fuer den PC-Test.
 * Ersetzt src/hw.c und src/fixmath.c. Keine Aussage ueber Timing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mega65.h"
#include "hw.h"
#include "fixmath.h"

struct host_vic VICIV;
uint8_t chip[0x60000];
uint8_t colram[0x8000];
uint8_t pal[256][3];
uint8_t shown;
static uint8_t *regs[8];
static int nregs;

uint32_t host_reg(void *p) {
  for (int i = 0; i < nregs; i++) if (regs[i] == p) return 0xE000000UL + ((uint32_t)i << 16);
  regs[nregs] = p;
  return 0xE000000UL + ((uint32_t)nregs++ << 16);
}

static uint8_t *resolve(uint32_t a, uint16_t n) {
  if ((a >> 24) == 0xE) return regs[(a >> 16) & 0xFF] + (a & 0xFFFF);
  if (a >= 0xFF80000UL) {
    if (a - 0xFF80000UL + n > sizeof colram) { fprintf(stderr, "Farb-RAM ueberlaufen %07lx\n", (unsigned long)a); exit(1); }
    return colram + (a - 0xFF80000UL);
  }
  if (a + n > sizeof chip) { fprintf(stderr, "Chip-RAM ueberlaufen %07lx\n", (unsigned long)a); exit(1); }
  if (a >= 0x20000 && a < 0x40000) { fprintf(stderr, "Schreibzugriff ins ROM %07lx\n", (unsigned long)a); exit(1); }
  return chip + a;
}

unsigned long dma_jobs, dma_bytes;
void dma_copy(uint32_t s, uint32_t d, uint16_t n) {
  uint32_t cnt = n ? n : 65536; dma_jobs++; dma_bytes += cnt;
  memmove(resolve(d, cnt), resolve(s, cnt), cnt);
}
void dma_fill(uint32_t d, uint8_t v, uint16_t n) {
  uint32_t cnt = n ? n : 65536; dma_jobs++; dma_bytes += cnt;
  memset(resolve(d, cnt), v, cnt);
}
void set_color(uint8_t i, uint8_t r, uint8_t g, uint8_t b) { pal[i][0] = r; pal[i][1] = g; pal[i][2] = b; }
void show_buffer(uint8_t b) { shown = b; }
uint16_t raster_line(void) { return 0; }
void wait_vblank(void) {}
void hw_init(void) {}
uint8_t key_col(uint8_t c) { (void)c; return 0; }
uint8_t joy2(void) { return 0; }

fix16 fmul(fix16 a, fix16 b) { return (fix16)(((int64_t)a * b) / 65536); }
int32_t mul32(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
