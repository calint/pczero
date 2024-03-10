#pragma once
// reviewed: 2024-03-09

//
// osca kernel header
//

namespace osca{

using TaskBits=short;
using TaskId=short;
using Register=int;
using Byte=unsigned char;

struct alignas(16) Task{
	Register eip{};
	Register esp{};
	Register eflags{};
	TaskBits bits{}; // 1: accepts keyboard focus, 2: is running
	TaskId id{};
	Register edi{};
	Register esi{};
	Register ebp{};
	Register esp_unused{};
	Register ebx{};
	Register edx{};
	Register ecx{};
	Register eax{};
	// note: The FSAVE instruction saves a 108-byte data structure to memory (fpu_state), with the
	//       first byte of the structure needed to be aligned on a 16-byte boundary.
	alignas(16) Byte fpu_state[108]{};
	Byte padding[4]{};

	constexpr inline auto get_id()const->TaskId{return id;}
	constexpr inline auto is_grab_keyboard_focus()const->bool{return bits&1;}
	constexpr inline auto is_running()const->bool{return bits&2;}
	constexpr inline auto set_running(const bool b)->void{if(b)bits|=2;else bits&=~2;}
};

// tasks list implemented in kernel.h
// used from osca.S in '_main', 'osca_yield', 'isr_tmr' and 'isr_fpu'
extern "C" struct Task osca_tasks[];
// pointer to end of tasks (1 past last entry)
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
// exported from osca.S
//

// current active task
extern "C" const Task*osca_task_active;

// time lower 32b
extern "C" volatile const unsigned osca_tmr_lo;

// time higher 32b
extern "C" volatile const unsigned osca_tmr_hi;

// switches to next task
// note: single small task yielding in a tight loop might inhibit
//       interrupts due to most time being spent in non-interruptable
//       task switching code
extern "C" auto osca_yield()->void;

//
// callbacks from osca.S
//

// called before starting tasks
extern "C" auto osca_init()->void;

// called from 'isr_kbd' when a key is pressed or released
extern "C" auto osca_on_key(unsigned scan_code)->void;

// called when interrupt other than keyboard or timer
extern "C" auto osca_exception()->void;

} // end namespace osca
