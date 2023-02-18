# tested on:
# * qemu 7.0.0 on linux 5.19
# * asus zenbook
#
# 00000-003FF  IVT (Interrupt Vector Table)
# 00400-005FF  BDA (BIOS Data Area)
# 00600-9FFFF  Ordinary application RAM
# 00600-007ff    osca IDT (Interrupt Descriptor Table)
# A0000-BFFFF  Video memory
# C0000-EFFFF  Optional ROMs (The VGA ROM is usually located at C0000)
# F0000-FFFFF  BIOS ROM
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - intro
.set IDT,0x600 # interrupt descriptor table address
.set FREQ_DIV_AND,8-1 # task switches per second 1024/8=128
                      # high task switching frequency might hang
                      # slower hardware
.global osca_key # last received keyboard scan code
.global osca_tmr_lo # 1024 Hz timer low bits
.global osca_tmr_hi # 1024 Hz timer high bits
.global osca_yield # export function
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
xor %bx,%bx # clear bx
mov %bx,%ds # data segment
mov %dl,osca_drv_b # save boot drive
mov %bx,%ss # setup stack
mov $_start,%sp # at 0x7c00
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - load
# https://wiki.osdev.org/Disk_access_using_the_BIOS_(INT_13h)
# Reading sectors with a CHS address
# Cylinder = 0 to 1023 (maybe 4095), Head = 0 to 15 (maybe 254, maybe 255), Sector = 1 to 63
#    Set AH = 2
#    AL = total sector count (0 is illegal) -- cannot cross ES page boundary, or a cylinder boundary, and must be < 128
#    CH = cylinder & 0xff
#    CL = Sector | ((cylinder >> 2) & 0xC0);
#    DH = Head -- may include two more cylinder bits
#    ES:BX -> buffer
#    Set DL = "drive number" -- typically 0x80, for the "C" drive
#    Issue an INT 0x13.
# The carry flag will be set if there is any error during the read. AH should be set to 0 on success.
#
cld # clear direction (forward)
# read 127 sectors * 512B = 65024 bytes
#   assumes load from USB. floppy does support that many sectors.
# command 2 (%ah), number of sectors to read (%al)
mov $0x027f,%ax
# from cylinder 0 (%ch), sector 2 (%cl) (sector starts from 1, not 0)
mov $0x0002,%cx
xor %dh,%dh # head 0
# %dl, drive number, is unchanged
mov $0x07e0,%bx # to es:bx=7e00:0000
mov %bx,%es
xor %bx,%bx
int $0x13 # read disk
#jnc lbl1 # if no error
#  # error
#  mov $0xb800,%ax # console segment
#  mov %ax,%es
#  movw $0x4045,%es:0 # E 'E1' to top left corner
#  movw $0x4031,%es:2 # 1
#  hlt
#lbl1:
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - mode 13h
# switch to vga mode 13h (320x200x8 bmp @ 0xa0000)
mov $0x13,%ax
int $0x10
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - display
# display loaded program on screen
#mov $0xa000,%ax # vga
#mov %ax,%es     # at 0xa0000
#xor %di,%di     #
#mov $0x7c0,%ax  # program
#mov %ax,%ds     # starting at 0x7c00
#xor %si,%si     #
#mov $0xffff,%cx # copy 65535 bytes
#rep movsb
#xor %ax,%ax     # restore data segment
#mov %ax,%ds     # to zero
#hlt
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - a20
# enable access to odd megs
# https://wiki.osdev.org/A20_Line
#   enabled by default on non ancient pc?
#movw $0x0404,%es:0x104
in $0x92,%al
or $2,%al
out %al,$0x92
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - protected mode
# enable 32bit protected mode
# https://wiki.osdev.org/Protected_Mode
#movw $0x0404,%es:0x108
lgdt gdtr # load global descriptor tables
mov %cr0,%eax # enter 32b protected mode
or $1,%al     #  set PE (Protection Enable) bit
mov %eax,%cr0 #  in CR0 (Control Register 0)
# far jump to selector 08h (offset into GDT, pointing at
# a 32bit PM code segment descriptor) to load CS with
# proper PM32 descriptor
jmp $8,$protected_mode # jmp and flush
.align 16,0x90
# global descriptor table
gdt:.quad 0x0000000000000000 #0x00:
    .quad 0x00cf9a000000ffff #0x08: 32b code 4g pl0 rx
    .quad 0x00cf92000000ffff #0x10: 32b data 4g pl0 rw
