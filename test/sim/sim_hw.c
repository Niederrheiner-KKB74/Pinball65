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
#include "table.h"

struct sim_vic VICIV;
uint8_t sim_rowbuf[2560], sim_colbuf[1344], sim_maskwin[1920], sim_clswin[280];
unsigned long sim_jobs;

/* v8.2: kleine Ausgabe statt printf (spart rund 3 KB, der Test passte sonst nicht mehr) */
void sim_puts(const char *s) { while (*s) putchar(*s++); }
void sim_num(const char *pre, long v) {
  char b[12]; int n = 0; unsigned long u;
  sim_puts(pre);
  if (v < 0) { putchar('-'); u = (unsigned long)-v; } else u = (unsigned long)v;
  do { b[n++] = (char)('0' + u % 10); u /= 10; } while (u);
  while (n) putchar(b[--n]);
}

void sim_fail(const char *what, uint32_t a, uint32_t b) {
  sim_puts("FEHLER: "); sim_puts(what); sim_num(" a=", (long)a); sim_num(" b=", (long)b); sim_puts("\n");
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
  if (p >= (uint16_t)sim_clswin && p + n <= (uint16_t)sim_clswin + sizeof sim_clswin) return;
  sim_fail("DMA ausserhalb der CPU-Puffer", a, n);
}

/* Farb-RAM-Bereich der Kollisionsmaske nachbilden (nur fuer den Aufbautest) */
#ifdef SIM_COLRAM
#ifndef SIM_MASK_FROM
#define SIM_MASK_FROM 0             /* erste nachgebildete Tischzeile */
#endif
static uint8_t colram_mask[25600 - SIM_MASK_FROM * 40];
static uint8_t sim_dummy[2000];
static uint8_t cls_mem[3200];
static uint8_t *far_ptr_mask(uint32_t a, uint16_t n) {
  if (a >= CLS_FAR && a + n <= CLS_FAR + sizeof cls_mem) return cls_mem + (uint16_t)(a - CLS_FAR);
  const uint32_t base = MASK_FAR + SIM_MASK_FROM * 40UL;
  if (a >= base && a + n <= base + sizeof colram_mask) return colram_mask + (uint16_t)(a - base);
  if (a >= MASK_FAR && a < base && n <= sizeof sim_dummy) return sim_dummy;   /* oberer Teil: leer */
  return 0;
}
#else
static uint8_t *far_ptr_mask(uint32_t a, uint16_t n) { (void)a; (void)n; return 0; }
#endif
/* Flipper-Spannen im Attic-RAM (render.c, 25 Phasen x 192 Byte) */
static uint8_t span_mem[25 * 192 + 50];
static uint8_t *span_ptr(uint32_t a, uint16_t n) {
  const uint32_t base = ATTIC_SPR + 0x38000UL;
  if (a >= base && a + n <= base + sizeof span_mem) return span_mem + (uint16_t)(a - base);
  return 0;
}
static uint8_t *far_ptr(uint32_t a, uint16_t n) {
  uint8_t *p = span_ptr(a, n);
  return p ? p : far_ptr_mask(a, n);
}

void dma_copy(uint32_t s, uint32_t d, uint16_t n) {
  sim_jobs++;
  uint8_t *fs = far_ptr(s, n), *fd = far_ptr(d, n);
  if (fd && s < 0x10000UL) { memcpy(fd, (void *)(uint16_t)s, n); return; }
  if (fs && d < 0x10000UL) { if (!span_ptr(s, n)) check_cpu(d, n); memcpy((void *)(uint16_t)d, fs, n); return; }
  check_cpu(d, n);
  if (s < 0x10000UL) check_cpu(s, n);
  if (d < 0x10000UL && s < 0x10000UL) memmove((void *)(uint16_t)d, (void *)(uint16_t)s, n);
  else if (d < 0x10000UL) memset((void *)(uint16_t)d, 0, n);   /* aus Fernspeicher: leer */
}
void dma_copy_trans(uint32_t s, uint32_t d, uint16_t n) { dma_copy(s, d, n); }
uint8_t dma_trans_ok(void) { return 0; }
void sid_poke(uint8_t r, uint8_t v) { (void)r; (void)v; }   /* Simulator: CPU-Weg pruefen */
void sid2_poke(uint8_t r, uint8_t v) { (void)r; (void)v; }
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
void hw_video_init(void) {}
uint8_t attic_ok(void) { return 1; }
uint8_t key_col(uint8_t c) { (void)c; return 0; }
uint8_t joy2(void) { return 0; }

/* Zaehler fuer das Laufzeitprofil (prof_main.c): Zeit in der Software-
 * Multiplikation, die auf dem MEGA65 die Hardware erledigt */
uint32_t fmul_cycles, fmul_calls, mul32_calls;
static uint32_t cyc(void) {
  volatile uint8_t *c = (uint8_t *)0xFFF0;
  return c[0] | ((uint32_t)c[1] << 8) | ((uint32_t)c[2] << 16) | ((uint32_t)c[3] << 24);
}
fix16 fmul(fix16 a, fix16 b) {
  uint32_t t = cyc(); fix16 r = fmul_sw(a, b); fmul_cycles += cyc() - t; fmul_calls++; return r;
}
int32_t mul32(int32_t a, int32_t b) {
  uint32_t t = cyc(); int32_t r = (int32_t)((uint32_t)a * (uint32_t)b); fmul_cycles += cyc() - t; mul32_calls++; return r;
}
void build_progress(uint8_t phase, uint8_t step) { (void)phase; (void)step; }
uint16_t raster_diff(uint16_t a, uint16_t b) { return b >= a ? b - a : b + 312 - a; }
