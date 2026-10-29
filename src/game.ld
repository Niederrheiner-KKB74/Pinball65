/* Pinball65 – Linker-Skript fuer das Spiel PB65GAME (v7.5)
 *
 * Wie das llvm-mos-Skript fuer den MEGA65, aber die Variablen ohne Startwert
 * (.bss, .noinit) liegen in $0200-$07FF. Diesen Bereich nutzt sonst nur das
 * KERNAL; das Spiel ruft es nur zum Speichern des Highscores auf und stellt
 * den Bereich dafuer aus dem Attic-RAM wieder her (hiscore.c). So bleibt
 * $2001-$CFFF ganz fuer Programm, Konstanten und Startwerte.
 */
__basic_zp_start = 0x0002;
__basic_zp_end = 0x0090;

MEMORY {
    ram (rw)   : ORIGIN = 0x2001, LENGTH = 0xAFFF
    lowram (rw) : ORIGIN = 0x0200, LENGTH = 0x0600
}

__rc0 = __basic_zp_start;
INCLUDE imag-regs.ld
__basic_zp_size = __basic_zp_end - __basic_zp_start;
MEMORY { zp : ORIGIN = __rc31 + 1, LENGTH = __basic_zp_end - (__rc31 + 1) }

INPUT(basic-header.o)
INPUT(unmap-basic.o)

REGION_ALIAS("c_readonly", ram)
REGION_ALIAS("c_writeable", ram)

SECTIONS {
    .basic_header : { *(.basic_header) }
    INCLUDE zp.ld
    INCLUDE text.ld
    INCLUDE rodata.ld
    INCLUDE data.ld
    .bss (NOLOAD) : { INCLUDE bss-sections.ld } >lowram
    INCLUDE bss-symbols.ld
    .noinit (NOLOAD) : { INCLUDE noinit-sections.ld } >lowram
}

/* Soft-Stapel von $D000 abwaerts (gemessen etwa 64 Byte) */
__stack = 0xd000;

OUTPUT_FORMAT {
    SHORT(0x2001)
    TRIM(ram)
}
