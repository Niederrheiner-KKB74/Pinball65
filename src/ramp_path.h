/* Automatisch erzeugt von tools/gen_ramp.py - nicht von Hand aendern */
#ifndef RAMP_PATH_H
#define RAMP_PATH_H
#include <stdint.h>

typedef struct { int32_t x0, y0, ux, uy, s0; } ramp_seg_t;   /* 16.16 */
#define RAMP_NSEG       50
#define RAMP_LEN        39083634L   /* 596.4 Pixel */
#define RAMP_CLIMB_END  9175040L
#define RAMP_DESC_START 25360178L   /* 387.0: rechts abwaerts */
#define RAMP_X0         61
#define RAMP_Y0         275   /* Einfahrt: Ball kreuzt diese Zeile nach oben */
#define RAMP_EXIT_X     14960838L   /* 228.3 */
#define RAMP_EXIT_Y     21514438L   /* 328.3 */
extern const ramp_seg_t RAMP_SEGS[RAMP_NSEG];

#endif
