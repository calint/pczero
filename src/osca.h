#pragma once
namespace osca{
using TaskBits=unsigned short;
using TaskId=unsigned short;
using Register=int;
struct Task{
	Register eip{0};
	Register esp{0};
	Register eflags{0};
	TaskBits bits{0};
	TaskId id{0};
	Register edi{0};
	Register esi{0};
	Register ebp{0};
	Register esp0{0};
	Register ebx{0};
	Register edx{0};
	Register ecx{0};
	Register eax{0};

	constexpr inline TaskId get_id()const{return id;}
//	constexpr inline TaskBits get_bits()const{return bits;}
	constexpr inline bool is_grab_keyboard_focus()const{return bits&1;}
	constexpr inline bool is_running()const{return bits&2;}
	constexpr inline void set_running(const bool b){
		if(b){
			bits|=2;
		}else{
			bits&=0xffff-2;
		}
	}
};

// tasks list implemented in kernel.h
// used by osca.S in osca_yield and isr_tmr
extern "C" struct Task osca_tasks[];
extern "C" struct Task*osca_tasks_end;

// halts the system
inline void osca_hang(){asm("cli;hlt");}

// rest of time slice is spent in halt
inline void osca_halt(){asm("hlt");}

// space for interrupt to happen
inline void osca_nop(){asm("nop");}

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
} // end namespace osca
