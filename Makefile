# Pinball65 – Build mit llvm-mos (Linux, macOS oder Windows mit make)
CC      := mos-mega65-clang
PYTHON  ?= python3
CFLAGS  := -Os -Wall -Wextra
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
