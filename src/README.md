# table of contents
* osca.S
* osca.hpp
* lib.hpp
* kernel.hpp
* libge.hpp
* game.hpp
* main.cpp

## osca.S
* assembler
* bootable binary
* loads 63½ KB for a total program size of 64 KB
* sets vga to video mode 320x200x8 starting at a0000h
* sets cpu in 32 bit protected mode
* maps interrupts to handlers
    * 1024 Hz timer
    * keyboard
* runs tasks
    * preemptive task switch at 128 Hz

## osca.hpp
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
* declares callback functions to be implemented in c++ by kernel.hpp
  * osca_init
  * osca_on_key

## lib.hpp
* library of classes and functions used by kernel.hpp and applications
* declares globals for printing text to display
  * vga13h
  * out
  * err

## kernel.hpp
* defines tasks to run
* implements memory allocation
* keyboard handling
* task focus switch
* task activation/deactivation

## libge.hpp
* game engine library

## game.hpp
* sample game featuring a targeting system

## main.cpp
* sample tasks