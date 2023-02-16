#pragma once

// called from osca.S before starting tasks
extern "C" void osca_init();

// called from keyboard interrupt when new scan code from keyboard
// callback from osca.S:isr_kbd
extern "C" void osca_keyb_ev();

// symbols exported from osca.S

// last received scan code
extern "C" volatile const unsigned char osca_key;

// time lower 32b
extern "C" volatile const unsigned osca_tmr_lo;

// time higher 32b
extern "C" volatile const unsigned osca_tmr_hi;

// switches to next task
extern "C" void osca_yield();

// halts the system
extern "C" inline void osca_halt(){asm("cli;hlt");}
