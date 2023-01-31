// tested on:
// * dell inspiron 1545
// * asus-eeepc-4g
// * hp-compaq-mini-110
// * sony-vaio-vgnfw11m
// * qemu 0.11.0 on linux 2.6
// * qemu 7.0.0 on linux 5.19
// * asus zenbook
//
// 00000-003FF  IVT (Interrupt Vector Table)
// 00400-005FF  BDA (BIOS Data Area)
// 00600-9FFFF  Ordinary application RAM
// A0000-BFFFF  Video memory
// C0000-EFFFF  Optional ROMs (The VGA ROM is usually located at C0000)
// F0000-FFFFF  BIOS ROM
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - intro
asm(".set IDT,0x600");// interrupt descriptor table address
asm(".set LOAD_SECTORS,0x1f");// 15½K
asm(".set PROG_SIZE,0x200+LOAD_SECTORS*0x200");
asm(".global osca_key");// last pressed key
asm(".global osca_t");// lower ticker value
asm(".global osca_t1");// high ticker value
asm(".global _start");// export entry point
asm(".code16");// boot in 16 bit mode
asm("_start:");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - bpb
// jump over BIOS Parameter Block (BPB) because 
// that memory may be written to by BIOS when booting from USB
asm("jmp _main");
asm(".space 3-(.-_start),0x90");// support 2 or 3 bytes encoded jmp
asm(".space 59,0x90");// BPB area that may be written to by BIOS
asm("_main:");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - setup
asm("xor %bx,%bx");
asm("mov %bx,%ds");// data segment
asm("mov %dl,osca_drv_b");// save boot drive
asm("mov %bx,%ss");// setup stack
asm("mov $_start,%sp");// 0x7c00
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - load
// %dl is the unchanged boot drive
asm("cld");// clear direction (forward)
asm("mov $(0x0200+LOAD_SECTORS),%ax");// command 2, 1fh sectors
asm("mov $0x0002,%cx");// from cylinder 0, sector 2
asm("mov $0,%dh");// head 0
asm("xor %bx,%bx");// to es:bx
asm("mov %bx,%es");
asm("mov $0x7e00,%bx");// (0:0x7e00)
asm("int $0x13");
asm("jnc 1f");// if no error jmp
asm("  mov $0xb800,%ax");// console segment
asm("  mov %ax,%es");
asm("  movw $0x1045,%es:0");// 'E' to top left corner
asm("  2:cli");// hang
asm("    hlt");
asm("    jmp 2b");// enough with cli,hlt?
asm("1:");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - mode 13h
asm("mov $0x13,%ax");// vga mode 320x200x8 bmp @ 0xa0000
asm("int $0x10");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - display
//asm("movw $0x0404,%es:0x100");
asm("mov $0xa000,%ax");
asm("mov %ax,%es");
asm("mov $0x8000,%di");
asm("mov $0x7c00,%si");
asm("mov $PROG_SIZE>>1,%cx");
asm("rep movsw");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - a20
//asm("movw $0x0404,%es:0x104");
asm("in $0x92,%al");// enable a20 line (odd megs)
asm("or $2,%al");
asm("out %al,$0x92");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - 32b
//asm("movw $0x0404,%es:0x108");
asm("lgdt gdtr");// load global descriptor tables
asm("mov %cr0,%eax");// enter 32b protected mode
asm("or $0x1,%al");
asm("mov %eax,%cr0");
asm("jmp $8,$pm");// jmp to flush
asm(".align 16,0x90");
asm("gdt:.quad 0x0000000000000000");//0x00:
asm("    .quad 0x00cf9a000000ffff");//0x08: 32b code 4g pl0 rx
asm("	 .quad 0x00cf92000000ffff");//0x10: 32b data 4g pl0 rw
asm("    .quad 0x009f9a000000ffff");//0x18: 16b code 1m rx
asm("    .quad 0x009f92000000ffff");//0x20: 16b data 1m rw
asm("gdtr:.word gdtr-gdt-1,gdt,0,0");
asm("ivtr:.word 0x03ff");
asm("     .long 0x00000000");
asm(".align 8,0");
asm("idtr:.word 0x03ff");
asm("     .long IDT");// idt address
asm(".align 8,0");
asm("pm:");// protected mode code
asm(".code32");
asm("mov $0x10,%ax");
asm("mov %ax,%ss");
asm("mov %ax,%ds");
asm("mov %ax,%es");
asm("mov %ax,%fs");
asm("mov %ax,%gs");
asm("xor %eax,%eax");
asm("xor %ebx,%ebx");
asm("xor %ecx,%ecx");
asm("xor %edx,%edx");
asm("xor %edi,%edi");
asm("xor %esi,%esi");
asm("xor %ebp,%ebp");
//asm("movw $0x0404,0xa0110");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - isr
//asm("movw $0x0404,0xa0114");
asm("cli");// disable interrupts
asm("mov $IDT,%ebx");// idt address
asm("mov $0x40,%ecx");// interrupt count
asm("1:");
asm("    movw $isr_err,(%ebx)");// offset 0..15
asm("    movw $0x0008,2(%ebx)");// selector in gdt
asm("    movb $0x00,  4(%ebx)");// unused
asm("    movb $0x8e,  5(%ebx)");// type_attrs p,pv0,!s,i32b
asm("    movw $0x0000,6(%ebx)");// offfset 16..31
asm("    add $8,%bx");
asm("loop 1b");
asm("movl $0x0e0e0f0f,0xa0118");// ? remove debugging
asm("movw $isr_tck,IDT+0x40");// only the lower offset
asm("movw $isr_kbd,IDT+0x48");//   handlers within 64K
asm("lidt idtr");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - start
//asm("movw $0x0404,0xa4000");
asm("mov osca_tsk_a,%ebx");// ebx points to active task record
asm("mov 4(%ebx),%esp");// restore esp
asm("sti");// enable interrupts (racing?)
asm("jmp *(%ebx)");// jmp to first task
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - vars
asm(".align 16,0x90");
asm("osca_drv_b:.byte 0x00");// boot drive
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - partition
asm(".space _start+436-.,0");// reserved
asm(".space 10,0");// partition table
asm(".space 16,0");// #1
asm(".space 16,0");// #2
asm(".space 16,0");// #3
asm(".space 16,0");// #4
asm(".word 0xaa55");// pc boot sector signature
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - saved
asm("sector2:");// 0x7e00 (saved at shutdown)
asm("osca_t:.long 0x00000000");
asm("osca_t1:.long 0x00000000");
asm("osca_key:.long 0x00000000");
asm(".space sector2+512-.");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - vars
asm("sector3:");// 0x8000 tasks switcher
asm("osca_tsk_a:.long tsk");
asm("isr_tck_eax:.long 0x00000000");
asm("isr_tck_ebx:.long 0x00000000");
asm("isr_tck_esp:.long 0x00000000");
asm("isr_tck_eip:.long 0x00000000");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - ierr
asm(".align 16,0x90");
asm("isr_err:");
asm("  cli");
asm("  incw 0xa0000");
asm("  jmp isr_err");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - ikbd
asm(".align 16,0x90");
asm("isr_kbd:");
asm("  push %ax");
asm("  in $0x60,%al");// read keyboard port
asm("  mov %al,osca_key");// store
asm("  mov %al,0xa0100");// to vga remove debugging?
asm("  pushal");// save register
asm("  call osca_keyb_ev");// call device keyb function ev
asm("  popal");// restore register
asm("  mov $0x20,%al");// ack interrupt
asm("  out %al,$0x20");
asm("  pop %ax");
asm("  iret");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - itck
asm(".align 16,0x90");
asm("isr_tck:");
asm("  cli");// disable interrupts while task switching
asm("  movw $0x0e0e,0xa0200");// remove debugging?
asm("  mov %eax,isr_tck_eax");// save eax,ebx
asm("  mov %ebx,isr_tck_ebx");
asm("  incl osca_t");// increase 64b ticker
asm("  adcl $0,osca_t1");
asm("  mov osca_t,%eax");// on screen. remove debugging?
asm("  mov %eax,0xa0130");
asm("  mov osca_tsk_a,%ebx");// ebx points to active task
asm("  mov (%esp),%eax");// get eip before irq from stack
asm("  mov %eax,(%ebx)");// save to task.eip
asm("  mov 8(%esp),%eax");// get eflags from stack
asm("  mov %eax,8(%ebx)");// save to task.eflags
asm("  mov %esp,%eax");// adjust esp
asm("  add $12,%eax");// eip,cs,eflags
asm("  mov %eax,4(%ebx)");// save to task.esp
asm("  mov %ebx,%esp");// save gprs
asm("  add $48,%esp");// move to end of task record
asm("  pushal");// pushes eax,ecx,edx,ebx,esp0,ebp,esi,edi
asm("  mov isr_tck_eax,%eax");// save proper eax,ebx
asm("  mov %eax,44(%ebx)");// task.eax
asm("  mov isr_tck_ebx,%eax");
asm("  mov %eax,32(%ebx)");// task.ebx
asm("  add $48,%ebx");// next task
asm("  cmp $tsk_eot,%ebx");// if last
asm("  jl 7f");
asm("    mov $tsk,%ebx");// roll
asm("  7:");
asm("  mov %ebx,osca_tsk_a");// save pointer of task
asm("  mov 4(%ebx),%esp");// restore esp
asm("  mov %esp,isr_tck_esp");// save esp, will be used as scratch
asm("  mov (%ebx),%esp");// restore eip
asm("  mov %esp,isr_tck_eip");// save for jump
asm("  mov %ebx,%esp");// restore gprs
asm("  add $16,%esp");// position stack pointer for pop
asm("  popal");
asm("  mov isr_tck_esp,%esp");// restore esp
asm("  push %ax");// ack irq
asm("  mov $0x20,%al");
asm("  out %al,$0x20");
asm("  pop %ax");
asm("  sti");// enable interrupts
asm("  jmp *isr_tck_eip");// jmp to restored eip. racing?
asm(".space sector3+512-.");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - tasks
asm("sector4:");//0x8200 tasks state table
asm("tsk:");// eip,  esp,       eflags,     bits,       edi        esi        ebp        esp0       ebx        edx        ecx        eax
asm("  .long tsk0,0x000afa00,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk1,0x000af780,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk2,0x000af500,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk3,0x000af280,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk4,0x000af000,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk5,0x000aed80,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk6,0x000aeb00,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk7,0x000ae880,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");

