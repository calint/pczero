# tested on:
# * dell inspiron 1545
# * asus-eeepc-4g
# * hp-compaq-mini-110
# * sony-vaio-vgnfw11m
# * qemu 0.11.0 on linux 2.6
# * qemu 7.0.0 on linux 5.19
# * asus zenbook
#
# 00000-003FF  IVT (Interrupt Vector Table)
# 00400-005FF  BDA (BIOS Data Area)
# 00600-9FFFF  Ordinary application RAM
# A0000-BFFFF  Video memory
# C0000-EFFFF  Optional ROMs (The VGA ROM is usually located at C0000)
# F0000-FFFFF  BIOS ROM
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - intro
.set IDT,0x600 # interrupt descriptor table address
.global osca_key # last pressed key
.global osca_t # lower ticker value
.global osca_th # high ticker value
.global _start # export entry point
#.text
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - bpb
.code16 # boot in 16 bit mode
_start:
# jump over BIOS Parameter Block (BPB) because
# that memory may be written to by BIOS when booting from USB
jmp _main
.space 3-(.-_start),0x90 # support 2 or 3 bytes encoded jmp
.space 59,0x90 # BPB area that may be written to by BIOS
_main:
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - setup
cli # disable interrupts
# disable NMI? outb(0x70,inb(0x70)|0x80);inb(0x71);
xor %bx,%bx # clear bx
mov %bx,%ds # data segment
mov %dl,osca_drv_b # save boot drive
mov %bx,%ss # setup stack
mov $_start,%sp # at 0x7c00
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - load
cld # clear direction (forward)
# command 2 (%ah), sectors to read (%al) 0x3f (31½K)
#mov $0x023f,%ax
# read 128 sectors * 512B = 65536 bytes. maximum without DMA exception
mov $0x0280,%ax
# from cylinder 0 (%ch), sector 2 (%cl) (sector starts from 1, not 0)
mov $0x0002,%cx
xor %dh,%dh # head 0
# %dl, drive number, is unchanged
mov $0x07e0,%bx # to es:bx=7e00:0000
mov %bx,%es
xor %bx,%bx
int $0x13 # read disk
#jnc 1f # if no error
#  # error
#  mov $0xb800,%ax # console segment
#  mov %ax,%es
#  movw $0x4045,%es:0 # E 'E1' to top left corner
#  movw $0x4031,%es:2 # 1
#  hlt
#1:
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - mode 13h
mov $0x13,%ax # vga mode 320x200x8 bmp @ 0xa0000
int $0x10
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - display
#movw $0x0404,%es:0x100
#mov $0xa000,%ax
#mov %ax,%es
#mov $0x8000,%di
#mov $0x7c00,%si
#mov $PROG_SIZE>>1,%cx
#rep movsw
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - a20
# enable access to odd megs
# enabled by default on non ancient pc?
#movw $0x0404,%es:0x104
in $0x92,%al
or $2,%al
out %al,$0x92
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - 32b
#movw $0x0404,%es:0x108
lgdt gdtr # load global descriptor tables
mov %cr0,%eax # enter 32b protected mode
or $0x1,%al
mov %eax,%cr0
jmp $8,$protected_mode # jmp and flush
.align 16,0x90
gdt:.quad 0x0000000000000000 #0x00:
    .quad 0x00cf9a000000ffff #0x08: 32b code 4g pl0 rx
	 .quad 0x00cf92000000ffff #0x10: 32b data 4g pl0 rw
    .quad 0x009f9a000000ffff #0x18: 16b code 1m rx
    .quad 0x009f92000000ffff #0x20: 16b data 1m rw
gdtr:.word gdtr-gdt-1,gdt,0,0
ivtr:.word 0x03ff
     .long 0x00000000
.align 8,0
idtr:.word 0x03ff
     .long IDT # idt address
