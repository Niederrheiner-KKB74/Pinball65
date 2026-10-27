/* Automatisch erzeugt von tools/gen_tables.py - nicht von Hand aendern */
#ifndef TABLES_H
#define TABLES_H
#include <stdint.h>

int32_t sin_tab(uint8_t i);          /* sin(i*2pi/256) in 16.16 */
extern const uint8_t ATAN_TAB[65];   /* atan(i/64) in 256er-Winkeln */
void tables_init(void);              /* vor allem anderen aufrufen */

#endif
