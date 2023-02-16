#pragma once

// called from osca.S before starting tasks
extern "C" void osca_init(); // called from keyboard interrupt when new scan code from keyboard

// callback from osca.S:isr_kbd
extern "C" void osca_keyb_ev(); // called from keyboard interrupt when new scan code from keyboard

// symbols exported from osca.S
extern "C" volatile const unsigned char osca_key; // last received keycode
//extern "C" volatile const unsigned osca_t; // time lower 32b
//extern "C" volatile const unsigned osca_th; // time higher 32b
extern "C" volatile const unsigned osca_tmr_lo; // time lower 32b
extern "C" volatile const unsigned osca_tmr_hi; // time higher 32b

// functions
extern "C" void osca_yield(); // switches to next task
extern "C" inline void osca_halt(){asm("cli;hlt");}
