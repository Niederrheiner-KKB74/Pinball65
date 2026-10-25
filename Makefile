# Pinball65 – Build mit llvm-mos (Linux, macOS oder Windows mit make)
CC      := mos-mega65-clang
PYTHON  ?= python3
# -mcpu=mos6502: llvm-mos erzeugte fuer den 45GS02 fehlerhafte INW-Befehle
# (Endlosschleife in band_disc, Version 2.1/2.2). Reiner 6502-Code laeuft auf dem
# 45GS02 unveraendert und entspricht dem, was der Simulator-Test prueft.
CFLAGS  := -Oz -mcpu=mos6502 -Wall -Wextra
SRC     := $(wildcard src/*.c)
HDR     := $(wildcard src/*.h)
# Spiel: alles ausser dem Lader
GAME_SRC := $(filter-out src/boot.c,$(SRC))
# Lader: baut die Tischdaten auf und laedt dann das Spiel nach
BOOT_SRC := src/boot.c src/table.c src/render.c src/physics.c src/hw.c src/dmd.c \
            src/font5x7.c src/tables.c src/fixmath.c src/ramp_path.c
NM      := llvm-nm

all: build/pinball65.d81

build/pb65game.prg: $(GAME_SRC) $(HDR)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $(GAME_SRC)

# Der Lader springt nach dem Laden zur Startadresse des Spiels (_start)
build/pinball65.prg: $(BOOT_SRC) $(HDR) build/pb65game.prg
	$(CC) $(CFLAGS) -DPB_BUILDER \
	  -DGAME_ENTRY=0x$$($(NM) build/pb65game.prg.elf | awk '$$3 == "_start" { print $$1 }') \
	  -o $@ $(BOOT_SRC)

build/loadtest.prg: test/loadtest/loadtest.c test/loadtest/pattern.c
	@mkdir -p build
	$(CC) -Os -mcpu=mos6502 -o $@ $^

build/pinball65.d81: build/pinball65.prg build/pb65game.prg build/loadtest.prg tools/mkd81.py
	$(PYTHON) tools/mkd81.py $@ PINBALL65 build/pinball65.prg build/pb65game.prg build/loadtest.prg

clean:
	rm -rf build

.PHONY: all clean
