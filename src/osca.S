##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - 
# kernel osca
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - info
# reviewed: 2024-03-10
# tested on:
# * linux 6.5.0 qemu-system-i386 8.0.4
# * asus zenbook UX305CA
#
# memory map from https://wiki.osdev.org/Memory_Map_(x86)
# ------------+--------+---------------------------------------
# 00000-003FF |   1 KB | Real Mode IVT (Interrupt Vector Table)
# 00400-004FF | 256 B  | BDA (BIOS data area)
# 00500-0067F | 384 B  | osca IDT (Interrupt Descriptor Table)
# 00680-077FF | ~28 KB | free
# 07800-07BFF |   1 KB | osca stack
# 07C00-07FFF |   1 KB | osca kernel
# 08000-7FFFF | 480 KB | application
# 80000-9FFFF | 128 KB | EBDA (Extended BIOS Data Area)
# A0000-BFFFF |	128 KB | Video display memory
# C0000-C7FFF |	 32 KB | Video BIOS
# C8000-EFFFF |	160 KB | BIOS Expansions
# F0000-FFFFF |  64 KB | Motherboard BIOS
#        
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - constants
.set IDT,0x500          # interrupt descriptor table address
.set IDT_ENTRY_COUNT,48 # number of interrupts registered
                        # 32 reserved exceptions + irq 0..15
.set IDT_ENTRY_SIZE,8   # size of an idt entry
.set FREQ_DIV_AND,8-1 # task switches per second 1024/8=128
                      # note: high task switching frequency
                      #       might hang slower hardware
