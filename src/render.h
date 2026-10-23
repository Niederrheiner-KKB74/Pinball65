/*
 * render.h – Zeichnen in den hinteren Bildpuffer (Double-Buffering)
 */
#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

extern uint8_t rowbuf[];

void render_init(void);
/* Zeichnet Flipper, Ball und Statuszeile in Puffer b (0 = A, 1 = B) */
void render_frame(uint8_t b, uint16_t frame, uint16_t load);

#endif
