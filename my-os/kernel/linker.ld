OUTPUT_FORMAT(binary)
ENTRY(_start)

SECTIONS
{
    . = 0x2000;

    .text : { *(.text) }

    . = ALIGN(4);
    .rodata : { *(.rodata) }

    . = ALIGN(4);
    .data : { *(.data) }

    . = ALIGN(4);
    _bss_start = .;
    .bss  : { *(.bss) *(COMMON) }
    _bss_end = .;

    _kernel_end = .;
}