.set TASK_STRUCT_OFFSET_EIP,0         # see osca.h: struct Task
.set TASK_STRUCT_OFFSET_ESP,4         # ..
.set TASK_STRUCT_OFFSET_EFLAGS,8      # ..
.set TASK_STRUCT_OFFSET_EFLAGS_END,12 # ..
.set TASK_STRUCT_OFFSET_BITS,12       # ..
.set TASK_STRUCT_OFFSET_GPRS,16       # ..
.set TASK_STRUCT_OFFSET_GPRS_END,48   # ..
.set TASK_STRUCT_OFFSET_FPU_STATE,48  # ..
#.set TASK_STRUCT_SIZE,TASK_STRUCT_OFFSET_FPU_STATE+94+2
# from 80386 SYSTEM SOFTWARE WRITER'S GUIDE 1987
#   7.1.2.1 COPROCESSOR CONTEXT SWITCHING
#   The save area should be 94 bytes, the size of the area used
#   by the FSAVE and FRSTOR instructions.
#
# from you.com:
#   The FSAVE instruction writes a total of 108 bytes to the
#   memory address specified.
#
# from Intel® 64 and IA-32 Architectures Software Developer’s Manual
#   page 3-424 Vol. 2A: FSAVE/FNSAVE—Store x87 FPU State
#   FSAVE m94/108byte Store FPU state to m94byte or m108byte
.set TASK_STRUCT_SIZE,TASK_STRUCT_OFFSET_FPU_STATE+108+4
.set TASK_BIT_FOCUS_KEYBOARD,1
.set TASK_BIT_RUNNING,2
.set EFLAGS_BIT_IF,0x200 # the 'Interrupt enable flag' bit
.set CR0_BIT_PE,1 # Protected mode enable bit
.set CR0_BIT_MP,2 # Monitor co-processor bit
.set CR0_BIT_TS,8 # Task switched bit
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - exports
.global osca_tick   # uint64 tick of 1024 Hz timer
.global osca_tick_lo # 1024 Hz timer low bits
.global osca_tick_hi # 1024 Hz timer high bits
.global osca_yield # export function
.global osca_task_active # pointer to active task
.global _start # export entry point
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - bpb
.code16 # boot in 16 bit mode
_start:
# https://stackoverflow.com/questions/47277702/custom-bootloader-booted-via-usb-drive-produces-incorrect-output-on-some-compute
# jump over BIOS Parameter Block (BPB) because
#  that memory may be written to by BIOS when booting from USB
jmp _main
.space 3-(.-_start),0x90 # support 2 or 3 bytes encoded jmp
.space 59,0x90 # BPB area that may be written to by BIOS
_main:
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - setup
#mov %dl,osca_boot_drive # save boot drive
xor %bx,%bx # clear bx
mov %bx,%ds # data segment
mov %bx,%ss     # setup stack
mov $_start,%sp #  at 0x7c00
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - load
# https://wiki.osdev.org/Disk_access_using_the_BIOS_(INT_13h)
# Reading sectors with a CHS address
#   Cylinder = 0 to 1023 (maybe 4095),
#   Head = 0 to 15 (maybe 254, maybe 255),
#   Sector = 1 to 63
# AH = 2
# AL = total sector count (0 is illegal) --
#      cannot cross ES page boundary, or a cylinder boundary,
#      and must be < 128
# CH = cylinder & 0xff
# CL = Sector | ((cylinder >> 2) & 0xC0);
# DH = Head -- may include two more cylinder bits
# ES:BX -> buffer
# Set DL = "drive number" -- typically 0x80, for the "C" drive
# INT 0x13.
# The carry flag will be set if there is any error during the
# read.
# AH should be set to 0 on success.
#
cld # clear direction (forward)
# read 127 sectors * 512B = 65024 bytes
#  assumes load from USB. floppy does not support that many
#   sectors.
# command 2 (%ah), number of sectors to read (%al)
mov $0x027f,%ax
# from cylinder 0 (%ch), sector 2 (%cl)
#  sector starts from 1, not 0
mov $0x0002,%cx
xor %dh,%dh # head 0
# %dl, drive number, is unchanged
mov $0x07e0,%bx # to es:bx=7e00:0000
mov %bx,%es     #
xor %bx,%bx     #
int $0x13 # read disk
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
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - to protected mode
# enable 32bit protected mode
# https://wiki.osdev.org/Protected_Mode
cli # done with bios, disable interrupts
lgdt gdtr # load global descriptor tables
mov %cr0,%eax      # enter 32b protected mode
or $CR0_BIT_PE,%al #  set 'Protection Enable' bit
mov %eax,%cr0      #   in CR0 (Control Register 0)
# far jump to selector 08h (offset into GDT, pointing at a
#  32bit PM code segment descriptor) to load CS with proper
#    PM32 descriptor
jmp $8,$protected_mode # jmp and flush
# global descriptor table
# https://wiki.osdev.org/Global_Descriptor_Table
# https://wiki.osdev.org/GDT_Tutorial
.align 4
gdt:.quad 0x0000000000000000 #0x00: reserved and unused
    .quad 0x00cf9a000000ffff #0x08: 32b code 4g pl0 rx
    .quad 0x00cf92000000ffff #0x10: 32b data 4g pl0 rw
gdt_end:
# global descriptor table register
gdtr:.word gdt_end-gdt-1 # limit of table (size of table - 1)
     .long gdt           # address
# interrupt descriptor table register
idtr:.word IDT_ENTRY_COUNT*IDT_ENTRY_SIZE-1 # limit of table
     .long IDT                              # idt address
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - 32b code
protected_mode:
.code32
.align 4
mov $0x10,%ax # offset in gdt, the data segment
mov %ax,%ds
mov %ax,%es
mov %ax,%fs
mov %ax,%gs
mov %ax,%ss
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - a20
# enable access to odd megs
# https://wiki.osdev.org/A20_Line
# fast A20 gate method (since IBM PS/2)
#   enabled by default on non ancient pc?
in $0x92,%al
or $2,%al
out %al,$0x92
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - map irqs
# reprogram the PIC (8259's) to map IRQ 0-15 to IDT mapping 32
# to 47 avoiding the first 32 reserved for exceptions
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
# disable system timer (IRQ 0)
# https://en.wikibooks.org/wiki/X86_Assembly/Programmable_Interrupt_Controller
in $0x21,%al
or $1,%al
out %al,$0x21
# disable the Non-Maskable Interrupt (NMI) before programming
#   RTC. NMI happens when there is a critical hardware failure
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
# programming RTC done. enable NMI
in $0x70,%al
and $0x7f,%al
out %al,$0x70
in $0x71,%al
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - isr
# setup interrupt service routines
# https://wiki.osdev.org/Interrupt_Descriptor_Table
mov $isr_other,%eax # store the upper 16 bits of the handler
shr $16,%eax        #  address in ax
mov $IDT,%ebx       # interrupt descriptor table address
mov $IDT_ENTRY_COUNT,%ecx # interrupt count
lp1:
    movw $isr_other,        (%ebx)    # offset 0..15
    movw $0x0008,          2(%ebx)    # selector in gdt
    movb $0x00,            4(%ebx)    # unused
    movb $0x8e,            5(%ebx)    # type_attrs p,pv0,!s,i32b
    mov  %ax,              6(%ebx)    # offfset 16..31
    add $IDT_ENTRY_SIZE,%ebx
