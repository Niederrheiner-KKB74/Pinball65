/*
 * music.h – SID-Musik auf dem zweiten SID-Paar (v6.9)
 */
#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>
#include "music_data.h"

#define MUSIC_VOL 0x0A              /* leiser als die Effekte (0x0F) */

void music_init(void);
/* Stueck starten: Lieder mit Wiederholung werden Hintergrund, Jingles
 * unterbrechen ihn und danach beginnt der Hintergrund von vorn */
void music_play(uint8_t song);
void music_mute(uint8_t on);
uint8_t music_muted(void);
/* Einmal pro Bild */
void music_update(void);

#endif