#    .quad 0x009f9a000000ffff #0x18: 16b code 1m rx
#    .quad 0x009f92000000ffff #0x20: 16b data 1m rw
# global descriptor table register
gdtr:.word gdtr-gdt-1 # limit of table (size of table - 1)
     .long gdt # address
# interrupt vector table (16 bit mode)
#ivtr:.word 0x03ff     # limit (size of table - 1)
#     .long 0x00000000 # address
.align 8,0
# interrupt descriptor table register (32 bit protected mode)
idtr:.word 0x03ff # limit of table (size of table - 1)
     .long IDT    # idt address
.align 8,0x90
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - 32b code
protected_mode:
.code32
mov $0x10,%ax # offset in gdt, the data segment
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
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - map irqs
# reprogram the PIC (8259's) to map IRQ 0-15 to IDT mapping 32 to 47
# avoiding the first 32 reserved for exceptions
# https://en.wikibooks.org/wiki/X86_Assembly/Programmable_Interrupt_Controller
mov $0x11,%al
out %al,$0x20 # restart PIC1
out %al,$0xa0 # restart PIC2

mov $0x20,%al
out %al,$0x21 # PIC1 now starts at 32
mov $0x28,%al
out %al,$0xa1 # PIC2 now starts at 40

mov $0x04,%al
out %al,$0x21
mov $0x02,%al # setup cascading
out %al,$0xa1

mov $0x01,%al
out %al,$0x21
out %al,$0xa1 # done
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - irq 8 timer
# disable the Non-Maskable Interrupt (NMI) before programming RTC
# this interrupt happens when there is a critical hardware failure
# https://wiki.osdev.org/Non_Maskable_Interrupt
in $0x70,%al
or $0x80,%al
out %al,$0x70
in $0x71,%al
# enable RTC 1024 Hz timer on IRQ 8
# https://wiki.osdev.org/RTC
mov $0x8b,%al
out %al,$0x70
in $0x71,%al
mov %al,%dl
mov $0x8b,%al
out %al,$0x70
mov %dl,%al
or $0x40,%al
out %al,$0x71
# enable NMI
in $0x70,%al
and $0x7f,%al
out %al,$0x70
in $0x71,%al
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - isr
# setup interrupt service routines
# https://wiki.osdev.org/Interrupt_Descriptor_Table
#movw $0x0404,0xa0114
mov $IDT,%ebx # idt address
mov $0x40,%ecx # interrupt count
lbl2:
    movw $isr_err,(%ebx) # offset 0..15
    movw $0x0008,2(%ebx) # selector in gdt
    movb $0x00,  4(%ebx) # unused
    movb $0x8e,  5(%ebx) # type_attrs p,pv0,!s,i32b
    movw $0x0000,6(%ebx) # offfset 16..31
    add $8,%bx