loop lp1
# setup IRQ 1 (keyboard) and IRQ 8 (RTC timer)
#  high 16 bits (IDT_ENTRY_SIZE+6) 0 because handler is within 64K
movw $isr_kbd,IDT+(32+1)*IDT_ENTRY_SIZE   # IRQ 1: keyboard
movw       $0,IDT+(32+1)*IDT_ENTRY_SIZE+6 #  offset 16..31
movw $isr_tmr,IDT+(32+8)*IDT_ENTRY_SIZE   # IRQ 8: 1024 Hz timer
movw       $0,IDT+(32+8)*IDT_ENTRY_SIZE+6 #  offset 16..31
# exception 7: device not available, triggered when a FPU
#  context switch might be needed
movw $isr_fpu,IDT+     7*IDT_ENTRY_SIZE
movw       $0,IDT+     7*IDT_ENTRY_SIZE+6 #  offset 16..31
# note: asus zenbook UX305CA generates ISA interrupt 7:
#       LPT1 / Unreliable "spurious" interrupt (usually)
#       see https://wiki.osdev.org/8259_PIC#Spurious_IRQs 
#       disabled with an empty interrupt handler: isr_7
#       (qemu and virtualbox does not generate this interrupt)
movw $isr_7,IDT+(32+7)*IDT_ENTRY_SIZE   # 
movw     $0,IDT+(32+7)*IDT_ENTRY_SIZE+6 #

lidt idtr # load the interrupt descriptor table register
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - fpu
# 80386 SYSTEM SOFTWARE WRITER'S GUIDE 1987
# 7.1.2.1 COPROCESSOR CONTEXT SWITCHING page 122
mov %cr0,%eax      # enable MP (monitor co-processor) bit in CR0
or $CR0_BIT_MP,%al #  if TS (task switched) bit is set and a FPU
mov %eax,%cr0      #   instruction is used then exception 7 is triggered
# initiate the tasks' FPU state with initial values
fninit # initiate FPU
mov $osca_tasks,%ebx # for each task
lp2:
  fsave TASK_STRUCT_OFFSET_FPU_STATE(%ebx) # store initiated fpu state
  add $TASK_STRUCT_SIZE,%ebx
  cmp osca_tasks_end,%ebx
  jl lp2
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - start
call osca_init # calls C function declared in osca.h
# restore cpu state for first task
mov osca_task_active,%ebx # ebx points to first task
mov TASK_STRUCT_OFFSET_EIP(%ebx),%esp  # save task.eip for jump
mov %esp,tmp_eip
# position esp for popa of registers
lea TASK_STRUCT_OFFSET_GPRS(%ebx),%esp
popa # pop edi,esi,ebp,esp0,ebx,edx,ecx,eax
# position stack pointer at task.eflags
sub $(TASK_STRUCT_OFFSET_GPRS_END-TASK_STRUCT_OFFSET_EFLAGS),%esp
popf # restore flags, no op that modifies flags after this
# restore task.esp
mov (-TASK_STRUCT_OFFSET_EFLAGS_END+TASK_STRUCT_OFFSET_ESP)(%esp),%esp
sti # enables interrupts after next instruction
jmp *tmp_eip # jmp to saved eip (first task)
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - spurios
.align 4
isr_7: # see https://wiki.osdev.org/8259_PIC#Spurious_IRQs
  iret
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - variables
.align 4
osca_tick:           # timer tick (1024 Hz), 64 bit, exported
osca_tick_lo:.long 0 #   low bits, exported
osca_tick_hi:.long 0 #   high bits, exported
osca_task_active:.long osca_tasks # pointer to active task, exported
osca_active_fpu:.long osca_tasks # pointer to task that has the FPU
tmp_eax:.long 0 # used in osca_yield and isr_tmr
tmp_ebx:.long 0 # ...
tmp_ecx:.long 0 # ...
tmp_eip:.long 0 # ...
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - signature
.space _start+510-.,0xf
.word 0xaa55 # pc boot sector signature
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - sector 2
sector2: # 0x7e00
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - yield
.align 4
osca_yield: # switches to next task
  cli # disable interrupts
  pushf # push flags. stack is now ..., EIP, EFLAGS
        # note: EFLAGS 'Interrupt enable flag' is off
        #       so popf in cli/sti section is safe
  incl 0xa0000+320-16 # create output on screen
  # start task switching
  mov %eax,tmp_eax # save eax,ebx,ecx
  mov %ebx,tmp_ebx # ..
  mov %ecx,tmp_ecx # ..
  mov osca_task_active,%ebx # ebx points to current active task
  mov %ebx,%ecx # ecx will point to the next task
  # find next task and store it in %ecx
