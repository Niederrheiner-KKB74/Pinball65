/* Ersatz fuer <mega65.h> beim PC-Test: nur was table.c braucht */
#ifndef HOST_MEGA65_H
#define HOST_MEGA65_H
#include <stdint.h>
struct host_vic { uint8_t bordercol; };
extern struct host_vic VICIV;
uint32_t host_reg(void *p);
#define CPUADDR(p) host_reg(p)
#endif