loop lbl2
#movl $0x0e0e0f0f,0xa0118 # on screen
# setup IRQ 0,1,8. keyboard, system timer and RTC timer
#  only the lower offset set, handlers within 64K
movw $isr_tck,IDT+(32+0)*8 # IRQ 0: keyboard
movw $isr_kbd,IDT+(32+1)*8 # IRQ 1: timer 18.2 Hz
movw $isr_tmr,IDT+(32+8)*8 # IRQ 8: timer 1024 Hz
lidt idtr
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - start
#movw $0x0404,0xa4000
call osca_init # calls C function
mov osca_tsk_active,%ebx # ebx points to active task record
# potential bug? because eflags not restored and registers in unknown state?
mov 4(%ebx),%esp # restore stack pointer
sti # enables interrupts after next instruction
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
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - sector 2
sector2: # 0x7e00
osca_tmr_lo:.long 0x00000000 # 1024 Hz tick lower
osca_tmr_hi:.long 0x00000000 # 1024 Hz tick higher
osca_tsk_active:.long tsk # pointer to first task record (tsk0)
isr_tmr_eax:.long 0x00000000 # used in isr_tmr
isr_tmr_ebx:.long 0x00000000 # ...
isr_tmr_esp:.long 0x00000000 # ...
isr_tmr_eip:.long 0x00000000 # ...
isr_tmr_eflags:.long 0x00000000 #...
osca_key:.byte 0x00 # last key event
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - yield
.align 16,0x90
osca_yield: # switches to next task
  cli # disable interrupts
  pushf # push flags. stack is now ..., EIP, EFLAGS
  # start task switching
  mov %eax,isr_tmr_eax # save eax,ebx
  mov %ebx,isr_tmr_ebx # eax,ebx will be used as scratch
  mov osca_tsk_active,%ebx # point ebx to active task
  mov 4(%esp),%eax # get eip from stack
  mov %eax,(%ebx) # save to task.eip
  mov (%esp),%eax # get eflags from stack
  mov %eax,8(%ebx) # save to task.eflags
  mov %esp,%eax # adjust esp to value before call
  add $8,%eax # skip eip,eflags
  mov %eax,4(%ebx) # save to task.esp
  mov %ebx,%esp # save gprs by preparing esp and then pusha
  add $48,%esp # move to end of task record
  pusha # pushes eax,ecx,edx,ebx,esp0,ebp,esi,edi to task record
  mov isr_tmr_eax,%eax # save proper eax,ebx
  mov %eax,44(%ebx) # to task.eax
  mov isr_tmr_ebx,%eax # save proper ebx
  mov %eax,32(%ebx) # to task.ebx
  add $48,%ebx # next task
  cmp $tsk_eot,%ebx # if last
  jl lbl5
    mov $tsk,%ebx # roll
  lbl5:
  mov %ebx,osca_tsk_active # save pointer to task record
  mov 4(%ebx),%esp # restore esp
  mov %esp,isr_tmr_esp # save esp, will be used as scratch
  mov (%ebx),%esp # restore eip
  mov %esp,isr_tmr_eip # save for jump
  mov 8(%ebx),%esp # get eflags
  mov %esp,isr_tmr_eflags # save to restore later
  mov %ebx,%esp # restore gprs
  add $16,%esp # position stack pointer for popa by skipping eip,esp,eflags,bits
  popa # write edi,esi,ebp,esp0,ebx,edx,ecx,eax
  push isr_tmr_eflags # restore eflags
  popf # restore eflags. no calculations done to alter it before resuming task
  mov isr_tmr_esp,%esp # restore task esp
  sti # enables interrupts after next instruction
  jmp *isr_tmr_eip # jmp to restored eip
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - interrupts
# "*NOTE* Because IA-32 tasks are not re-entrant, an interrupt-handler
# task must disable interrupts between the time it completes handling
# the interrupt and the time it executes the IRET instruction. This
# action prevents another interrupt from occurring while the interrupt
# task's TSS is still marked busy, which would cause a general-protection
# (#GP) exception."
#
#—Intel Software Developer Manual
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - irq 8 timer
.align 16,0x90
isr_tmr:
  cli # disable interrupts
  incl 0xa0000+320-12 # on screen
  incl osca_tmr_lo     # increase lower 32 bits ticker
  adcl $0,osca_tmr_hi  # add carry to upper 32 bits
  push %eax
  mov osca_tmr_lo,%eax # switch task every 8'th tick
  and $FREQ_DIV_AND,%eax # 1024/8 Hz (running at 1024 Hz hangs Asus Zenbook)
  pop %eax
  jz switch_task  # if no task switch acknowledge irq and return
    push %ax      # save ax
    mov $0x0c,%al # read register C in ISR
    out %al,$0x70 #  to keep getting
    in $0x71,%al  #  irq 8
    mov $0x20,%al # acknowledge interrupt (EOI) to
    out %al,$0xa0 # second controller PIC1
    out %al,$0x20 # main controller PIC0
    pop %ax       # restore ax
    sti           # enables interrupts after next instruction
    iret
