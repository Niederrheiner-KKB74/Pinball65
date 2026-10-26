/* Automatisch erzeugt von tools/gen_ramp.py - nicht von Hand aendern */
#ifndef RAMP_PATH_H
#define RAMP_PATH_H
#include <stdint.h>

typedef struct { int32_t x0, y0, ux, uy, s0; } ramp_seg_t;   /* 16.16 */
#define RAMP_NSEG       42
#define RAMP_LEN        35321650L   /* 539.0 Pixel */
#define RAMP_CLIMB_END  9175040L
#define RAMP_DESC_START 26801970L   /* 409.0: rechts abwaerts */
#define RAMP_X0         61
#define RAMP_Y0         275   /* Einfahrt: Ball kreuzt diese Zeile nach oben */
#define RAMP_EXIT_X     17170432L   /* 262.0 */
#define RAMP_EXIT_Y     16515072L   /* 252.0 */
extern const ramp_seg_t RAMP_SEGS[RAMP_NSEG];

#endif
