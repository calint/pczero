#pragma once

// callback from _osca.S
extern "C" void osca_keyb_ev();// called from keyboard interrupt when new keycode from keyboard

// symbols exported from _osca.S
extern "C" unsigned char osca_key;// last received keycode
extern "C" unsigned int osca_t;// time lower 32b
extern "C" unsigned int osca_t1;// time higher 32b

// functions
extern "C" inline void osca_yield(){asm("hlt");}// ? do task switch