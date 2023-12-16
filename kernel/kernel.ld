OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

SECTIONS
{
  /*
   * ensure that entry.S / _entry is at 0x80000000,
   * where qemu's -kernel jumps.
   */
  . = 0x80000000;

  .text : {
    *(.text .text.*)
    . = ALIGN(0x1000);
    _trampoline = .;
    *(trampsec)
    . = ALIGN(0x1000);
    ASSERT(. - _trampoline == 0x1000, "error: trampoline larger than one page");
    PROVIDE(etext = .);
  }

  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
    . = ALIGN(16);
    *(.rodata .rodata.*)
  }

  . = ALIGN(0x1000);

  .data : {
    *(.sdata .sdata.*) /* do not need to distinguish this from .data */
    . = ALIGN(16);
    *(.data .data.*)
  }

  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
    . = ALIGN(16);
    *(.bss .bss.*)
  }

  PROVIDE(end = .);

  . = ALIGN(0x1000);
  .rmp : {
    _rmp_start = .;
    . += 16*1024*1024; /* 16M RMP covers 0-4GB physical memory */
    _rmp_size = ABSOLUTE(. - _rmp_start);
  }

  . = ALIGN(0x1000);
  .vm : {
    _vm_start = .;
    . += 64*64*4096;
    _vm_size = ABSOLUTE(. - _vm_start);
  }

  . = ALIGN(0x1000);
  PROVIDE(heap = .);
}
