/*
 * Pinball65 – Meilenstein 1: Ball und Flipper auf einem statischen Tisch
 *
 * - Hardware wird komplett uebernommen (kein KERNAL nach dem Start)
 * - Full-Colour-Charakter-Modus, zwei Bildpuffer (kein Flackern)
 * - Physik aus dem Browser-Prototyp, 8 Substeps pro Bild, 50 Hz
 * - Fester Ausschnitt: untere 192 Zeilen des Tisches
 *
 * Tasten: Shift links / A = linker Flipper, Shift rechts / L = rechter Flipper,
 * Leertaste halten = Plunger, T = Testabschuss, B = Ball zurueck.
 * Joystick Port 2: links/rechts = Flipper, Feuer oder unten = Plunger.
 */
#include <mega65.h>
#include "hw.h"
#include "table.h"
#include "physics.h"
#include "render.h"

static void read_input(input_t *in) {
  static uint8_t prev_t, prev_b;
  uint8_t c1 = key_col(1), c2 = key_col(2), c3 = key_col(3);
  uint8_t c5 = key_col(5), c6 = key_col(6), c7 = key_col(7);
  uint8_t j = joy2();
  in->left = (c1 & 0x80) || (c1 & 0x04) || (j & 0x04);      /* Shift links, A */
  in->right = (c6 & 0x10) || (c5 & 0x04) || (j & 0x08);     /* Shift rechts, L */
  in->plunger = (c7 & 0x10) || (j & 0x10) || (j & 0x02);    /* Leertaste */
  uint8_t t = (c2 & 0x40) != 0, b = (c3 & 0x10) != 0;       /* T, B */
  in->test = t && !prev_t;
  in->reset = b && !prev_b;
  prev_t = t; prev_b = b;
}

int main(void) {
  hw_init();
  table_build_palette();
  dma_fill(FB_A, C_BG, 64000);          /* dunkler Bildschirm waehrend der Berechnung */
  show_buffer(0);

  table_build_mask();
  table_build_art(rowbuf);
  render_init();
  phys_init();

  uint8_t front = 0;
  uint16_t frame = 0, load = 0;
  input_t in;
  for (;;) {
    wait_vblank();
    show_buffer(front);                 /* fertig gezeichneten Puffer zeigen */
    frame++;
    read_input(&in);
    game_frame(&in);
    uint8_t back = front ^ 1;
    render_frame(back, frame, load);
    front = back;
    uint16_t l = raster_line();         /* Auslastung: Rasterzeilen seit Zeile 251 */
    load = l >= 251 ? l - 251 : l + 312 - 251;
  }
}
