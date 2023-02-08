#include"osca.h"
#include"lib.h"
#include"lib2d.h"
#include"libge.h"

extern "C" [[noreturn]] void tsk0();
extern "C" [[noreturn]] void tsk2();
extern "C" [[noreturn]] void tsk3();
extern "C" [[noreturn]] void tsk4();

namespace osca{
	class Heap{
		static Data d_;
		static char*ptr_; // pointer to free memory
		static char*ptr_lim_; // limit of buffer
	public:
		static auto init_statics(const Data&d){
			d_=d;
			ptr_=reinterpret_cast<char*>(d.address());
			ptr_lim_=ptr_+d.size();
		}
		static inline const Data&data(){
			return d_;
		}
		static inline auto alloc(const unsigned size)->void*{
			char*p=ptr_;
			ptr_+=size;
			if(ptr_>ptr_lim_){
				err.p("heap overrun");
				osca_halt();
			}
			return reinterpret_cast<void*>(p);
		}
		static inline auto clear_buffer(const unsigned char b=0)->void{d_.clear(b);}
	};
	Data Heap::d_ {nullptr,0};
	char*Heap::ptr_;
	char*Heap::ptr_lim_;
}
// called by C++ to allocate and free memory
void operator delete(void*ptr,unsigned size)noexcept;
void operator delete[](void*ptr,unsigned size)noexcept;

void*operator new[](unsigned count){return osca::Heap::alloc(count);}
void*operator new(unsigned count){return osca::Heap::alloc(count);}
void operator delete(void*ptr)noexcept{
//	out.p("d:").p_hex_32b(reinterpret_cast<unsigned int>(ptr)).p(' ');
}
void operator delete(void*ptr,unsigned size)noexcept{
//	out.p("ds:").p_hex_16b(static_cast<unsigned short>(size)).p(' ').p_hex_32b(reinterpret_cast<unsigned int>(ptr)).p(' ');
}
void operator delete[](void*ptr)noexcept{
//	out.p("da:").p_hex_32b(reinterpret_cast<unsigned int>(ptr)).p(' ');
}
void operator delete[](void*ptr,unsigned size)noexcept{
//	out.p("das:").p_hex_16b(static_cast<unsigned short>(size)).p(' ').p_hex_32b(reinterpret_cast<unsigned int>(ptr)).p(' ');
}

namespace osca{
	class{
		unsigned char buf[2<<4]; // minimum size 2 and a power of 2, max size 256
		unsigned char s; // next event index
		unsigned char e; // last event index +1 & roll
	public:
		// called by osca_keyb_ev
		constexpr auto on_key(unsigned char ch){
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
	}osca_keyb;
} // end namespace

// called by osca from the keyboard interrupt
extern "C" void osca_keyb_ev(){
	*reinterpret_cast<int*>(0xa0000+4)=osca_key;
	osca::osca_keyb.on_key(osca_key);
	static unsigned char*p=reinterpret_cast<unsigned char*>(0xa0000+320*49+100);
	*p++=osca_key;
}
// called by osca from the keyboard interrupt
extern "C" void osca_init(){
	using namespace osca;
	// green dot on screen (top left)
	*reinterpret_cast<int*>(0xa0000)=0x02;

	// set 64 KB of 0x11 starting at 1 MB
	pz_memset(Address(0x10'0000),0x11,0x1'0000);

	// initiate statics
	vga13h=Vga13h();
	err=PrinterToVga();
	err.pos({1,1}).fg(4);
	out=PrinterToVga();
	out.pos({1,2}).fg(2);
	Heap::init_statics({Address(0x10'0000),320*50});
	Heap::clear_buffer(0x12);
	Object::init_statics();
	PhysicsState::init_statics();
	PhysicsState::clear_buffer(1);
}
