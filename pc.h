extern "C" void osca_keyb_ev();//called from keyboard interrupt when new keycode from keyboard
extern "C" volatile const int osca_key;//last received keycode (char)
extern "C" volatile const int osca_t;//time lower 32b
extern "C" volatile const int osca_t1;//time higher 32b
extern "C" inline void osca_yield(){asm("hlt");}
