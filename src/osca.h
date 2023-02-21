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
	constexpr inline bool is_grab_keyboard_focus()const{return bits&1;}
	constexpr inline bool is_running()const{return bits&2;}
	constexpr inline void set_running(const bool b){if(b)bits|=2;else bits&=0xffff-2;}
};

// tasks list implemented in kernel.h
// used by osca.S in osca_yield and isr_tmr
extern "C" struct Task osca_tasks[];
// pointer to end of tasks (last entry + 1)
extern "C" struct Task*osca_tasks_end;

// hangs the system
inline auto osca_hang()->void{asm("cli;hlt");}

// rest of time slice is spent in halt
inline auto osca_halt()->void{asm("hlt");}

// nop instruction
inline auto osca_nop()->void{asm("nop");}

// disables interrupts
inline auto osca_cli()->void{asm("cli");}

// enables interrupts
inline auto osca_sti()->void{asm("sti");}

//inline int osca_get_eax(){int r;asm("mov %%eax,%0":"=a"(r));return r;}
//inline int osca_get_ebx(){int r;asm("mov %%ebx,%0":"=b"(r));return r;}
//inline int osca_get_ecx(){int r;asm("mov %%ecx,%0":"=c"(r));return r;}
//inline int osca_get_edx(){int r;asm("mov %%edx,%0":"=d"(r));return r;}
//inline int osca_get_esi(){int r;asm("mov %%esi,%0":"=S"(r));return r;}
//inline int osca_get_edi(){int r;asm("mov %%edi,%0":"=D"(r));return r;}
//inline int osca_get_eax(){int r;asm("mov %%eax,%0":"=m"(r));return r;}
//inline int osca_get_ebx(){int r;asm("mov %%ebx,%0":"=m"(r));return r;}
//inline int osca_get_ecx(){int r;asm("mov %%ecx,%0":"=m"(r));return r;}
//inline int osca_get_edx(){int r;asm("mov %%edx,%0":"=m"(r));return r;}
//inline int osca_get_esi(){int r;asm("mov %%esi,%0":"=m"(r));return r;}
//inline int osca_get_edi(){int r;asm("mov %%edi,%0":"=m"(r));return r;}

//
// callbacks from osca.S
//

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
