/* Automatisch erzeugt von tools/gen_music.py - nicht von Hand aendern */
#ifndef MUSIC_DATA_H
#define MUSIC_DATA_H
#include <stdint.h>

enum { MUS_GAME, MUS_TITLE, MUS_FANFARE, MUS_MULTI, MUS_OVER, MUS_COUNT };
typedef struct { uint8_t tempo, loop; const uint8_t *order[3]; } song_t;
typedef struct { uint8_t wave, ad, sr; uint16_t pw; uint8_t sweep, fx, arg; } instr_t;
extern const song_t SONGS[MUS_COUNT];
extern const instr_t INSTR[];
extern const uint8_t MUS_PAT[];
extern const uint16_t MUS_OFS[];
extern const uint16_t NOTE6[12];

#endif