.align 8,0x90
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - protected mode
protected_mode:
.code32
mov $0x10,%ax
mov %ax,%ss
mov %ax,%ds
mov %ax,%es
mov %ax,%fs
mov %ax,%gs
xor %eax,%eax
xor %ebx,%ebx
xor %ecx,%ecx
xor %edx,%edx
xor %edi,%edi
xor %esi,%esi
xor %ebp,%ebp
#movw $0x0404,0xa0110
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - isr
#movw $0x0404,0xa0114
mov $IDT,%ebx # idt address
mov $0x40,%ecx # interrupt count
1:
    movw $isr_err,(%ebx) # offset 0..15
    movw $0x0008,2(%ebx) # selector in gdt
    movb $0x00,  4(%ebx) # unused
    movb $0x8e,  5(%ebx) # type_attrs p,pv0,!s,i32b
    movw $0x0000,6(%ebx) # offfset 16..31
    add $8,%bx
loop 1b
#movl $0x0e0e0f0f,0xa0118 # on screen
movw $isr_tck,IDT+0x40 # only the lower offset
movw $isr_kbd,IDT+0x48 #   handlers within 64K
lidt idtr
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - start
#movw $0x0404,0xa4000
call osca_init
mov osca_tsk_a,%ebx # ebx points to active task record
# potential bug? because eflags not restored and registers in unknown state?)
mov 4(%ebx),%esp # restore stack pointer (only?)
sti # enable interrupts (racing?)
jmp *(%ebx) # jmp to first task
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - vars
.align 16,0x90
osca_drv_b:.byte 0x00 # boot drive
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - partition
.space _start+436-.,0X90 # reserved
.space 10,0 # partition table
.space 16,0 # #1
.space 16,0 # #2
.space 16,0 # #3
.space 16,0 # #4
.word 0xaa55 # pc boot sector signature
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - interrupts
sector2: # 0x7e00
osca_t:.long 0x00000000 # tick lower
osca_th:.long 0x00000000 # tick higher
osca_tsk_a:.long tsk # pointer to first task record (tsk0)
isr_tck_eax:.long 0x00000000 # used in isr_tck
isr_tck_ebx:.long 0x00000000 # ...
isr_tck_esp:.long 0x00000000 # ...
isr_tck_eip:.long 0x00000000 # ...
isr_tck_eflags:.long 0x00000000 #...
osca_key:.byte 0x00 # last key event
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - ierr
.align 16,0x90
isr_err:
  cli # pop error code and print it?
  pop %eax
  mov %eax,0xa0000
  incl 0xa0004
  jmp isr_err
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - ikbd
.align 16,0x90
isr_kbd:
  cli # is disabling interrupts necessary?
  push %ax # save to be clobbered %ax
  in $0x60,%al # read keyboard port
  mov %al,osca_key # store
  mov %al,0xa0000+320-8 # on screen
  pusha # save registers
  call osca_keyb_ev # call keyb handler function
  popa # restore register
  mov $0x20,%al # ack interrupt
  out %al,$0x20 # ...
  pop %ax # restore %ax
  sti # enable interrupts
  iret
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - itck
.align 16,0x90
isr_tck:
  incl 0xa0000+320-4 # on screen, top right corner
  # enable this to disable task switching. will run only 'tsk0'
  incl osca_t # increase 64b ticker
  adcl $0,osca_th # add carry to upper 32 bits
  push %ax # ack irq
  mov $0x20,%al
  out %al,$0x20
  pop %ax
  iret

  cli # disable interrupts while switching task
  mov %eax,isr_tck_eax # save eax,ebx
  mov %ebx,isr_tck_ebx # eax,ebx will be used as scratch
  incl osca_t # increase 64b ticker
  adcl $0,osca_th # add carry to upper 32 bits
#  mov osca_t,%eax # on screen
#  mov %eax,0xa0130
  mov osca_tsk_a,%ebx # point ebx to active task
  mov (%esp),%eax # get eip before irq from stack
  mov %eax,(%ebx) # save to task.eip
  mov 8(%esp),%eax # get eflags before ieq from stack
  mov %eax,8(%ebx) # save to task.eflags
  mov %esp,%eax # adjust esp to value before irq
  add $12,%eax # skip eip,cs,eflags
