/*
 * sim_hw.c – Hardware-Attrappe fuer den 6502-Simulator.
 * Fernspeicher (Chip-RAM ab $10000, Attic, Farb-RAM) wird nicht nachgebildet;
 * geprueft wird jeder DMA-Auftrag, der CPU-Speicher (Bank 0) beschreibt.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hw.h"
#include "fixmath.h"

struct sim_vic VICIV;
uint8_t sim_rowbuf[2560], sim_colbuf[1344], sim_maskwin[1920];
unsigned long sim_jobs;

void sim_fail(const char *what, uint32_t a, uint32_t b) {
  printf("FEHLER: %s a=%lx b=%lx\n", what, (unsigned long)a, (unsigned long)b);
  exit(1);
}

static void check_cpu(uint32_t a, uint16_t n) {
  if (a >= 0x10000UL) return;                      /* Fernspeicher: ignoriert */
  uint16_t p = (uint16_t)a;
  const uint16_t rb = (uint16_t)sim_rowbuf, cb = (uint16_t)sim_colbuf, mb = (uint16_t)sim_maskwin;
  if (n == 0) sim_fail("DMA-Laenge 0 (= 64 KB!) in CPU-Speicher", a, 0);
  if (p >= rb && p + n <= rb + sizeof sim_rowbuf) return;
  if (p >= cb && p + n <= cb + sizeof sim_colbuf) return;
  if (p >= mb && p + n <= mb + sizeof sim_maskwin) return;
  sim_fail("DMA ausserhalb der CPU-Puffer", a, n);
}

/* Farb-RAM-Bereich der Kollisionsmaske nachbilden (nur fuer den Aufbautest) */
#ifdef SIM_COLRAM
#ifndef SIM_MASK_FROM
#define SIM_MASK_FROM 0             /* erste nachgebildete Tischzeile */
#endif
static uint8_t colram_mask[25600 - SIM_MASK_FROM * 40];
static uint8_t sim_dummy[2000];
static uint8_t *far_ptr(uint32_t a, uint16_t n) {
  const uint32_t base = 0xFF81000UL + SIM_MASK_FROM * 40UL;
  if (a >= base && a + n <= base + sizeof colram_mask) return colram_mask + (uint16_t)(a - base);
  if (a >= 0xFF81000UL && a < base && n <= sizeof sim_dummy) return sim_dummy;   /* oberer Teil: leer */
  return 0;
}
#else
static uint8_t *far_ptr(uint32_t a, uint16_t n) { (void)a; (void)n; return 0; }
#endif

void dma_copy(uint32_t s, uint32_t d, uint16_t n) {
  sim_jobs++;
  uint8_t *fs = far_ptr(s, n), *fd = far_ptr(d, n);
  if (fd && s < 0x10000UL) { memcpy(fd, (void *)(uint16_t)s, n); return; }
  if (fs && d < 0x10000UL) { check_cpu(d, n); memcpy((void *)(uint16_t)d, fs, n); return; }
  check_cpu(d, n);
  if (s < 0x10000UL) check_cpu(s, n);
  if (d < 0x10000UL && s < 0x10000UL) memmove((void *)(uint16_t)d, (void *)(uint16_t)s, n);
  else if (d < 0x10000UL) memset((void *)(uint16_t)d, 0, n);   /* aus Fernspeicher: leer */
}
void dma_fill(uint32_t d, uint8_t v, uint16_t n) {
  sim_jobs++;
  check_cpu(d, n);
  if (d < 0x10000UL) memset((void *)(uint16_t)d, v, n);
}
void set_color(uint8_t i, uint8_t r, uint8_t g, uint8_t b) { (void)i; (void)r; (void)g; (void)b; }
void show_buffer(uint8_t b) { (void)b; }
uint16_t raster_line(void) { return 0; }
void wait_vblank(void) {}
void hw_init(void) {}
uint8_t attic_ok(void) { return 1; }
uint8_t key_col(uint8_t c) { (void)c; return 0; }
uint8_t joy2(void) { return 0; }

fix16 fmul(fix16 a, fix16 b) { return fmul_sw(a, b); }
int32_t mul32(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }
void build_progress(uint8_t phase, uint8_t step) { (void)phase; (void)step; }
void frame_mark(void) {}
uint8_t frame_overran(void) { return 0; }
