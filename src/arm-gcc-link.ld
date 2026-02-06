OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
/* Internal Memory Map*/
MEMORY
{
    rom (rx)  : ORIGIN = 0x08000000, LENGTH = 0x00010000
    ram (rwx) : ORIGIN = 0x20000000, LENGTH = 0x00002000
}

_eram = 0x20000000 + 0x00002000;
__HeapSize = 0x800;
SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.text*)

        KEEP(*(.init))
        KEEP(*(.fini))

        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        /* .dtors */
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        *(.rodata*)

        KEEP(*(.eh_fram e*))
    } > rom

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > rom

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > rom 
    __exidx_end = .;
    __etext = .;

    /* _sidata is used in coide startup code */
    _sidata = __etext;

    .data : AT (__etext)
    {
        __data_start__ = .;

        /* _sdata is used in coide startup code */
        _sdata = __data_start__;

        *(vtable)
        *(.data*)

        . = ALIGN(8);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(8);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);

        . = ALIGN(8);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        KEEP(*(.jcr*))
        . = ALIGN(8);
        /* All data end */
        __data_end__ = .;

        /* _edata is used in coide startup code */
        _edata = __data_end__;
    } > ram

    .bss :
    {
        . = ALIGN(8);
        __bss_start__ = .;
        _sbss = __bss_start__;
        *(.bss*)
        *(COMMON)
        . = ALIGN(8);
        __bss_end__ = .;
        _ebss = __bss_end__;
    } > ram

    .heap :
    {
        . = ALIGN(8);
        __HeapStart = .;
        __end__ = .;
        _end = __end__;
        end = __end__;
        *(.heap*)
        . = . + __HeapSize;
        __HeapLimit = .;
    } > ram

    /* .co_stack section doesn't contains any symbols. It is only
     * used for linker to calculate size of stack sections, and assign
     * values to stack symbols later */
    .co_stack (NOLOAD):
    {
        . = ALIGN(8);
        __StackStart = .;
        *(.co_stack .co_stack.*)
        . = ALIGN(8);
        __StackEnd = .;
    } > ram

    /* NOTE: These variables don't seem to affect anything.
     *
     * Set stack top to end of ram , and stack limit move down by
     * size of co_stack section */
    __StackTop = ORIGIN(ram) + LENGTH(ram);
    __StackLimit = __StackTop - SIZEOF(.co_stack);
    PROVIDE(__stack = __StackTop);

    /* Check if data + heap + stack exceeds ram  limit */
    ASSERT(__StackLimit >= __HeapLimit, "region ram overflowed with stack")

    .last_section (NOLOAD):
    {
        . = ALIGN(8);
    } > ram
}
