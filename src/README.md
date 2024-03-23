# table of contents
## osca.S
* assembler
* bootable binary
* loads 63½ KB for a total program size of 64 KB
* sets video mode to 320x200x8 starting at a0000h
* sets the cpu in 32 bit protected mode
* maps interrupts to handlers
    * timer 1024 Hz
    * keyboard
* runs tasks
    * preemptive task switch 128 Hz

## osca.h
* makes variables declared in osca.S available in c++
* declares necessary variables to be provided by c++
  * task list
* implements low level functions to interact with osca.S
  * osca_interrupts_disable
  * osca_interrupts_enable
  * osca_yield
* declares callback functions to be implemented in c++ by kernel.h
  * osca_init
  * osca_on_key
  * osca_on_exception

## lib.h
* library of classes and functions used by kernel.h and applications
* declares globals for printing text to display

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