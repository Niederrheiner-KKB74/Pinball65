/*
 * host_main.c – Fuehrt Meilenstein 1 auf dem PC aus und schreibt Bilder,
 * die aus dem simulierten FCM-Speicher so gelesen werden wie vom VIC-IV.
 */
#include <stdio.h>
#include <string.h>
#include "hw.h"
#include "table.h"
#include "physics.h"
#include "render.h"

extern uint8_t chip[];
extern uint8_t pal[256][3];
extern unsigned long dma_jobs, dma_bytes;

static void dump(const char *name, uint8_t b) {
  FILE *f = fopen(name, "wb");
  fprintf(f, "P6 320 200 255\n");
  uint32_t base = b ? 0x1400 : 0x1000;        /* Zeichennummer wie in SCREEN_A/B */
  for (int y = 0; y < 200; y++)
    for (int x = 0; x < 320; x++) {
      uint32_t ch = base + (y >> 3) * 40 + (x >> 3);
      uint8_t v = chip[ch * 64 + (y & 7) * 8 + (x & 7)];
      fwrite(pal[v], 1, 3, f);
    }
  fclose(f);
}

int main(void) {
  table_build_palette();
  dma_fill(FB_A, C_BG, 64000);
  table_build_mask();
  table_build_art(rowbuf);
  render_init();
  phys_init();
  dump("frame_start.ppm", 0);

  input_t in; memset(&in, 0, sizeof in);
  uint8_t front = 0;
  for (uint16_t frame = 1; frame <= 900; frame++) {
    memset(&in, 0, sizeof in);
    if (frame == 2) in.test = 1;                         /* Testabschuss */
    if (frame > 400 && frame < 420) in.left = in.right = 1;
    if (frame > 600 && (frame % 40) < 8) in.left = 1;
    if (frame > 600 && (frame % 40) >= 20 && (frame % 40) < 28) in.right = 1;
    game_frame(&in);
    uint8_t back = front ^ 1;
    unsigned long j0 = dma_jobs, b0 = dma_bytes;
    render_frame(back, frame, 0);
    front = back;
    if (frame % 25 == 0 || frame == 3)
      printf("F%4u st=%u x=%7.2f y=%7.2f vx=%6.3f vy=%6.3f bump=%u sling=%u dma=%lu/%lu\n",
             frame, state, ball.x / 65536.0, ball.y / 65536.0, ball.vx / 65536.0, ball.vy / 65536.0,
             bumper_hits, sling_hits, dma_jobs - j0, dma_bytes - b0);
    if (frame == 122 && snap_valid) printf("SNAPSHOT x=%ld y=%ld\n", (long)snap_x, (long)snap_y);
    if (frame == 410) dump("frame_flippers_up.ppm", front);
    if (frame == 110) dump("frame_play.ppm", front);
    if (frame == 900) dump("frame_end.ppm", front);
    for (int i = 0; i < 2 && 0; i++);
  }
  return 0;
}
