SECTIONS
{
  . = 0;
  .text : { *(.text) }
  .got : { *(.got) }
  .data : { *(.data) }
  .bss : { *(.bss) }
  .stack : { *(.stack) }
}
