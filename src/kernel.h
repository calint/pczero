#pragma once
#include"osca.h"
#include"lib.h"

extern "C" [[noreturn]] void tsk0();
extern "C" [[noreturn]] void tsk2();
extern "C" [[noreturn]] void tsk3();
extern "C" [[noreturn]] void tsk4();

namespace osca{
struct Task osca_tasks[]{
	//        eip   esp        eflags bits  id   edi esi ebp esp0 ebx edx ecx eax
	{Register(tsk4),0x000af000,0     ,1    ,4   ,0  ,0  ,0  ,0   ,0  ,0  ,0  ,0  },
	{Register(tsk0),0x000afa00,0     ,1    ,0   ,0  ,0  ,0  ,0   ,0  ,0  ,0  ,0  },
	{Register(tsk3),0x000af280,0     ,0    ,3   ,0  ,0  ,0  ,0   ,0  ,0  ,0  ,0  },
};
Task*osca_tasks_end=osca_tasks+sizeof(osca_tasks)/sizeof(Task);

struct HeapEntry final{
	void*ptr{nullptr}; // pointer to memory
	unsigned size_bytes{0}; // size of allocated memory in bytes
};
using SizeCount=Size;
class Heap final{
	inline static Data d_{nullptr,0};
	inline static char*ptr_{nullptr}; // pointer to free memory
	inline static char*ptr_lim_{nullptr}; // limit of buffer
	inline static HeapEntry*entry_used_start_{nullptr}; // beginning of vector containing used memory info
	inline static HeapEntry*entry_used_next_{nullptr}; // next available slot
	inline static HeapEntry*entry_used_end_{nullptr}; // limit of used entries memory
	inline static HeapEntry*entry_free_start_{nullptr}; // beginning of vector containing freed memory info
	inline static HeapEntry*entry_free_next_{nullptr};
	inline static HeapEntry*entry_free_end_{nullptr};
	inline static SizeCount nentries_max_{0};
public:
	static auto init_statics(const Data&d,const SizeCount nentries_max)->void{
		d_=d;
		ptr_=reinterpret_cast<char*>(d.address());
		nentries_max_=nentries_max;

		entry_used_start_=static_cast<HeapEntry*>(d.end())-nentries_max;
		if(reinterpret_cast<char*>(entry_used_start_)<ptr_){
			err.p("Heap.init_statics:1");
			osca_halt();
		}
		entry_used_next_=entry_used_start_;
		entry_used_end_=entry_used_start_+nentries_max;

		entry_free_start_=entry_used_start_-nentries_max;
		entry_free_next_=entry_free_start_;
		entry_free_end_=entry_free_start_+nentries_max;
		ptr_lim_=reinterpret_cast<char*>(entry_used_start_);
	}
	static inline auto data()->const Data&{
		return d_;
	}
	// called by operator 'new'
	static auto alloc(const unsigned size_bytes)->void*{
//			err.p_hex_32b(size).spc();
		// try to find a free slot with that size
		HeapEntry*he=entry_free_start_;
		while(he<entry_free_end_){
			if(he->size_bytes==size_bytes){
				// found a matching size entry
				void*p=he->ptr;
				// move to used entries
				if(entry_used_next_>=entry_used_end_){
					err.p("Heap.alloc: 1");
					osca_halt();
				}
				*entry_used_next_=*he;
				entry_used_next_++;
				// copy last free to this slot
				entry_free_next_--;
				*he=*entry_free_next_;
				pz_memset(entry_free_next_,3,sizeof(HeapEntry));
				return p;
			}
			he++;
		}
		// did not find in free list
		char*p=ptr_;
		ptr_+=size_bytes;
		if(ptr_>ptr_lim_){
			err.p("Heap.alloc: 2");
			osca_halt();
		}
		if(entry_used_next_>=entry_used_end_){
			err.p("Heap.alloc: 3");
			osca_halt();
		}
		// write to used list
		*entry_used_next_={p,size_bytes};
		entry_used_next_++;
		return reinterpret_cast<void*>(p);
	}
	// called by operator 'delete'
	static auto free(void*ptr)->void{
//			err.p_hex_32b(reinterpret_cast<unsigned>(ptr)).spc();
		// find the allocated memory in the used list
		HeapEntry*hep=entry_used_start_;
		while(hep<entry_used_next_){
			if(hep->ptr==ptr){
				// found the allocation entry
				// copy entry from used to free
				if(entry_free_next_>=entry_free_end_){
					err.p("Heap.free: 1");
					osca_halt();
				}
				*entry_free_next_=*hep;
				entry_free_next_++;

				// copy last entry from used list to this entry
				entry_used_next_--;
				const unsigned size=hep->size_bytes;
				*hep=*entry_used_next_;

				// debugging
				pz_memset(ptr,0xf,SizeBytes(size));
				pz_memset(entry_used_next_,5,sizeof(HeapEntry));
				return;
			}
			hep++;
		}
		// did not find the allocated memory. probably a double delete
		err.p("Heap.free: 2");
		osca_halt();
	}
	static auto clear_buffer(const char b=0)->void{d_.clear(b);}
	static auto clear_heap_entries(char free_area=0,char used_area=0)->void{
		const SizeBytes hes=SizeBytes(sizeof(HeapEntry));
		pz_memset(entry_free_start_,free_area,nentries_max_*hes);
		pz_memset(entry_used_start_,used_area,nentries_max_*hes);
	}
};

class Keyboard{
	unsigned char buf[2<<4]{}; // minimum size 2 and a power of 2, max size 256
	unsigned char s{0}; // next event index
	unsigned char e{0}; // last event index +1 & roll
public:
	// called by osca_keyb_ev
	constexpr auto on_key(unsigned char ch)->void{
		const unsigned char ne=(e+1)&(sizeof(buf)-1);// next "end" index
		if(ne==s)// check overrun
			return;// write would overwrite. display on status line?
		buf[e]=ch;
		e=ne;
	}
	// returns keyboard scan code or 0 if no more events.
	constexpr auto get_next_scan_code()->unsigned char{
		if(s==e)
			return 0; // no more events
		const unsigned char ch=buf[s];
		s++;
		s&=sizeof(buf)-1;// roll
		return ch;
	}
};
extern Keyboard keyboard;
Keyboard keyboard; // global initialized by osca_init
inline static bool keyboard_ctrl_pressed{false};
inline static Task*task_focused{nullptr};
inline static TaskId task_focused_id{0};

// called by osca before starting tasks
// initiates globals
extern "C" void osca_init(){
	using namespace osca;
	// green dot on screen (top left)
	*reinterpret_cast<int*>(0xa0000)=0x02;

	// set 64 KB of 0x11 starting at 1 MB
	pz_memset(Address(0x10'0000),0x11,0x1'0000);

	// initiate statics
	vga13h=Vga13h{};
	err=PrinterToVga{};
	err.pos({1,1}).fg(4);
	out=PrinterToVga{};
	out.pos({1,2}).fg(2);
	keyboard=Keyboard{};
	task_focused=&osca_tasks[0];
	task_focused_id=task_focused->get_id();
	Heap::init_statics({Address(0x10'0000),320*100},World::nobjects_max);
	Heap::clear_buffer(0x12);
	Heap::clear_heap_entries(3,5);
	Object::init_statics();
	PhysicsState::init_statics();
	PhysicsState::clear_buffer(1);
}
// called by osca from the keyboard interrupt
// there is no task switch during this function
extern "C" void osca_keyb_ev(){
	using namespace osca;
	// on screen
	*reinterpret_cast<int*>(0xa0000+4)=osca_key;

	if(osca_key==0x1d)keyboard_ctrl_pressed=true;
	else if(osca_key==0x9d)keyboard_ctrl_pressed=false;
	// ? implement better task focus switch (same behaviour as alt+tab)
	if(keyboard_ctrl_pressed&&osca_key==0xf){ // ctrl+tab
		const Task*prev_task_focused=task_focused;
		while(true){
			task_focused++;
			if(task_focused==osca_tasks_end){
				task_focused=osca_tasks;
			}
			if(task_focused==prev_task_focused)
				return;
			if(task_focused->is_grab_keyboard_focus()){
				task_focused_id=task_focused->get_id();
				return;
			}
		}
	}

	// to keyboard handler
	keyboard.on_key(osca_key);

//	static unsigned char*p=reinterpret_cast<unsigned char*>(0xa0000+320*49+100);
//	*p++=osca_key;
}
} // end namespace osca

// called by C++ to allocate and free memory
void operator delete(void*ptr,unsigned size)noexcept;
void operator delete[](void*ptr,unsigned size)noexcept;

void*operator new[](unsigned count){return osca::Heap::alloc(count);}
void*operator new(unsigned count){return osca::Heap::alloc(count);}
void operator delete(void*ptr)noexcept{osca::Heap::free(ptr);}
void operator delete(void*ptr,unsigned size)noexcept{osca::Heap::free(ptr);}
void operator delete[](void*ptr)noexcept{osca::Heap::free(ptr);}
void operator delete[](void*ptr,unsigned size)noexcept{osca::Heap::free(ptr);}
