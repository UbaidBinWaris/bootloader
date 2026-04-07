OUTPUT_FORMAT(binary)
ENTRY(_start)

SECTIONS
{
    . = 0x2000;

    _kernel_start = .;

    .text :
    {
        *(.text)
        *(.text.*)
    }

    . = ALIGN(4);
    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
    }

    . = ALIGN(4);
    .data :
    {
        *(.data)
        *(.data.*)
    }

    . = ALIGN(4);
    _bss_start = .;
    .bss :
    {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
    _bss_end = .;

    _kernel_end = .;
}