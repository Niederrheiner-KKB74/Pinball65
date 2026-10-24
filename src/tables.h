/* Automatisch erzeugt von tools/gen_tables.py - nicht von Hand aendern */
#ifndef TABLES_H
#define TABLES_H
#include <stdint.h>

extern int32_t SIN_TAB[256];         /* sin(i*2pi/256) in 16.16, aus tables_init() */
extern const uint8_t ATAN_TAB[65];   /* atan(i/64) in 256er-Winkeln */
void tables_init(void);              /* vor allem anderen aufrufen */

#endif
