# tested on:
# * qemu 7.0.0 on linux 5.19
# * asus zenbook
#
# 00000-003FF  IVT (Interrupt Vector Table)
# 00400-005FF  BDA (BIOS Data Area)
# 00600-9FFFF  Ordinary application RAM
# 00600-0077F    osca IDT (Interrupt Descriptor Table)
# 00780-07BFF    free
# 07C00-07FFF    osca kernel
# 08000-         application
# A0000-BFFFF  Video memory
# C0000-EFFFF  Optional ROMs (The VGA ROM is usually located at C0000)
# F0000-FFFFF  BIOS ROM
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - constants
.set IDT,0x600          # interrupt descriptor table address
.set IDT_ENTRY_COUNT,48 # number of interrupts registered
                        # 32 reserved exceptions + irq 0..15
.set IDT_ENTRY_SIZE,8   # size of an idt entry
.set FREQ_DIV_AND,8-1 # task switches per second 1024/8=128
                      # note. high task switching frequency
                      # might hang slower hardware
.set TASK_STRUCT_SIZE,48
.set TASK_BIT_FOCUS_KEYBOARD,1
.set TASK_BIT_RUNNING,2
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - exports
.global osca_key # last received keyboard scan code
.global osca_tmr_lo # 1024 Hz timer low bits
.global osca_tmr_hi # 1024 Hz timer high bits
.global osca_yield # export function
.global osca_active_task # pointer to active task
.global _start # export entry point
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - bpb
.code16 # boot in 16 bit mode
_start:
# https://stackoverflow.com/questions/47277702/custom-bootloader-booted-via-usb-drive-produces-incorrect-output-on-some-compute
# jump over BIOS Parameter Block (BPB) because
# that memory may be written to by BIOS when booting from USB
jmp _main
.space 3-(.-_start),0x90 # support 2 or 3 bytes encoded jmp
.space 59,0 # BPB area that may be written to by BIOS
_main:
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - setup
#mov %dl,osca_boot_drive # save boot drive
xor %bx,%bx # clear bx
mov %bx,%ds # data segment
mov %bx,%ss # setup stack
mov $_start,%sp # at 0x7c00
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
# The carry flag will be set if there is any error during the read.
# AH should be set to 0 on success.
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
mov %cr0,%eax # enter 32b protected mode
or $1,%al     #  set PE (Protection Enable) bit
mov %eax,%cr0 #  in CR0 (Control Register 0)
# far jump to selector 08h (offset into GDT, pointing at
# a 32bit PM code segment descriptor) to load CS with
# proper PM32 descriptor
jmp $8,$protected_mode # jmp and flush
# global descriptor table
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
#   enabled by default on non ancient pc?
in $0x92,%al
or $2,%al
out %al,$0x92
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
# disable system timer (IRQ 0)
# https://en.wikibooks.org/wiki/X86_Assembly/Programmable_Interrupt_Controller
in $0x21,%al
or $1,%al
out %al,$0x21
# disable the Non-Maskable Interrupt (NMI) before programming RTC
#   NMI happens when there is a critical hardware failure
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
mov $osca_exception,%eax # store the upper bits of the handler
shr $16,%eax             #  in ax
mov $IDT,%ebx # idt address
mov $IDT_ENTRY_COUNT,%ecx # interrupt count
lbl2:
    movw $osca_exception,(%ebx)    # offset 0..15
    movw $0x0008,       2(%ebx)    # selector in gdt
    movb $0x00,         4(%ebx)    # unused
    movb $0x8e,         5(%ebx)    # type_attrs p,pv0,!s,i32b
    movw %ax,           6(%ebx)    # offfset 16..31
    add $IDT_ENTRY_SIZE,%ebx # increment by the size of the entry