switch_task:
  # start task switching
  mov %eax,isr_tmr_eax # save eax,ebx
  mov %ebx,isr_tmr_ebx # eax,ebx will be used as scratch
  mov osca_tsk_active,%ebx # point ebx to active task
  # stack is ..., EFLAGS, CS, EIP
  mov (%esp),%eax # get eip before irq from stack
  mov %eax,(%ebx) # save to task.eip
  mov 8(%esp),%eax # get eflags before ieq from stack
  mov %eax,8(%ebx) # save to task.eflags
  mov %esp,%eax # adjust esp to value before irq
  add $12,%eax # skip eip,cs,eflags
  mov %eax,4(%ebx) # save to task.esp
  mov %ebx,%esp # save gprs by preparing esp and then pusha
  add $48,%esp # move to end of task record
  pusha # pushes eax,ecx,edx,ebx,esp0,ebp,esi,edi to task record
  mov isr_tmr_eax,%eax # save proper eax,ebx
  mov %eax,44(%ebx) # to task.eax
  mov isr_tmr_ebx,%eax # save proper ebx
  mov %eax,32(%ebx) # to task.ebx
  add $48,%ebx # next task
  cmp $tsk_eot,%ebx # if last
  jl lbl4
    mov $tsk,%ebx # roll
  lbl4:
  mov %ebx,osca_tsk_active # save pointer to task record
  mov 4(%ebx),%esp # restore esp
  mov %esp,isr_tmr_esp # save esp, will be used as scratch
  mov (%ebx),%esp # restore eip
  mov %esp,isr_tmr_eip # save for jump
  mov 8(%ebx),%esp # get eflags
  mov %esp,isr_tmr_eflags # save to restore later
  mov %ebx,%esp # restore gprs
  add $16,%esp # position stack pointer for popa by skipping eip,esp,eflags,bits
  popa # write edi,esi,ebp,esp0,ebx,edx,ecx,eax
  push isr_tmr_eflags # restore eflags
  popf # restore eflags. no calculations done to alter it before resuming task.
  mov isr_tmr_esp,%esp # restore task esp

  # https://forum.osdev.org/viewtopic.php?f=1&t=15526&start=0
  # https://www.compuphase.com/int70.txt
  # "When an IRQ 8 fires and interrupt 70h is called, status register C holds a bit
  # mask that tells what kind of interrupt occurred: periodic interrupt, alarm
  # interrupt or update ended interrupt. And unless you read status register C,
  # IRQ 8 will not be generated again. This means that you must read status
  # register C inside your ISR for interrupt 70h even when you normally don't care
  # about its contents. Otherwise you will only see a single interrupt."
  # acknowledge interrupt and jmp
  push %ax # save to be clobbered %ax

  mov $0x0c,%al # read register C in ISR
  out %al,$0x70 #  to keep getting
  in $0x71,%al  #  irq 8

  mov $0x20,%al # acknowledge interrupt (EOI) to
  out %al,$0xa0 #  second controller PIC1
  out %al,$0x20 #  main controller PIC0

  pop %ax # restore %ax
  sti # enables interrupts after next instruction
  jmp *isr_tmr_eip # jmp to restored eip
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - ierr
.align 16,0x90
isr_err:
  cli
  pop %eax # pop error
  mov %eax,0xa0000 # to screen (top-left)
  lbl3:
    incl 0xa0004
  jmp lbl3
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - ikbd
.align 16,0x90
isr_kbd:
  cli # disable interrupts
  push %ax # save to be clobbered ax
  in $0x60,%al # read keyboard port
  mov %al,osca_key # store
  mov %al,0xa0000+320-8 # on screen
  pusha # save registers
  call osca_keyb_ev # call keyboard handler C function
  popa # restore register
  mov $0x20,%al # ack interrupt
  out %al,$0x20 # ...
  pop %ax # restore ax
  sti # enables interrupts after next instruction
  iret
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - itck
.align 16,0x90
isr_tck:
  cli # disable interrupts
  incl 0xa0000+320-4 # on screen, top right corner
  push %ax # ack irq
  mov $0x20,%al
  out %al,$0x20
  pop %ax
  sti # enables interrupts after next instruction
  iret
.space sector2+512-.,0X90
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - tasks
sector3: # 0x8000 tasks state table
tsk: # eip,  esp,       eflags,    bits,       edi        esi        ebp        esp0       ebx        edx        ecx        eax
 .long tsk0 ,0x000afa00,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
 .long tsk1 ,0x000af780,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk2 ,0x000af500,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
#  .long tsk3 ,0x000af280,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
 .long tsk4 ,0x000af000,0x00000000,0x00000000, 0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
tsk_eot:
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
.space sector3+512-.,0X90
