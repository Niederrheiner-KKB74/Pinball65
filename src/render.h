/*
 * render.h – Tischausschnitt zeichnen (Scrolling, Flipper, Ball)
 */
#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

void render_init(void);
/* Kamera: oberste sichtbare Tischzeile 0 .. TH-VIEW_H, folgt dem Ball */
int16_t camera_update(void);
/* Zeichnet den Ausschnitt ab Tischzeile cam in Puffer b (0 = A, 1 = B) */
void render_frame(uint8_t b, int16_t cam);
/* Messwert: Rasterzeilen fuer das Kopieren des Hintergrunds im letzten Bild */
extern uint16_t render_bg_lines;

#endif