loop lbl2
# setup IRQ 1 (keyboard) and IRQ 8 (RTC timer)
#  only the lower offset set, handlers within 64K
movw $isr_kbd,IDT+(32+1)*IDT_ENTRY_SIZE # IRQ 1: keyboard
movw $isr_tmr,IDT+(32+8)*IDT_ENTRY_SIZE # IRQ 8: timer 1024 Hz
lidt idtr
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - start
call osca_init # calls C function
mov osca_active_task,%ebx # ebx points to active task record
mov (%ebx),%esp  # save task.eip for jump
mov %esp,tmp_eip # ..
mov %ebx,%esp # position stack pointer
add $16,%esp  #   for popa by skipping eip,esp,eflags,bits
popa          #   pop edi,esi,ebp,esp0,ebx,edx,ecx,eax
sub $TASK_STRUCT_SIZE-8,%esp # position stack pointer at task.eflags
popf # restore flags, no op that modifies flags after this
mov -8(%esp),%esp # restore task.esp
sti # enables interrupts after next instruction
jmp *tmp_eip # jmp to saved eip (first task)
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - variables
.align 4
osca_tmr_lo:.long 0x00000000 # 1024 Hz tick lower, exported
osca_tmr_hi:.long 0x00000000 # 1024 Hz tick higher, exported
osca_active_task:.long osca_tasks # pointer to active task, exported
tmp_eax:.long 0x00000000 # used in osca_yield and isr_tmr
tmp_ebx:.long 0x00000000 # ...
tmp_ecx:.long 0x00000000 # ...
tmp_eip:.long 0x00000000 # ...
tmp_eflags:.long 0x00000000 #...
osca_key:.byte 0x00 # last key event, exported
#osca_boot_drive:.byte 0x00 # boot drive
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - partitions
.space _start+436-.,0
.space 10,0 # partition table
.space 16,0 # #1
.space 16,0 # #2
.space 16,0 # #3
.space 16,0 # #4
.word 0xaa55 # pc boot sector signature
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - sector 2
sector2: # 0x7e00
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - yield
.align 4
osca_yield: # switches to next task
  cli # disable interrupts
  pushf # push flags. stack is now ..., EIP, EFLAGS
  incl 0xa0000+320-12 # create output on screen
  # start task switching
  mov %eax,tmp_eax # save eax,ebx,ecx
  mov %ebx,tmp_ebx # ..
  mov %ecx,tmp_ecx # ..
  mov osca_active_task,%ebx # ebx points to current active task
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
  mov 12(%ecx),%ax # get task.bits to check if task is running
  and $TASK_BIT_RUNNING,%ax
  jz osca_yield_next_task
  # switch to task pointed by ecx
  # stack is ..., EIP, EFLAGS
  # save task pointed by ebx
  pop 8(%ebx) # pop eflags to task.eflags
  pop (%ebx) # pop eip to task.eip
  mov %esp,4(%ebx) # save stack pointer to task.esp as it was before the call
  lea TASK_STRUCT_SIZE(%ebx),%esp # save gprs. move esp to end of task record then pusha
  mov %ecx,osca_active_task # save pointer to next task record
  mov tmp_eax,%eax # restore eax
  mov tmp_ebx,%ebx # restore ebx
  mov tmp_ecx,%ecx # restore ecx
  pusha # pushes eax,ecx,edx,ebx,esp0,ebp,esi,edi to task record
  # restore task pointed by ecx
  mov osca_active_task,%ecx
  # restore cpu state
  mov (%ecx),%esp # restore eip
  mov %esp,tmp_eip # save for jump
  mov 8(%ecx),%esp # get eflags
  mov %esp,tmp_eflags # save to restore later
  lea 16(%ecx),%esp # restore gprs
  popa # pop edi,esi,ebp,esp0,ebx,edx,ecx,eax
  mov -(TASK_STRUCT_SIZE-4)(%esp),%esp # restore task esp
  push tmp_eflags # restore eflags
  popf            # ..
#
# ?! hangs after a while in QEMU emulator version 7.0.0 (Debian 1:7.0+dfsg-7ubuntu2.1)
# sometimes with a crash. used in is_tmr and works on asus zenbook
# restore task pointed by ecx
#  mov (%ecx),%esp # get task.eip
#  mov %esp,tmp_eip # save for jump
#  lea 16(%ecx),%esp # restore gprs. position stack pointer for popa by skipping eip,esp,eflags,bits
#  popa # pop edi,esi,ebp,esp0,ebx,edx,ecx,eax
#  sub $TASK_STRUCT_SIZE-8,%esp # position stack pointer at task.eflags
#  popf # restore flags, no op that modifies flags after this
#  mov -8(%esp),%esp # restore task esp

  sti # enables interrupts after next instruction
  jmp *tmp_eip # jmp to restored eip

