/* Ersatz fuer <mega65.h> beim PC-Test: nur was der Code braucht */
#ifndef HOST_MEGA65_H
#define HOST_MEGA65_H
#include <stdint.h>
struct host_vic { uint8_t bordercol; };
extern struct host_vic VICIV;
uint32_t host_reg(void *p);
#define CPUADDR(p) host_reg(p)
extern uint8_t host_rowbuf[2560], host_colbuf[1344], host_maskwin[1920];
#define ROWBUF  host_rowbuf
#define COLBUF  host_colbuf
#define MASKWIN host_maskwin
#endif
