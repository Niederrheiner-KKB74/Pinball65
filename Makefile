# Pinball65 – Build mit llvm-mos (Linux, macOS oder Windows mit make)
CC      := mos-mega65-clang
PYTHON  ?= python3
# -mcpu=mos6502: llvm-mos erzeugte fuer den 45GS02 fehlerhafte INW-Befehle
# (Endlosschleife in band_disc, Version 2.1/2.2). Reiner 6502-Code laeuft auf dem
# 45GS02 unveraendert und entspricht dem, was der Simulator-Test prueft.
CFLAGS  := -Oz -mcpu=mos6502 -Wall -Wextra
SRC     := $(wildcard src/*.c)
HDR     := $(wildcard src/*.h)

all: build/pinball65.d81

build/pinball65.prg: $(SRC) $(HDR)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $(SRC)

build/pinball65.d81: build/pinball65.prg tools/mkd81.py
	$(PYTHON) tools/mkd81.py $@ PINBALL65 $<

clean:
	rm -rf build

.PHONY: all clean
