OUTPUT_FORMAT("binary");

SECTIONS {
    . = 0x7c00;
    .text :{
        /* include all sections (.text, .data, .bss, etc.) from 
         * the specified object files, not just their .text sections. */
        bin/src/osca.o
        bin/src/main.o
    }
    . = ALIGN(0x10000);
    free_mem_start_symbol = .;
}