osca_yield_next_task:
  add $TASK_STRUCT_SIZE,%ecx # next task
  cmp osca_tasks_end,%ecx # if last
  jl lbl1
    mov $osca_tasks,%ecx # roll
  lbl1:
  cmp %ebx,%ecx # if no active task found
  je osca_yield_exit_without_task_switch
  # get task.bits to check if task is running
  mov TASK_STRUCT_OFFSET_BITS(%ecx),%ax
  and $TASK_BIT_RUNNING,%ax
  jz osca_yield_next_task

  # enable TS (Task switched) bit in cr0
  # when this bit is enabled and a numeric (floating point)
  # operation executes the cpu triggers exception 7 which
  # calls 'isr_fpu' that restores, if necessary, fpu state 
  # of current running task.
  # 80386 SYSTEM SOFTWARE WRITER'S GUIDE 1987
  # 7.1.2.1 COPROCESSOR CONTEXT SWITCHING page 122
  mov %cr0,%eax
  or $CR0_BIT_TS,%al
  mov %eax,%cr0

  # switch from task pointed by ebx to task pointed by ecx
  # stack is ..., EIP, EFLAGS
  # save cpu state of task pointed by ebx
  pop TASK_STRUCT_OFFSET_EFLAGS(%ebx) # pop eflags to task.eflags
  pop TASK_STRUCT_OFFSET_EIP(%ebx) # pop eip to task.eip
  # save stack pointer to task.esp as it was before the call
  mov %esp,TASK_STRUCT_OFFSET_ESP(%ebx)
  # save gprs. move esp to end of gprs then pusha
  lea TASK_STRUCT_OFFSET_GPRS_END(%ebx),%esp
  mov %ecx,osca_task_active # save pointer to next task record
  mov tmp_eax,%eax # restore eax
  mov tmp_ebx,%ebx # restore ebx
  mov tmp_ecx,%ecx # restore ecx
  pusha # pushes eax,ecx,edx,ebx,esp0,ebp,esi,edi to task record

  # restore cpu state of task pointed by ecx
  mov osca_task_active,%ecx
  mov TASK_STRUCT_OFFSET_EIP(%ecx),%esp # get task.eip
  mov %esp,tmp_eip # save for jump
  # restore gprs. position stack pointer for popa by skipping
  #  eip,esp,eflags,bits
  lea TASK_STRUCT_OFFSET_GPRS(%ecx),%esp
  popa # pop edi,esi,ebp,esp0,ebx,edx,ecx,eax
  # position stack pointer at task.eflags
  sub $(TASK_STRUCT_OFFSET_GPRS_END-TASK_STRUCT_OFFSET_EFLAGS),%esp
  popf # restore flags, no op that modifies flags after this
  # restore task esp
  mov (-TASK_STRUCT_OFFSET_EFLAGS_END+TASK_STRUCT_OFFSET_ESP)(%esp),%esp

  # task switch done
  sti # enables interrupts after next instruction
  jmp *tmp_eip # jmp to restored eip

