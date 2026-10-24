/*
 * host_main.c – Fuehrt den Spielablauf auf dem PC aus und schreibt Bilder.
 *
 * Bildschirmbilder werden wie vom VIC-IV gelesen: Bildschirmkarte -> Zeichen-
 * nummer -> 64 Byte Zeichendaten. Dazu ein Bild des ganzen Tisches aus den
 * Spaltenstreifen im (simulierten) Attic-RAM.
 */
#include <stdio.h>
#include <string.h>
#include "hw.h"
#include "table.h"
#include "physics.h"
#include "render.h"
#include "dmd.h"

extern uint8_t chip[], attic[];
extern uint8_t pal[256][3];
extern unsigned long dma_jobs, dma_bytes;

static void dump_screen(const char *name, uint8_t b) {
  FILE *f = fopen(name, "wb");
  fprintf(f, "P6 320 200 255\n");
  const uint8_t *map = chip + (b ? MAP_B : MAP_A);
  for (int y = 0; y < 200; y++)
    for (int x = 0; x < 320; x++) {
      int cell = (y >> 3) * 40 + (x >> 3);
      uint32_t ch = map[cell * 2] | (map[cell * 2 + 1] << 8);
      uint8_t v = chip[ch * 64 + (y & 7) * 8 + (x & 7)];
      fwrite(pal[v], 1, 3, f);
    }
  fclose(f);
}

static void dump_table(const char *name) {
  FILE *f = fopen(name, "wb");
  fprintf(f, "P6 320 640 255\n");
  for (int y = 0; y < 640; y++)
    for (int x = 0; x < 320; x++)
      fwrite(pal[attic[(x >> 3) * STRIP_BYTES + y * 8 + (x & 7)]], 1, 3, f);
  fclose(f);
}

int main(void) {
  hw_init();
  table_build_palette();
  dma_fill(FB_A, C_BG, 40 * COL_BYTES);
  dma_fill(FB_B, C_BG, 40 * COL_BYTES);
  dmd_text("SEARCHING FOR *", "LOADING");
  dmd_render();
  table_build_mask();
  table_build_art();
  render_init();
  phys_init();
  dump_table("table_full.ppm");

  input_t in;
  uint8_t front = 0;
  for (uint16_t frame = 1; frame <= 900; frame++) {
    memset(&in, 0, sizeof in);
    if (frame == 2) in.test = 1;                         /* Testabschuss */
    if (frame > 400 && frame < 420) in.left = in.right = 1;
    if (frame > 600 && (frame % 40) < 8) in.left = 1;
    if (frame > 600 && (frame % 40) >= 20 && (frame % 40) < 28) in.right = 1;
    game_frame(&in);
    int16_t cam = camera_update();
    uint8_t back = front ^ 1;
    unsigned long j0 = dma_jobs, b0 = dma_bytes;
    render_frame(back, cam);
    char l1[27], l2[27], *p;
    p = dmd_put(l1, "TEST "); dmd_num(p, test_count, 3);
    p = dmd_put(l2, "C"); dmd_num(p, (uint32_t)cam, 3);
    dmd_text(l1, l2);
    dmd_render();
    front = back;
    if (frame % 25 == 0 || frame == 3)
      printf("F%4u st=%u x=%7.2f y=%7.2f cam=%3d bump=%u sling=%u dma=%lu/%lu\n",
             frame, state, ball.x / 65536.0, ball.y / 65536.0, cam,
             bumper_hits, sling_hits, dma_jobs - j0, dma_bytes - b0);
    if (frame == TEST_FRAMES + 2 && snap_valid) printf("SNAPSHOT x=%ld y=%ld\n", (long)snap_x, (long)snap_y);
    if (frame == 3) dump_screen("frame_start.ppm", front);
    if (frame == 30) dump_screen("frame_top.ppm", front);
    if (frame == 410) dump_screen("frame_flippers_up.ppm", front);
    if (frame == 110) dump_screen("frame_play.ppm", front);
  }
  return 0;
}
void build_progress(uint8_t phase, uint8_t step) { (void)phase; (void)step; }
