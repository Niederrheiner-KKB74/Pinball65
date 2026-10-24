# 6502-Simulator-Test: Physik, Darstellung und Display mit dem echten
# llvm-mos-Compiler (16-Bit-int), DMA-Zugriffe in CPU-Speicher werden geprueft.
CC  := mos-sim-clang
SRC := sim_main.c sim_hw.c ../../src/table.c ../../src/physics.c ../../src/render.c \
       ../../src/tables.c ../../src/dmd.c ../../src/font5x7.c
simtest: $(SRC) $(wildcard ../../src/*.h) mega65.h
	$(CC) -Oz -I . -I ../../src -include mega65.h -o $@ $(SRC)
run: simtest
	mos-sim simtest
clean:
	rm -f simtest simtest.elf
.PHONY: run clean