osca_yield_exit_without_task_switch:
  # restore clobbered registers
  mov tmp_eax,%eax
  mov tmp_ebx,%ebx
  mov tmp_ecx,%ecx
  # stack is ..., EIP, EFLAGS
  popf # restore flags
  sti # enables interrupts after next instruction
  ret
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - interrupts
# "*NOTE* Because IA-32 tasks are not re-entrant, an
# interrupt-handler task must disable interrupts between the
# time it completes handling the interrupt and the time it
# executes the IRET instruction. This action prevents another
# interrupt from occurring while the interrupt task's TSS is
# still marked busy, which would cause a general-protection
# (#GP) exception."
#
# —Intel Software Developer Manual
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - 1024Hz timer
.align 4
isr_tmr:
  cli # disable interrupts
  incl 0xa0000+320-8 # create output on screen
  addl $1,osca_tick_lo # increase lower 32 bits ticker
  adcl $0,osca_tick_hi # add carry to upper 32 bits
  mov %eax,tmp_eax # save eax
  mov osca_tick_lo,%eax # switch task every 8'th tick
  and $FREQ_DIV_AND,%eax # 1024/8 Hz
  jnz isr_tmr_acknowledge_and_return # if no task switch
  # start task switching
  mov %ebx,tmp_ebx # save ebx
  mov %ecx,tmp_ecx # save ecx
  mov osca_task_active,%ebx
  mov %ebx,%ecx # ecx will point to the next task
  # find next task and store it in ecx
isr_tmr_next_task:
  add $TASK_STRUCT_SIZE,%ecx # next task
  cmp osca_tasks_end,%ecx # if last
  jl lbl2
    mov $osca_tasks,%ecx # roll
  lbl2:
  cmp %ebx,%ecx # if no active task found
  je isr_tmr_exit_without_task_switch
  # get bits to check if task is running
  mov TASK_STRUCT_OFFSET_BITS(%ecx),%ax
  and $TASK_BIT_RUNNING,%ax # check 'running' bit
  jz isr_tmr_next_task # if not 'running' get next task

  # enable TS (Task switched) bit in cr0
  # (see comment in osca_yield_next_task)
  mov %cr0,%eax
  or $CR0_BIT_TS,%al
  mov %eax,%cr0

  # switch from task pointed to by ebx to task pointed by ecx
  # stack is ..., EFLAGS, CS, EIP
  # save cpu state of task pointed by ebx
  pop TASK_STRUCT_OFFSET_EIP(%ebx) # pop eip to task.eip
  add $4,%esp # skip cs
  pop TASK_STRUCT_OFFSET_EFLAGS(%ebx) # pop eflags to task.eflags
  # clear the 'enable interrupts' flag so that popf is safe in
  #  a cli/sti section
  andl $~EFLAGS_BIT_IF,TASK_STRUCT_OFFSET_EFLAGS(%ebx)
  # save stack pointer as it was before irq to task.esp
  mov %esp,TASK_STRUCT_OFFSET_ESP(%ebx)
  # save gprs. move esp to end of gprs then pusha
  lea TASK_STRUCT_OFFSET_GPRS_END(%ebx),%esp
  mov %ecx,osca_task_active # save pointer to next task record
  mov tmp_eax,%eax # save proper eax
  mov tmp_ebx,%ebx # save proper ebx
  mov tmp_ecx,%ecx # save proper ecx
  pusha # pushes eax,ecx,edx,ebx,esp0,ebp,esi,edi to task record
  # restore cpu state of task pointed by ecx
  mov osca_task_active,%ecx
  mov TASK_STRUCT_OFFSET_EIP(%ecx),%esp # get task.eip
  mov %esp,tmp_eip # save for jump
  # restore gprs. position stack pointer for popa by skipping
  #  eip,esp,eflags,bits
  lea TASK_STRUCT_OFFSET_GPRS(%ecx),%esp
  popa # pop edi,esi,ebp,esp0,ebx,edx,ecx,eax
  # position stack pointer at task.eflags
  sub $(TASK_STRUCT_OFFSET_GPRS_END-TASK_STRUCT_OFFSET_EFLAGS),%esp
  popf # restore flags, no op that modifies flags after this
  # restore task esp
  mov (-TASK_STRUCT_OFFSET_EFLAGS_END+TASK_STRUCT_OFFSET_ESP)(%esp),%esp
  # task switch done

  # acknowledge interrupt and jmp
  # https://forum.osdev.org/viewtopic.php?f=1&t=15526&start=0
  # https://www.compuphase.com/int70.txt
  # "When an IRQ 8 fires and interrupt 70h is called, status
  # register C holds a bit mask that tells what kind of
  # interrupt occurred: periodic interrupt, alarm interrupt or
  # update ended interrupt. And unless you read status
  # register C, IRQ 8 will not be generated again. This means
  # that you must read status register C inside your ISR for
  # interrupt 70h even when you normally don't care about its
  # contents. Otherwise you will only see a single interrupt."
  push %eax # save to be clobbered ax
  mov $0x0c,%al # read register C in ISR
  out %al,$0x70 #  to keep getting
  in $0x71,%al  #   irq 8
  mov $0x20,%al # acknowledge interrupt (EOI) to
  out %al,$0xa0 #  second controller PIC1
  out %al,$0x20 #   main controller PIC0
  pop %eax # restore ax
  sti # enables interrupts after next instruction
  jmp *tmp_eip # jmp to restored eip

