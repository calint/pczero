#pragma once
// reviewed: 2024-03-09

//
// osca kernel
//

#include"osca.h"
#include"lib.h"

// sample tasks implemented in 'main.cc'
extern "C" [[noreturn]] auto tsk0()->void;
extern "C" [[noreturn]] auto tsk1()->void;
extern "C" [[noreturn]] auto tsk2()->void;
extern "C" [[noreturn]] auto tsk3()->void;
extern "C" [[noreturn]] auto tsk4()->void;

namespace osca{

// called by the interrupt handler for events other than keyboard or timer
extern "C" auto osca_on_exception()->void{
	static uint32 stack_0;
	static uint32 stack_1;
	static uint32 stack_2;
	static uint32 stack_3;
	static uint32 stack_4;
	static uint32 stack_5;
	static uint32 stack_6;
	static uint32 stack_7;

	osca_interrupts_disable();

	asm("mov   (%%esp),%0":"=r"(stack_0));
	asm("mov  4(%%esp),%0":"=r"(stack_1));
	asm("mov  8(%%esp),%0":"=r"(stack_2));
	asm("mov 12(%%esp),%0":"=r"(stack_3));
	asm("mov 16(%%esp),%0":"=r"(stack_4));
	asm("mov 20(%%esp),%0":"=r"(stack_5));
	asm("mov 24(%%esp),%0":"=r"(stack_6));
	asm("mov 28(%%esp),%0":"=r"(stack_7));

	err.p("osca exception").nl();
	err.spc().p_hex_32b(stack_0);
	err.spc().p_hex_32b(stack_1);
	err.spc().p_hex_32b(stack_2);
	err.spc().p_hex_32b(stack_3).nl();
	err.spc().p_hex_32b(stack_4);
	err.spc().p_hex_32b(stack_5);
	err.spc().p_hex_32b(stack_6);
	err.spc().p_hex_32b(stack_7);

	osca_hang();
}

// note: The FSAVE instruction saves a 108-byte data structure to memory (fpu_state), with the
//       first byte of the field needed to be aligned to a 16-byte boundary.
alignas(16) struct Task osca_tasks[]{
	//                                     :-> 0b01 grabs keyboard focus, 0b10 running
	//    eip     esp              eflags bits   id   edi  esi  ebp  esp0 ebx  edx  ecx  eax
	{uint32(tsk4),0xa'0000+320*176,0     ,0b11  ,1   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   },
	{uint32(tsk1),0xa'0000+320*180,0     ,0b10  ,2   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   },
	{uint32(tsk0),0xa'0000+320*184,0     ,0b11  ,3   ,0xde,0xec,0xeb,0xe5,0xb ,0xd ,0xc ,int32("kernel osca")},
	{uint32(tsk2),0xa'0000+320*188,0     ,0b10  ,4   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   },
	{uint32(tsk3),0xa'0000+320*192,0     ,0b10  ,5   ,0   ,0   ,0   ,0   ,0   ,0   ,1   ,140 },
	{uint32(tsk3),0xa'0000+320*196,0     ,0b10  ,6   ,0   ,0   ,0   ,0   ,0   ,0   ,2   ,160 },
	{uint32(tsk3),0xa'0000+320*200,0     ,0b10  ,7   ,0   ,0   ,0   ,0   ,0   ,0   ,4   ,180 },
};
const Task* const osca_tasks_end=osca_tasks+sizeof(osca_tasks)/sizeof(Task);

constexpr uint32 heap_entries_size=256;

class Heap final{
	struct Entry final{
		void*ptr{};
		uint32 size_bytes{};
	};

	inline static Data data_{}; // location and size of heap
	inline static char*mem_pos_{}; // position in heap to contiguous memory
	inline static char*mem_end_{}; // end of heap memory (1 past last)
	inline static Entry*ls_used_{}; // list of used memory entries
	inline static Entry*ls_used_pos_{}; // next available slot
	inline static Entry*ls_used_end_{}; // end (1 past last) of used entries list
	inline static Entry*ls_free_{}; // list of freed memory entries
	inline static Entry*ls_free_pos_{}; // next available slot
	inline static Entry*ls_free_end_{}; // end (1 past last) of free entries list
	inline static Size entries_size_{}; // maximum slots
public:
	static auto init_statics(const Data&data,const Size entries_size)->void{
		data_=data;
		entries_size_=entries_size;

		// place start of free memory
		mem_pos_=reinterpret_cast<char*>(data.address());

		// place used entries area at top of the heap
		ls_used_=static_cast<Entry*>(data.end())-entries_size;
		if(reinterpret_cast<char*>(ls_used_)<mem_pos_){
			err.p("Heap:init_statics:1");
			osca_hang();
		}
		ls_used_pos_=ls_used_;
		ls_used_end_=ls_used_+entries_size;

		// place free entries area before used entries
		ls_free_=ls_used_-entries_size;
		if(reinterpret_cast<char*>(ls_free_)<mem_pos_){
			err.p("Heap:init_statics:2");
			osca_hang();
		}
		ls_free_pos_=ls_free_;
		ls_free_end_=ls_free_+entries_size;

		// place end of free heap memory to start of free entries area
		mem_end_=reinterpret_cast<char*>(ls_free_);
	}
	static inline auto data()->const Data&{return data_;	}
	// called by operator 'new'
	static auto alloc(const uint32 size_bytes)->void*{
		// try to find a free slot of that size
		for(Entry*ent=ls_free_;ent<ls_free_pos_;ent++){
			if(ent->size_bytes!=size_bytes){
				continue;
			}

			// found a matching size entry
			void*ptr=ent->ptr;
			// move to used entries
			if(ls_used_pos_>=ls_used_end_){
				err.p("Heap:alloc:1");
				osca_hang();
			}
			*ls_used_pos_=*ent;
			ls_used_pos_++;
			ls_free_pos_--;
			*ent=*ls_free_pos_;
			pz_memset(ls_free_pos_,0x0f,sizeof(Entry)); // debugging (can be removed)
			return ptr;
		}
		// did not find in free list, create new
		char*ptr=mem_pos_;
		mem_pos_+=size_bytes;
		if(mem_pos_>mem_end_){
			err.p("Heap:alloc:2");
			osca_hang();
		}
		if(ls_used_pos_>=ls_used_end_){
			err.p("Heap:alloc:3");
			osca_hang();
		}
		// write to used list
		*ls_used_pos_={ptr,size_bytes};
		ls_used_pos_++;
		return ptr;
	}
	// called by operator 'delete'
	static auto free(void*ptr)->void{
		// find the allocated memory in the used list
		
		for(Entry*ent=ls_used_;ent<ls_used_pos_;ent++){
			if(ent->ptr!=ptr){
				continue;
			}

			// found the allocation entry
			// copy entry from used to free
			if(ls_free_pos_>=ls_free_end_){
				err.p("Heap:free:1");
				osca_hang();
			}
			*ls_free_pos_=*ent;
			ls_free_pos_++;

			// copy last entry from used list to this entry
			ls_used_pos_--;
			const uint32 size=ent->size_bytes;
			*ent=*ls_used_pos_;

			// debugging (can be removed)
			pz_memset(ptr,0x0f,SizeBytes(size));
			pz_memset(ls_used_pos_,0x0f,sizeof(Entry));
			return;
		}
		// did not find the allocated memory. probably a double delete
		err.p("Heap:free:2");
		osca_hang();
	}
	static auto clear(const uint8 b=0)->void{data_.clear(b);}
	static auto clear_heap_entries(const uint8 free_area=0,const uint8 used_area=0)->void{
		const SizeBytes es=SizeBytes(sizeof(Entry));
		pz_memset(ls_free_,free_area,entries_size_*es);
		pz_memset(ls_used_,used_area,entries_size_*es);
	}
};

class Keyboard{
	uint8 buf_[2<<3]{}; // ring buffer, minimum size 2, size power of 2, max 256
	uint8 out_{}; // next event index
	uint8 in_{}; // last event index +1 & roll
public:
	// called by 'osca_on_key'
	constexpr auto on_key(const uint8 scan_code)->void{
		const uint8 next_in=(in_+1)&(sizeof(buf_)-1);
		if(next_in==out_){ // check overrun
			return; // write would overwrite unhandled scan_code. display on status line?
		}
		buf_[in_]=scan_code;
		in_=next_in;
	}
	// returns keyboard scan code or 0 if no more events
	constexpr auto get_next_key()->uint8{
		if(out_==in_){
			return 0; // no more events
		}
		const uint8 scan_code=buf_[out_];
		out_++;
		out_&=sizeof(buf_)-1; // roll
		return scan_code;
	}
};

extern Keyboard keyboard;
Keyboard keyboard; // global initialized by 'osca_init'

// focused task that should read keyboard
inline Task*osca_task_focused{};

// declared in linker script 'link.ld' after code and data at first 64KB boundary
// address of symbol marks start of contiguous memory
extern "C" int free_mem_start_symbol;

// called by osca before starting tasks
// initiates globals
extern "C" auto osca_init()->void{
	using namespace osca;

	// green dot on screen (top left)
	*reinterpret_cast<char*>(0xa'0000)=2;

	// initiate globals

	vga13h=Vga13h{};
	
	err=PrinterToVga{};
	err.pos({1,1}).fg(4);
	
	out=PrinterToVga{};
	out.pos({1,2}).fg(2);
	
	keyboard=Keyboard{};
	
	osca_task_focused=&osca_tasks[0];

	//
	// setup heap
	//

	// start of contiguous free memory
	const Address free_mem_start=Address(&free_mem_start_symbol);
	
	// usable memory before EBDA. see https://wiki.osdev.org/Memory_Map_(x86)
	// const unsigned short usable_kb_before_ebda = *reinterpret_cast<unsigned short*>(0x413);
	// size of free memory (to beginning of EBDA)
	// const Address start_of_ebda=Address(usable_kb_before_ebda<<10);
	// debugging
	// err.p_hex_32b(unsigned(start_of_ebda));
	// size of free memory
	// const SizeBytes free_mem_size=SizeBytes(start_of_ebda)-SizeBytes(free_mem_start);
	
	// safe version does not use unused EBDA memory 
	const SizeBytes free_mem_size=SizeBytes(0x8'0000)-SizeBytes(free_mem_start);
	// err.p_hex_32b(unsigned(free_mem_size));

	// clear free memory
	pz_memset(free_mem_start,0,free_mem_size);

	// initiate heap with a size of 320*100 B
	Heap::init_statics({free_mem_start,320*100},heap_entries_size);
	// fill buffers with colors for debugging output
	Heap::clear(0x2c);
	Heap::clear_heap_entries(0x2e,0x2f);
}

// called by osca from the keyboard interrupt
// there is no task switch during this function
extern "C" auto osca_on_key(const uint8 scan_code)->void{
	static bool keyboard_ctrl_pressed{};

	using namespace osca;

	// on screen
	*reinterpret_cast<uint32*>(0xa0000+4)=scan_code;

	if(scan_code==0x1d)keyboard_ctrl_pressed=true;
	else if(scan_code==0x9d)keyboard_ctrl_pressed=false;
	
	// ? implement better task focus switch (same behaviour as alt+tab)
	if(keyboard_ctrl_pressed){ // ctrl+tab
		if(scan_code==0xf){ // tab pressed
			const Task*prev_task_focused=osca_task_focused;
			while(true){
				osca_task_focused++;
				if(osca_task_focused==osca_tasks_end){
					osca_task_focused=osca_tasks;
				}
				if(osca_task_focused==prev_task_focused){
					return; // found no new focusable task
				}
				if(osca_task_focused->is_running() && osca_task_focused->is_grab_keyboard_focus()){
					// task is running and requests keyboard focus
					return;
				}
			}
		}

		// if F1 through F12 pressed then toggle running state of task
		if(scan_code>=0x3b && scan_code<0x3b+12){
			const uint8 tsk_ix=scan_code-0x3b;
			if(sizeof(osca_tasks)/sizeof(Task)>tsk_ix){
				osca_tasks[tsk_ix].set_running(!osca_tasks[tsk_ix].is_running());
			}
			return;
		}
	}

	// to keyboard buffer
	keyboard.on_key(scan_code);
}
} // end namespace osca

// called by C++ to allocate and free memory
void*operator new[](unsigned size){return osca::Heap::alloc(size);}
void*operator new(unsigned size){return osca::Heap::alloc(size);}
void operator delete(void*ptr)noexcept{osca::Heap::free(ptr);}
void operator delete(void*ptr,unsigned size)noexcept;
void operator delete(void*ptr,unsigned size)noexcept{osca::Heap::free(ptr);}
void operator delete[](void*ptr)noexcept{osca::Heap::free(ptr);}
void operator delete[](void*ptr,unsigned size)noexcept;
void operator delete[](void*ptr,unsigned size)noexcept{osca::Heap::free(ptr);}
