/* Automatisch erzeugt von tools/gen_ramp.py - nicht von Hand aendern */
#ifndef RAMP_PATH_H
#define RAMP_PATH_H
#include <stdint.h>

/* v6.4 kompakt: x0, y0 in 12.4, ux, uy in 2.14, s0 in 16.16 (spart 392 Byte) */
typedef struct { int16_t x0, y0, ux, uy; int32_t s0; } ramp_seg_t;
#define RS_P(v) ((int32_t)(v) << 12)   /* Lage nach 16.16 */
#define RS_U(v) ((int32_t)(v) << 2)    /* Richtung nach 16.16 */
#define RAMP_NSEG       49
#define RAMP_LEN        39420285L   /* 601.5 Pixel */
#define RAMP_CLIMB_END  9175040L
#define RAMP_DESC_START 30900605L   /* 471.5: rechts abwaerts */
#define RAMP_X0         128
#define RAMP_Y0         300   /* Einfahrt: Ball kreuzt diese Zeile nach oben */
#define RAMP_EXIT_X     17170432L   /* 262.0 */
#define RAMP_EXIT_Y     16515072L   /* 252.0 */
extern const ramp_seg_t RAMP_SEGS[RAMP_NSEG];

#endif
