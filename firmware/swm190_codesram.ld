/*
 * SWM190, MEM_CONFIG = 2 (cache off).
 * The boot ROM copies __image_size bytes from serial flash (0x41000000) into the 32 KB
 * code SRAM at 0x00000000 and runs from there. Data RAM is 4 KB at 0x20000000.
 * Everything that must survive (code, rodata, .data initialisers) lives in CODE and is
 * covered by __image_size, which goes into vector slot 0x24.
 */
ENTRY(Reset_Handler)

MEMORY
{
  CODE (rx)  : ORIGIN = 0x00000000, LENGTH = 32K
  RAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 4K
}

SECTIONS
{
  .text :
  {
    KEEP(*(.isr_vector))          /* vectors + boot header + "Switch QUAL" stub at 0xC0 */
    *(.text.Reset_Handler)
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
  } > CODE

  .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } > CODE

  __data_load__ = LOADADDR(.data);
  .data :
  {
    __data_start__ = .;
    *(.data*)
    . = ALIGN(4);
    __data_end__ = .;
  } > RAM AT > CODE

  __image_end__ = LOADADDR(.data) + SIZEOF(.data);
  __image_size  = ALIGN(__image_end__, 4096);

  .bss (NOLOAD) :
  {
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > RAM

  .heap (NOLOAD) : { *(.heap*) } > RAM

  .stack (ORIGIN(RAM) + LENGTH(RAM) - SIZEOF(.stack)) (NOLOAD) :
  {
    *(.stack*)
  } > RAM

  ASSERT(__bss_end__ <= ADDR(.stack), "RAM overflow: .data/.bss collide with stack")
  ASSERT(__image_size <= 0x8000, "image larger than 32 KB code SRAM")
}