#  add $16,%eax # skip error code,eip,cs,eflags
  mov %eax,4(%ebx) # save to task.esp
  mov %ebx,%esp # save gprs by preparing esp and then pusha
  add $48,%esp # move to end of task record
  pusha # pushes eax,ecx,edx,ebx,esp0,ebp,esi,edi to task record
  mov isr_tck_eax,%eax # save proper eax,ebx
  mov %eax,44(%ebx) # to task.eax
  mov isr_tck_ebx,%eax # save proper ebx
  mov %eax,32(%ebx) # to task.ebx
  add $48,%ebx # next task
  cmp $tsk_eot,%ebx # if last
  jl 7f
    mov $tsk,%ebx # roll
  7:
  mov %ebx,osca_tsk_a # save pointer to task record
  mov 4(%ebx),%esp # restore esp
  mov %esp,isr_tck_esp # save esp, will be used as scratch
  mov (%ebx),%esp # restore eip
  mov %esp,isr_tck_eip # save for jump
  mov 8(%ebx),%esp # get eflags
  mov %esp,isr_tck_eflags # save to restore later
  mov %ebx,%esp # restore gprs
  add $16,%esp # position stack pointer for popa by skipping eip,esp,eflags,bits
  popa # write edi,esi,ebp,esp0,ebx,edx,ecx,eax
  push isr_tck_eflags # restore eflags
  popf # restore eflags. no calculations done to alter it before resuming task.
  mov isr_tck_esp,%esp # restore task esp
  push %ax # ack irq
  mov $0x20,%al
  out %al,$0x20
  pop %ax
  sti # enable interrupts
  jmp *isr_tck_eip # jmp to restored eip. racing?
.space sector2+512-.,0X90
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - tasks
# sector3: #0x8200 tasks state table
.align 16,0x90
tsk: # eip,  esp,       eflags,     bits,       edi        esi        ebp        esp0       ebx        edx        ecx        eax
tsk_eot:
  .long tsk4 ,0x000afa00,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
  .long tsk0 ,0x000afa00,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
  .long tsk1 ,0x000af780,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
  .long tsk2 ,0x000af500,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
  .long tsk3 ,0x000af280,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
  .long tsk4 ,0x000af000,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#tsk_eot:
#  .long tsk5 ,0x000aed80,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk6 ,0x000aeb00,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk7 ,0x000ae880,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000

#  .long tsk8 ,0x000ae600,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk9 ,0x000ae380,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk10,0x000ae100,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk11,0x000ade80,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk12,0x000adc00,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk13,0x000ad980,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk14,0x000ad700,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk15,0x000ad480,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk16,0x000ad200,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk17,0x000acf80,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk18,0x000acd00,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk19,0x000aca80,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#tsk_eot:
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - shutdown
#.align 16,0x90
#mode16: # protected mode to 16b mode
#.code16
#mov $0x20,%ax
#mov %ax,%ds
#mov %ax,%ss
#mov $0x7c00,%sp
#lidt ivtr
#mov %cr0,%eax
#and $0xfe,%al
#mov %eax,%cr0
#jmp $0x0,$rm
#.align 16,0x90
#rm:
#xor %ax,%ax
#mov %ax,%ds
#mov %ax,%ss
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - save
## save 2nd sector
#mov $0x0301,%ax # command 3, 1 sector
#mov $0x0002,%cx # track 0, sector 2
#xor %dh,%dh # head 0
#mov osca_drv_b,%dl # saved boot drive
#xor %bx,%bx # from es:bx (0:0x7e00)
#mov %bx,%es
#mov $0x7e00,%bx
#int $0x13
#jc 8f # if error
## display save ack?
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - done
#8:
#cli
#hlt
.space sector2+1024-.,0X90