osca_yield_exit_without_task_switch:
  # restore clobbered registers
  mov tmp_eax,%eax
  mov tmp_ebx,%ebx
  mov tmp_ecx,%ecx
  # stack is ..., EIP, EFLAGS
  popf # restore flags
  sti
  ret
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - interrupts
# "*NOTE* Because IA-32 tasks are not re-entrant, an interrupt-handler
# task must disable interrupts between the time it completes handling
# the interrupt and the time it executes the IRET instruction. This
# action prevents another interrupt from occurring while the interrupt
# task's TSS is still marked busy, which would cause a general-protection
# (#GP) exception."
#
#—Intel Software Developer Manual
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - 1024Hz timer
.align 4
isr_tmr:
  cli # disable interrupts
  incl 0xa0000+320-4 # create output on screen
  incl osca_tmr_lo    # increase lower 32 bits ticker
  adcl $0,osca_tmr_hi # add carry to upper 32 bits
  push %eax
  mov osca_tmr_lo,%eax # switch task every 8'th tick
  and $FREQ_DIV_AND,%eax # 1024/8 Hz
  pop %eax
  jnz isr_tmr_acknowledge_and_return # if no task switch
  # start task switching
  mov %eax,tmp_eax # save eax,ebx,ecx
  mov %ebx,tmp_ebx # ..
  mov %ecx,tmp_ecx # ..
  mov osca_active_task,%ebx # ebx points to current active task
  mov %ebx,%ecx # ecx will point to the next task
  # find next task and store it in %ecx
isr_tmr_next_task:
  add $TASK_STRUCT_SIZE,%ecx # next task
  cmp osca_tasks_end,%ecx # if last
  jl lbl4
    mov $osca_tasks,%ecx # roll
  lbl4:
  cmp %ebx,%ecx # if no active task found
  je isr_tmr_exit_without_task_switch # exit without task switch
  mov 12(%ecx),%ax # get bits to check if task is running
  and $TASK_BIT_RUNNING,%ax # check 'running' bit
  jz isr_tmr_next_task # if not 'running' get next task
  # switch to task pointed by ecx
  # stack is ..., EFLAGS, CS, EIP
  # save task state
  pop (%ebx) # pop eip to task.eip
  add $4,%esp # skip cs
  pop 8(%ebx) # pop eflags to task.eflags
  mov %esp,4(%ebx) # save stack pointer as it was before irq to task.esp
  lea TASK_STRUCT_SIZE(%ebx),%esp # save gprs. move esp to end of task record then pusha
  mov %ecx,osca_active_task # save pointer to next task record
  mov tmp_eax,%eax # save proper eax
  mov tmp_ebx,%ebx # save proper ebx
  mov tmp_ecx,%ecx # save proper ecx
  pusha # pushes eax,ecx,edx,ebx,esp0,ebp,esi,edi to task record
  # restore task pointed by ecx
  mov osca_active_task,%ecx
  mov (%ecx),%esp # get task.eip
  mov %esp,tmp_eip # save for jump
  lea 16(%ecx),%esp # restore gprs. position stack pointer for popa by skipping eip,esp,eflags,bits
  popa # pop edi,esi,ebp,esp0,ebx,edx,ecx,eax
  sub $TASK_STRUCT_SIZE-8,%esp # position stack pointer at task.eflags
  popf # restore flags, no op that modifies flags after this
  mov -8(%esp),%esp # restore task esp
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
  jmp *tmp_eip # jmp to restored eip

isr_tmr_exit_without_task_switch:
  # restore clobbered registers
  mov tmp_eax,%eax
  mov tmp_ebx,%ebx
  mov tmp_ecx,%ecx

isr_tmr_acknowledge_and_return:
  # acknowledge interrupt and exit
  push %ax # save to be clobbered %ax
  mov $0x0c,%al # read register C in ISR
  out %al,$0x70 #  to keep getting
  in $0x71,%al  #  irq 8
  mov $0x20,%al # acknowledge interrupt (EOI) to
  out %al,$0xa0 #  second controller PIC1
  out %al,$0x20 #  main controller PIC0
  pop %ax # restore %ax
  sti # enables interrupts after next instruction
  iret
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - ikbd
.align 4
isr_kbd:
  cli # disable interrupts
  push %ax # save to be clobbered ax
  in $0x60,%al # read keyboard port
  mov %al,osca_key # store
  mov %al,0xa0000+320-8 # create output on screen
  pusha # save registers
  call osca_keyb_ev # call keyboard handler C function
  popa # restore registers
  mov $0x20,%al # ack interrupt
  out %al,$0x20 # ...
  pop %ax # restore ax
  sti # enables interrupts after next instruction
  iret
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - ierr
.align 4
isr_err:
  cli
  pop %eax # pop error
  mov %eax,0xa0000       # to screen (top-left)
  mov %eax,0xa0000+320   # ..
  mov %eax,0xa0000+320*2 # ..
  mov %eax,0xa0000+320*3 # ..
  lbl3:
    incl 0xa0008
  jmp lbl3
.space sector2+512-.,0
##-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - tasks
