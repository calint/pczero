# table of contents
* osca.S
* osca.h
* lib.h
* kernel.h
* libge.h
* game.h
* main.cc

## osca.S
* assembler
* bootable binary
* loads 63½ KB for a total program size of 64 KB
* sets vga to video mode 320x200x8 starting at a0000h
* sets cpu in 32 bit protected mode
* maps interrupts to handlers
    * timer 1024 Hz
    * keyboard
* runs tasks
    * preemptive task switch 128 Hz

## osca.h
* makes variables declared in osca.S available in c++
  * osca_tick
  * osca_tick_lo
  * osca_tick_hi
  * osca_task_active
* declares necessary variables to be provided by c++
  * osca_tasks
  * osca_tasks_end
* implements low level functions to interact with osca.S
  * osca_interrupts_disable
  * osca_interrupts_enable
  * osca_yield
  * osca_hang
* declares callback functions to be implemented in c++ by kernel.h
  * osca_init
  * osca_on_key

## lib.h
* library of classes and functions used by kernel.h and applications
* declares globals for printing text to display
  * vga13h
  * out
  * err

## kernel.h
* implements memory allocation
* keyboard handling
* task focus switch
* task activation/deactivation

## libge.h
* game engine library

## game.h
* sample game featuring a targeting system

## main.cc
* sample tasks