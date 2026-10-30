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
#include "hw.h"

enum { SND_BUMPER, SND_SLING, SND_TARGET, SND_LANE, SND_FLIPPER, SND_DRAIN,
       SND_COMPLETE, SND_START, SND_SPIN, SND_DRIVE, SND_RAMP, SND_CLICK, SND_WHOOSH, SND_COUNT };

/* Ein Schritt eines Klangeffekts */
typedef struct {
  uint8_t wave;       /* Steuerregister ohne Gate: $10 Dreieck, $20 Saege, $40 Puls, $80 Rauschen; 0 = Ende */
  uint8_t ad, sr;
  uint16_t freq;
  int16_t step;       /* Aenderung pro Bild */
  uint8_t frames;
} fxstep_t;
#define N_STEPS 39                  /* Anzahl der Schritte in sfx_steps.c */
extern const fxstep_t STEPS[N_STEPS];

void snd_init(void);
void snd_play(uint8_t fx);
/* v7.7: leiser Effekt (Klick, Spinner): nur auf einer freien Stimme, sonst gar nicht */
void snd_soft(uint8_t fx);
/* Einmal pro Bild: Frequenzen nachfuehren, abgelaufene Effekte beenden */
void snd_update(void);

#endif
