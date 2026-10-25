/*
 * sound.h – einfache Klangeffekte mit dem SID
 *
 * Drei Stimmen, jede spielt hoechstens einen Effekt: Startfrequenz,
 * Frequenzaenderung pro Bild, Dauer in Bildern, Wellenform und Huellkurve.
 * Geschrieben wird gleichzeitig in den linken und rechten SID des MEGA65.
 */
#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

enum { SND_BUMPER, SND_SLING, SND_TARGET, SND_LANE, SND_FLIPPER, SND_DRAIN,
       SND_COMPLETE, SND_START, SND_SPIN, SND_DRIVE, SND_RAMP, SND_COUNT };

void snd_init(void);
void snd_play(uint8_t fx);
/* Einmal pro Bild: Frequenzen nachfuehren, abgelaufene Effekte beenden */
void snd_update(void);

#endif
