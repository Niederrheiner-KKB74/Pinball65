/*
 * game.h – Spielregeln Meilenstein 3: Punkte, Baelle, Lampen, Bonus, Meldungen
 */
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include "physics.h"

#define BALLS_PER_GAME 3

extern uint32_t score;
extern uint8_t ball_no, mult, game_over;
extern uint16_t bonus;          /* Bonuspunkte in Tausend, vor Multiplikator */
extern uint8_t ramps;           /* Rampen in diesem Ball */

/* Meldung im Display (Zeile 1 und 2), solange msg_timer > 0 */
extern char msg1[], msg2[];
extern uint8_t msg_timer;

void game_new(void);
/* Vor game_frame: sperrt den Plunger nach Spielende, Plunger startet neu */
void game_input(input_t *in);
/* Nach game_frame: Ereignisse auswerten, Lampen und Klang */
void game_update(const input_t *in);

/* Zahl mit Tausenderpunkten (1.234.560) */
char *fmt_points(char *p, uint32_t v);

#endif
