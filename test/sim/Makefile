# 6502-Simulator-Test: Physik, Darstellung und Display mit dem echten
# llvm-mos-Compiler (16-Bit-int), DMA-Zugriffe in CPU-Speicher werden geprueft.
CC  := mos-sim-clang
SRC := sim_main.c sim_hw.c ../../src/table.c ../../src/physics.c ../../src/render.c \
       ../../src/tables.c ../../src/dmd.c ../../src/game.c ../../src/sound.c ../../src/sfx_steps.c ../../src/ramp_path.c ../../src/font5x7.c ../../src/music.c ../../src/hiscore.c ../../src/music_data.c
simtest: $(SRC) $(wildcard ../../src/*.h) mega65.h
	$(CC) -Oz -fno-unroll-loops -DPB_BUILDER -I . -I ../../src -include mega65.h -o $@ $(SRC)
run: simtest
	mos-sim simtest
clean:
	rm -f simtest simtest.elf
# Laufzeitprofil der Physik (Kollisionsmaske ab Tischzeile 400 nachgebildet)
PROF_SRC := prof_main.c $(filter-out sim_main.c,$(SRC))
proftest: $(PROF_SRC) $(wildcard ../../src/*.h) mega65.h
	$(CC) -Oz -fno-unroll-loops -DSIM_COLRAM -DSIM_MASK_FROM=400 -DPB_BUILDER -I . -I ../../src -include mega65.h -o $@ $(PROF_SRC)
prof: proftest
	mos-sim --profile proftest 2> profile.txt
	python3 ../../tools/sim_profile.py proftest.elf profile.txt | head -30
.PHONY: run clean prof