asm("  .long tsk8,0x000ae600,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk8,0x000ae380,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk8,0x000ae100,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk8,0x000ade80,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk8,0x000adc00,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk8,0x000ad980,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk8,0x000ad700,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk8,0x000ad480,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");

asm("  .long tsk9,0x000ad200,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk10,0x000acf80,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk8,0x000acd00,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("  .long tsk8,0x000aca80,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000");
asm("tsk_eot:");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - shutdown
asm(".align 16,0x90");
asm("mode16:");// protected mode to 16b mode
asm(".code16");
asm("mov $0x20,%ax");
asm("mov %ax,%ds");
asm("mov %ax,%ss");
asm("mov $0x7c00,%sp");
asm("lidt ivtr");
asm("mov %cr0,%eax");
asm("and $0xfe,%al");
asm("mov %eax,%cr0");
asm("jmp $0x0,$rm");
asm(".align 16,0x90");
asm("rm:");
asm("xor %ax,%ax");
asm("mov %ax,%ds");
asm("mov %ax,%ss");
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - save
// save 2nd sector
asm("mov $0x0301,%ax");// command 3, 1 sector
asm("mov $0x0002,%cx");// track 0, sector 2
asm("xor %dh,%dh");// head 0
asm("mov osca_drv_b,%dl");// saved boot drive
asm("xor %bx,%bx");// from es:bx (0:0x7e00)
asm("mov %bx,%es");
asm("mov $0x7e00,%bx");
asm("int $0x13");
asm("jc 8f");// if error
// display save ack?
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - done
asm("8:cli");
asm("  hlt");
asm("  jmp 8b");
asm("page0:");
asm(".align 0x400");
asm(".space 0x1000,1");
