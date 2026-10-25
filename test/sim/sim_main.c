/* Spielablauf im 6502-Simulator: Physik, Darstellung und Display mit 16-Bit-ints */
#include <stdio.h>
#include <string.h>
#include "hw.h"
#include "table.h"
#include "tables.h"
#include "physics.h"
#include "render.h"
#include "dmd.h"
#include "game.h"
#include "sound.h"

extern unsigned long sim_jobs;

int main(void) {
  tables_init();
  printf("start\n");
  table_build_mask();
  printf("maske ok\n");
  phys_init();
  render_init();
  snd_init();
  game_new();
  input_t in;
  uint8_t front = 0;
  for (uint16_t frame = 1; frame <= 400; frame++) {
    memset(&in, 0, sizeof in);
    if (frame == 2) in.test = 1;
    if (frame > 150 && (frame % 30) < 6) in.left = in.right = 1;
    game_input(&in);
    game_frame(&in);
    game_update(&in);
    int16_t cam = camera_update();
    uint8_t back = front ^ 1;
    render_frame(back, cam);
    char l1[27], l2[27], *p;
    p = dmd_put(l1, "C"); dmd_num(p, (uint32_t)cam, 3);
    p = dmd_put(l2, "Y"); dmd_num(p, (uint32_t)(ball.y >> 16) & 0x3FF, 3);
    dmd_text(l1, l2);
    dmd_render();
    front = back;
    if (frame % 50 == 0)
      printf("F%u x=%d y=%d cam=%d st=%u jobs=%lu\n", frame, (int)(ball.x >> 16), (int)(ball.y >> 16), cam, state, sim_jobs);
  }
  printf("fertig\n");
  return 0;
}
