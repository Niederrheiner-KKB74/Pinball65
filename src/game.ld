/* Pinball65 – Linker-Skript fuer das Spiel PB65GAME (v7.7)
 *
 * Wie das llvm-mos-Skript fuer den MEGA65, mit einem zweiten Bereich
 * $E000-$FFF9 im RAM unter dem KERNAL:
 * - $2001-$CFFF: Programm und Startwerte (.text, .data); nur das steht in der
 *   Datei PB65GAME.
 * - $E000-$FFF9: Konstanten (.rodata) und Variablen ohne Startwert (.bss,
 *   .noinit). Die Konstanten zieht das Makefile aus der ELF-Datei, der Lader
 *   bringt sie mit und legt sie im Attic-RAM ab (ATTIC_RODATA). main() blendet
 *   das KERNAL aus, holt die Konstanten und loescht die Variablen (himem_init).
 *   Der Startcode loescht .bss deshalb nicht (Groesse 0): er ruft danach noch
 *   CHROUT auf, und dafuer muss das KERNAL bei $E000 noch eingeblendet sein.
 * - $FFFA-$FFFF: Sprungvektoren (NMI und IRQ zeigen im Spiel auf ein RTI).
 */
__basic_zp_start = 0x0002;
__basic_zp_end = 0x0090;

MEMORY {
    ram (rw)   : ORIGIN = 0x2001, LENGTH = 0xAFFF
    hiram (rw) : ORIGIN = 0xE000, LENGTH = 0x1FFA
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
    /* v8.0: Ausstieg zu BASIC; muss unter $A000 liegen (BASIC-ROM wird eingeblendet) */
    .lowtext : { *(.lowtext) } >ram
    INCLUDE text.ld
    INCLUDE data.ld
    /* v8.0: dazu selten gebrauchter Code (.hitext, Makro HICODE in hw.h) */
    .rodata : { INCLUDE rodata-sections.ld *(.hitext .hitext.*) } >hiram
    __rodata_start = ADDR(.rodata);
    __rodata_size = SIZEOF(.rodata);
    .bss (NOLOAD) : { INCLUDE bss-sections.ld } >hiram
    __bss_size = 0;
    .noinit (NOLOAD) : { INCLUDE noinit-sections.ld } >hiram
    __hibss_start = ADDR(.bss);
    __hibss_size = ADDR(.noinit) + SIZEOF(.noinit) - ADDR(.bss);
}

/* Soft-Stapel von $D000 abwaerts (gemessen etwa 64 Byte) */
__stack = 0xd000;

OUTPUT_FORMAT {
    SHORT(0x2001)
    TRIM(ram)
}