isr_tmr_exit_without_task_switch:
  # restore clobbered registers
  mov tmp_ebx,%ebx
  mov tmp_ecx,%ecx

isr_tmr_acknowledge_and_return:
  # acknowledge interrupt and exit
  mov $0x0c,%al # read register C in ISR
  out %al,$0x70 #  to keep getting
  in $0x71,%al  #   irq 8
  mov $0x20,%al # acknowledge interrupt (EOI) to
  out %al,$0xa0 #  second controller PIC1
  out %al,$0x20 #   main controller PIC0
  mov tmp_eax,%eax # restore clobbered register
  sti # enables interrupts after next instruction
  iret

##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - keyboard
.align 4
isr_kbd:
  cli # disable interrupts
  mov %esp,tmp_ecx # save current task stack pointer
  mov $_start,%esp # use osca stack
  pusha # save registers
  xor %eax,%eax # clear eax
  in $0x60,%al # read keyboard port
  mov %al,0xa0000+320-1 # create output on screen
  push %eax # function argument
  call osca_on_key #  callback c function
  pop %eax # restore stack
  mov $0x20,%al # ack interrupt
  out %al,$0x20 # ...
  popa # restore registers
  mov tmp_ecx,%esp # restore task stack pointer
  sti # enables interrupts after next instruction
  iret

##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - fpu state switch
.align 4
isr_fpu:
  cli # disable interrupts
  mov %eax,tmp_eax # save clobbered eax
  clts # reset the TS (Task Switched) flag in CR0
  # is the active task same as active FPU state?
  mov osca_task_active,%eax
  cmp osca_active_fpu,%eax
  je isr_fpu_done
    # not same task, switch FPU state
    incl 0xa0000+320-24 # on screen
    mov %ebx,tmp_ebx # save clobbered ebx
    mov osca_active_fpu,%ebx
    fsave TASK_STRUCT_OFFSET_FPU_STATE(%ebx) # save state
    frstor TASK_STRUCT_OFFSET_FPU_STATE(%eax) # restore state
    mov %eax,osca_active_fpu # set the active FPU task
    mov tmp_ebx,%ebx # restore ebx

isr_fpu_done:
  mov tmp_eax,%eax # restore eax
  sti # enables interrupts after next instruction
  iret

##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - other
.align 4
isr_other:
  cli
  incl 0xa0000+320-32 # on screen
  sti
  iret

##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - done
.space sector2+512-.,0xf
