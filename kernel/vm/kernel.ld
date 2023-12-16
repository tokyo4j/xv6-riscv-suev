OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

SECTIONS
{
  . = 0x0;

  .private : {
    *(.entry)
    *(.text .text.*)
    . = ALIGN(16);
    *(.rodata .rodata.*)
    . = ALIGN(16);
    *(.data .data.*)
    . = ALIGN(16);
    *(.bss .bss.*)
    . = ALIGN(4096);
  }

  PROVIDE(end = .);
}
