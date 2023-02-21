#pragma once
namespace osca{
using TaskBits=unsigned;
using TaskId=unsigned short;
using Register=int;
struct Task{
	Register eip{0};
	Register esp{0};
	Register eflags{0};
	TaskBits bits{0};
	Register edi{0};
	Register esi{0};
	Register ebp{0};
	Register esp0{0};
	Register ebx{0};
	Register edx{0};
	Register ecx{0};
	Register eax{0};

	constexpr inline TaskId get_id()const{return static_cast<TaskId>(bits);}
	constexpr inline TaskBits get_bits()const{return bits;}
};

// halts the system
inline void osca_halt(){asm("cli;hlt");}

// called from osca.S before starting tasks
extern "C" void osca_init();
// called from osca.S:isr_kbd
// keyboard interrupt when new scan code from keyboard
extern "C" void osca_keyb_ev();

//
// exported from osca.S
//
// current active task
extern "C" Task*osca_active_task;
// last received scan code
extern "C" volatile const unsigned char osca_key;
// time lower 32b
extern "C" volatile const unsigned osca_tmr_lo;
// time higher 32b
extern "C" volatile const unsigned osca_tmr_hi;
// switches to next task
// note. single small task yielding in a tight loop might inhibit
// interrupts due to most time being spent in non-interruptable
// task switching code
extern "C" void osca_yield();
}
