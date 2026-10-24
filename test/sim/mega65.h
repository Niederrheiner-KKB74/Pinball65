/* Ersatz fuer <mega65.h> im 6502-Simulator (mos-sim): echte 16-Bit-ints */
#ifndef SIM_MEGA65_H
#define SIM_MEGA65_H
#include <stdint.h>
struct sim_vic { uint8_t bordercol; };
extern struct sim_vic VICIV;
extern uint8_t sim_rowbuf[2560], sim_colbuf[1344], sim_maskwin[1920];
#define ROWBUF  sim_rowbuf
#define COLBUF  sim_colbuf
#define MASKWIN sim_maskwin
#define PB_CHECK 1
void sim_fail(const char *what, uint32_t a, uint32_t b);
#endif
