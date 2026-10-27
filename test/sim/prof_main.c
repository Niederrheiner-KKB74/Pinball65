/* Laufzeitprofil der Physik im 6502-Simulator: Ball im unteren Tischteil,
 * Flipper schlagen regelmaessig. Aufruf ueber make prof. */
#include <stdio.h>
#include <string.h>
#include "hw.h"
#include "table.h"
#include "tables.h"
#include "physics.h"
#include "render.h"
#include "game.h"
#include "sound.h"
#ifndef PROF_FRAMES
#define PROF_FRAMES 300
#endif

static uint32_t cyc(void) {
  volatile uint8_t *c = (uint8_t *)0xFFF0;
  return c[0] | ((uint32_t)c[1] << 8) | ((uint32_t)c[2] << 16) | ((uint32_t)c[3] << 24);
}
extern uint32_t fmul_cycles, fmul_calls, mul32_calls;

int main(void) {
  tables_init();
  table_build_mask();
  phys_init();
  game_new();
  state = ST_PLAY;
  in_lane_reset();
  ball.x = 150L << 16; ball.y = 470L << 16; ball.vx = 20000; ball.vy = 0;
#ifdef PROF_MULTI                 /* make prof CFLAGS=-DPROF_MULTI: zweiter Ball */
  phys_multiball(); ball2.b.x = 200L << 16; ball2.b.y = 450L << 16; ball2.b.vy = 0; ball2.inl = 0;
#endif
  input_t in;
  uint32_t total = 0, worst = 0, fm0, fc0, mc0;
  for (uint16_t f = 0; f < PROF_FRAMES; f++) {
    memset(&in, 0, sizeof in);
    if ((f % 40) < 8) in.left = 1;
    if ((f % 40) >= 20 && (f % 40) < 28) in.right = 1;
    fm0 = fmul_cycles; fc0 = fmul_calls; mc0 = mul32_calls;
    uint32_t t0 = cyc();
    game_frame(&in);
    uint32_t t = cyc() - t0;
    /* Software-Multiplikation herausrechnen, Hardware: etwa 70 Zyklen je Aufruf */
    t = t - (fmul_cycles - fm0) + (fmul_calls - fc0 + mul32_calls - mc0) * 70;
    total += t;
    if (t > worst) worst = t;
    if (state != ST_PLAY) { phys_init(); state = ST_PLAY; in_lane_reset(); ball.x = 150L << 16; ball.y = 470L << 16; }
#ifdef PROF_MULTI
    if (!multi) { phys_multiball(); ball2.b.x = 200L << 16; ball2.b.y = 450L << 16; ball2.b.vy = 0; ball2.inl = 0; }
#endif
  }
  printf("Physik: Mittel %lu, hoechstens %lu Zyklen pro Bild (ohne Software-Multiplikation)\n",
         (unsigned long)(total / (PROF_FRAMES ? PROF_FRAMES : 1)), (unsigned long)worst);
  return 0;
}
