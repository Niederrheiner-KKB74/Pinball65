# PC-Test: derselbe C-Code wie auf dem MEGA65, Hardware simuliert
CC ?= gcc
SRC := host_main.c host_hw.c ../../src/table.c ../../src/physics.c ../../src/render.c \
       ../../src/tables.c ../../src/dmd.c ../../src/game.c ../../src/sound.c ../../src/ramp_path.c ../../src/font5x7.c ../../src/music.c ../../src/hiscore.c ../../src/music_data.c
hosttest: $(SRC) $(wildcard ../../src/*.h) mega65.h
	$(CC) -O1 -Wall -Wextra -DPB_BUILDER -I . -I ../../src -include mega65.h -o $@ $(SRC)
run: hosttest
	./hosttest
clean:
	rm -f hosttest *.ppm *.png
.PHONY: run clean
