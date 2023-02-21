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
// note. single small task yielding in a tight loop might inhibit
// interrupts due to most time being spent in non-interruptable
// task switching code
extern "C" void osca_yield();

// halts the system
extern "C" inline void osca_halt(){asm("cli;hlt");}

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

	TaskId get_id(){
		return static_cast<TaskId>(bits);
	}
	TaskBits get_bits(){
		return bits;
	}
};

extern "C" Task*osca_active_task;
extern "C" Task*osca_tasks;
extern "C" Task*osca_tasks_end;
}
