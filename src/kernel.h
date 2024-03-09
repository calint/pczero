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
extern "C" auto osca_exception()->void{
	static unsigned stack_0;
	static unsigned stack_1;
	static unsigned stack_2;
	static unsigned stack_3;
	static unsigned stack_4;
	static unsigned stack_5;
	asm("mov (%%esp),%0":"=r"(stack_0));
	asm("mov 4(%%esp),%0":"=r"(stack_1));
	asm("mov 8(%%esp),%0":"=r"(stack_2));
	asm("mov 12(%%esp),%0":"=r"(stack_3));
	asm("mov 16(%%esp),%0":"=r"(stack_4));
	asm("mov 20(%%esp),%0":"=r"(stack_5));

	err.p("osca exception").nl();
	err.spc().p_hex_32b(stack_0);
	err.spc().p_hex_32b(stack_1);
	err.spc().p_hex_32b(stack_2);
	err.spc().p_hex_32b(stack_3);
	err.spc().p_hex_32b(stack_4);
	err.spc().p_hex_32b(stack_5);
	osca_hang();
}

// note: The FSAVE instruction saves a 108-byte data structure to memory (fpu_state), with the
//       first byte of the field needed to be aligned on a 16-byte boundary.
alignas(16) struct Task osca_tasks[]{
	//                                       :-> 0b01 grabs keyboard focus, 0b10 active
	//        eip   esp              eflags bits   id   edi  esi  ebp  esp0 ebx  edx  ecx  eax
	{Register(tsk4),0xa'0000+320*176,0     ,0b11  ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   },
	{Register(tsk1),0xa'0000+320*180,0     ,0b10  ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   },
	{Register(tsk0),0xa'0000+320*184,0     ,0b11  ,1   ,0xde,0xec,0xeb,0xe5,0xb ,0xd ,0xc ,Register("kernel osca")},
	{Register(tsk2),0xa'0000+320*188,0     ,0b10  ,2   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,0   },
	{Register(tsk3),0xa'0000+320*192,0     ,0b10  ,3   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,140 },
	{Register(tsk3),0xa'0000+320*196,0     ,0b10  ,4   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,160 },
	{Register(tsk3),0xa'0000+320*200,0     ,0b10  ,5   ,0   ,0   ,0   ,0   ,0   ,0   ,0   ,180 },
};
Task*osca_tasks_end=osca_tasks+sizeof(osca_tasks)/sizeof(Task);

class Heap final{
	struct Entry final{
		void*ptr{};
		unsigned size_bytes{};
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
	inline static Size nentries_max_{}; // maximum slots
public:
	static auto init_statics(const Data&d,const Size nentries_max)->void{
		data_=d;
		nentries_max_=nentries_max;

		// place start of free memory
		mem_pos_=reinterpret_cast<char*>(d.address());

		// place used entries area at top of the heap
		ls_used_=static_cast<Entry*>(d.end())-nentries_max;
		if(reinterpret_cast<char*>(ls_used_)<mem_pos_){
			err.p("Heap:init_statics:1");
			osca_hang();
		}
		ls_used_pos_=ls_used_;
		ls_used_end_=ls_used_+nentries_max;

		// place free entries area before used entries
		ls_free_=ls_used_-nentries_max;
		if(reinterpret_cast<char*>(ls_free_)<mem_pos_){
			err.p("Heap:init_statics:2");
			osca_hang();
		}
		ls_free_pos_=ls_free_;
		ls_free_end_=ls_free_+nentries_max;

		// place end of free heap memory to start of free entries area
		mem_end_=reinterpret_cast<char*>(ls_free_);
	}
	static inline auto data()->const Data&{return data_;	}
	// called by operator 'new'
	static auto alloc(const unsigned size_bytes)->void*{
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
			const unsigned size=ent->size_bytes;
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
	static auto clear(const Byte b=0)->void{data_.clear(b);}
	static auto clear_heap_entries(const Byte free_area=0,const Byte used_area=0)->void{
		const SizeBytes es=SizeBytes(sizeof(Entry));
		pz_memset(ls_free_,free_area,nentries_max_*es);
		pz_memset(ls_used_,used_area,nentries_max_*es);
	}
};

class Keyboard{
	Byte buf[2<<3]{}; // minimum size 2 and a power of 2, max size 256
	Byte s{}; // next event index
	Byte e{}; // last event index +1 & roll
public:
	// called by 'osca_keyb_ev'
	constexpr auto on_key(const Byte ch)->void{
		const Byte ne=(e+1)&(sizeof(buf)-1); // next end index
		if(ne==s){ // check overrun
			return; // write would overwrite unhandled key. display on status line?
		}
		buf[e]=ch;
		e=ne;
	}
	// returns keyboard scan code or 0 if no more events.
	constexpr auto get_next_scan_code()->Byte{
		if(s==e){
			return 0; // no more events
		}
		const Byte ch=buf[s];
		s++;
		s&=sizeof(buf)-1; // roll
		return ch;
	}
};

extern Keyboard keyboard;
Keyboard keyboard; // global initialized by 'osca_init'

inline static bool keyboard_ctrl_pressed{};
inline Task*task_focused{};
inline TaskId task_focused_id{};

// declared in linker script 'link.ld' after code and data at first 64KB boundary
// address of symbol marks start of contiguous memory
extern "C" int free_mem_start_symbol;

// called by osca before starting tasks
// initiates globals
extern "C" auto osca_init()->void{
	using namespace osca;
	// green dot on screen (top left)
	*reinterpret_cast<char*>(0xa'0000)=2;
	
	// start of contiguous free memory
	Address free_mem_start=Address(&free_mem_start_symbol);
	// size of free memory (to the vga address space that starts at 0xa'0000)
	SizeBytes free_mem_size=0xa'0000-reinterpret_cast<SizeBytes>(free_mem_start);
	// clear free memory
	pz_memset(free_mem_start,0,free_mem_size);

	// initiate statics
	vga13h=Vga13h{};
	
	err=PrinterToVga{};
	err.pos({1,1}).fg(4);
	
	out=PrinterToVga{};
	out.pos({1,2}).fg(2);
	
	keyboard=Keyboard{};
	
	task_focused=&osca_tasks[0];
	task_focused_id=task_focused->get_id();
	
	// initiate heap with a size of 320*100 B
	Heap::init_statics({free_mem_start,320*100},nobjects_max);
	Heap::clear(0x2c);
	Heap::clear_heap_entries(0x2e,0x2f);
}
// called by osca from the keyboard interrupt
// there is no task switch during this function
extern "C" auto osca_keyb_ev()->void{
	using namespace osca;
	// on screen
	*reinterpret_cast<Byte*>(0xa0000+4)=osca_key;

	if(osca_key==0x1d)keyboard_ctrl_pressed=true;
	else if(osca_key==0x9d)keyboard_ctrl_pressed=false;
	
	// ? implement better task focus switch (same behaviour as alt+tab)
	if(keyboard_ctrl_pressed){ // ctrl+tab
		if(osca_key==0xf){ // tab pressed
			const Task*prev_task_focused=task_focused;
			while(true){
				task_focused++;
				if(task_focused==osca_tasks_end){
					task_focused=osca_tasks;
				}
				if(task_focused==prev_task_focused){
					return; // no new focusable task
				}
				if(task_focused->is_running() && task_focused->is_grab_keyboard_focus()){
					task_focused_id=task_focused->get_id();
					return;
				}
			}
		}

		// if F1 through F12 pressed toggle running state of task
		if(osca_key>=0x3b && osca_key<=0x3b+12){
			const Byte tsk=osca_key-0x3b;
			if(sizeof(osca_tasks)/sizeof(Task)>tsk){
				osca_tasks[tsk].set_running(!osca_tasks[tsk].is_running());
			}
			return;
		}
	}

	// to keyboard handler
	keyboard.on_key(osca_key);
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
