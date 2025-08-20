OUTPUT_FORMAT("binary");

SECTIONS {
    . = 0x7c00;
    .text :{
        bin/src/osca.o
        bin/src/main.o
        *(.text)
    }
    .rodata :{
        *(.rodata)
        *(.rodata.*)
    }
    .srodata :{
        *(.srodata)
        *(.srodata.*)
    }
    .sdata :{
        *(.sdata)
        *(.sdata.*)
    }
    .data :{
        *(.data)
        *(.data.*)
    }
    .bss :{
        *(.bss)
        *(.bss.*)
    }
    . = ALIGN(0x10000);
    free_mem_start_symbol = .;
}