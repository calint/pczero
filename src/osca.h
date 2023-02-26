#pragma once
namespace osca{
using TaskBits=unsigned short;
using TaskId=unsigned short;
using Register=int;
struct alignas(16) Task{
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
	// note. The FSSAVE instruction saves a 108-byte data structure to memory (fpu_state), with the
	//       first byte of the structure needing to be aligned on a 16-byte boundary.
	char fpu_state[108]{};
	char padding0{0};
	char padding1{0};
	char padding2{0};
	char padding3{0};

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
inline auto osca_disable_interrupts()->void{asm("cli");}

// enables interrupts
inline auto osca_enable_interrupts()->void{asm("sti");}

//
// callbacks from osca.S
//

// called from osca.S before starting tasks
extern "C" void osca_init();

// called from osca.S:isr_kbd
// keyboard interrupt when new scan code from keyboard
extern "C" void osca_keyb_ev();

// called when interrupt other than keyboard and timer
extern "C" void osca_exception();

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
