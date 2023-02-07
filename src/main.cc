#include "osca.h"
#include "lib.h"
#include "lib2d.h"
#include "libge.h"

using namespace osca;

class Heap{
	static Data d_;
	static char*ptr_;
	static char*ptr_lim_;
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
			err.pos({1,1}).p("heap overrun");// ? hlt
		}
		return reinterpret_cast<void*>(p);
	}
	static auto clear_buffer(unsigned char b)->void{
		pz_memset(d_.address(),b,d_.size());
	}
};
Data Heap::d_={nullptr,0};
char*Heap::ptr_;
char*Heap::ptr_lim_;


void operator delete(void*ptr,unsigned size)noexcept;
void operator delete[](void*ptr,unsigned size)noexcept;

// called by C++ to allocate and free memory
//void*operator new[](unsigned count){return heap_main.alloc(count);}
//void*operator new(unsigned count){return heap_main.alloc(count);}
void*operator new[](unsigned count){return Heap::alloc(count);}
void*operator new(unsigned count){return Heap::alloc(count);}

void operator delete(void*ptr)noexcept{
//	out.printer().p("D:").p_hex_32b(reinterpret_cast<unsigned int>(ptr)).p(' ');
}
void operator delete(void*ptr,unsigned size)noexcept{
//	out.printer().p("DS:").p_hex_32b(size).p(' ').p_hex_32b(reinterpret_cast<unsigned int>(ptr)).p(' ');
}
void operator delete[](void*ptr)noexcept{
//	out.printer().p("DA:").p_hex_32b(reinterpret_cast<unsigned int>(ptr)).p(' ');
}
void operator delete[](void*ptr,unsigned size)noexcept{
//	out.printer().p("DSA:").p_hex_32b(size).p(' ').p_hex_32b(reinterpret_cast<unsigned int>(ptr)).p(' ');
}

// called by osca from the keyboard interrupt
extern "C" void osca_init(){
	// green dot on screen (top left)
	*reinterpret_cast<int*>(0xa0000)=0x02;

	// clear 128 KB starting at 1 MB with 0x20
	pz_memset(Address(0x10'0000),0x11,0x20'000);

	// initiate statics
	vga13h=Vga13h();
	err=PrinterToVga();
	err.pos({1,1}).fg(4);
	out=PrinterToVga();
	out.pos({1,2}).fg(2);
	Heap::init_statics({Address(0x10'0000),320*50});
	Heap::clear_buffer(0x12);
	Object::init_statics();
	PhysicsState::init_statics(Address(0x11'0000),Object::all_len);
	PhysicsState::clear_buffer(1);
}


extern "C" [[noreturn]] void tsk0();
extern "C" [[noreturn]] void tsk2();
extern "C" [[noreturn]] void tsk3();
extern "C" [[noreturn]] void tsk4();

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

// called by osca from the keyboard interrupt
extern "C" void osca_keyb_ev(){
	*reinterpret_cast<int*>(0xa0000+4)=osca_key;
	osca_keyb.on_key(osca_key);
	static unsigned char*p=reinterpret_cast<unsigned char*>(0xa0000+320*49+100);
	*p++=osca_key;
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" [[noreturn]] void tsk0(){
	PrinterToBitmap pb{vga13h.bmp()};

	pb.pos({30,1});
	for(int i=0;i<16;i++){
		pb.p_hex(i);
	}
	pb.fg(5).pos({13,2});
	for(char i='0';i<='9';i++){
		pb.p(i);
	}
	pb.fg(6);
	for(char i='a';i<='z';i++){
		pb.p(i);
	}
	pb.fg(7).pos({13+10,3});
	for(char i='A';i<='Z';i++){
		pb.p(i);
	}

	pb.fg(2).pos({15,4}).p("hello world!").nl();
	pb.fg(6).p("\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
	pb.fg(7).p(' ').p_hex_32b(sizeof(table_ascii_to_font)/sizeof(int));

	pb.pos({5,7}).fg(2).p('_');

	while(true){
		// handle keyboard events
		while(true){
			const unsigned char sc=osca_keyb.get_next_scan_code();
			if(!sc)
				break;
			if(sc==0xe){ // backspace
				pb.backspace().backspace().p('_');
				continue;
			}
			if(sc==0x1c){ // return
				pb.backspace().nl().p('_');
				continue;
			}
			char ch=table_scancode_to_ascii[sc];
			if(!ch) // not an ascii. probably key release
				continue;
			if(ch>='a'&&ch<='z')
				ch&=~0x20;
			pb.backspace().p(ch).p('_');
		}
		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
asm(".global tsk1");
asm(".align 16");
asm("tsk1:");
asm("  incl 0xa0044");
asm("  hlt");
asm("  jmp tsk1");
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" [[noreturn]] void tsk2(){
	while(true){
//		osca_yield();
		// copy kernel to screen
		Data src=Data(Address(0x07c00),512*3);// kernel binary
//		Data dst1=Data(Address(0x11'0000),512*3);// to odd meg testing a20 enabled line
		Data dst2=Data(Address(0xa'0000+320*150),512*3);// on screen line 150
//		Data dst2=Data(Address(0xabb80),512*3);// on screen line 150
//		src.to(dst1);
		src.to(dst2);
		osca_yield();
//		dst1.to(dst2);
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" [[noreturn]] void tsk3(){
	while(true){
		vga13h.bmp().data().pointer().offset(8).write(osca_t);
		osca_yield(); // without this the next line is optimized away. why=
//		*(int*)(0xa0000+320-4)=osca_t;
//		err.pos({1,1}).p_hex_32b(reinterpret_cast<unsigned>(osca_t));
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
static class ObjectDefRectangle:public ObjectDef{
public:
	constexpr ObjectDefRectangle(){
		npts_=5;
		pts_=new Point2D[npts_]{
			{ 0, 0},
			{-2,-1},
			{ 2,-1},
			{ 2, 1},
			{-2, 1},
		};
	}
}default_object_def_rectangle;

static class ObjectDefShip:public ObjectDef{
public:
	constexpr ObjectDefShip(){
		npts_=4;
		pts_=new Point2D[npts_]{
			{ 0, 0},
			{ 0,-1},
			{-1,.5},
			{ 1,.5},
		};
	}
}default_object_def_ship;


static unsigned char colr;
class Bullet:public Object{
public:
	Bullet():
		Object{default_object_def_ship,3,{0,0},0,++colr}
	{}
	constexpr virtual auto update()->bool override{
		Object::update();
		if(phy().pos().x>300){
			return false;
		}
		if(phy().pos().x<20){
			return false;
		}
		if(phy().pos().y>130){
			return false;
		}
		if(phy().pos().y<70){
			return false;
		}
		return true;
	}
};

class Ship:public Object{
public:
	Ship():
		Object{default_object_def_ship,4,{120,100},0,2}
	{}

	constexpr virtual auto update()->bool override{
		Object::update();
		if(phy().pos().x>300||phy().pos().x<20){
			phy().dpos_.x=-phy().dpos_.x;
		}
		if(phy().pos().y>130||phy().pos().y<70){
			phy().dpos_.y=-phy().dpos_.y;
		}
		return true;
	}

	auto fire(){
		Bullet*b=new Bullet;
		b->phy().set_pos(phy().pos());
		b->phy().set_dpos(forward_vector().scale(5));
		b->phy().set_angle(phy().angle());
	}
};

extern "C" [[noreturn]] void tsk4(){
	//----------------------------------------------------------
	// init statics
	//----------------------------------------------------------
	default_object_def_rectangle=ObjectDefRectangle();
	default_object_def_ship=ObjectDefShip();

	//----------------------------------------------------------

	// init stack
	Bitmap&db=vga13h.bmp();
	PrinterToBitmap pb{db};
	Degrees deg=0;
	Matrix2D R;

//	const Address heap_address=heap_main.data().address();
	const Address heap_address=Heap::data().address();
	const Address heap_disp_at_addr=db.data().pointer().offset(50*320).address();
	const SizeBytes heap_disp_size=320*50;

	const Address physt_address=PhysicsState::mem_start;
	const Address physt_disp_at_addr=db.data().pointer().offset(100*320).address();
	const SizeBytes physt_disp_size=320*50;

	Ship*shp=new Ship;
//	shp->phy().set_dangle(deg_to_rad(-5));
	shp->phy().set_dpos({1,1});


	Ship*shp2=new Ship;
	shp2->phy().set_dangle(deg_to_rad(7));
	shp2->phy().set_dpos({-1,0});

	Object*wall=new Object{default_object_def_rectangle,10,{100,100},0,4};
	wall->phy().set_dangle(deg_to_rad(5));

	// start task
	while(true){
		// copy heap
		pz_memcpy(heap_disp_at_addr,heap_address,heap_disp_size);
		pz_memcpy(physt_disp_at_addr,physt_address,physt_disp_size);

		pb.pos({1,2}).fg(2).p("t=").p_hex_32b(osca_t);

		PhysicsState::update_physics_states();
		Object::update_all();
		Object::render_all(db);

		const char ch=table_scancode_to_ascii[osca_key];
		if(ch){
			if(ch=='c'){
				if(Object::hasFreeSlot()){
					shp=new Ship;
					shp->phy().set_dangle(deg_to_rad(-5));
					shp->phy().set_dpos({1,1});
				}else{
					err.pos({1,1}).p("out of free slots");
				}
			}
			if(shp){
				switch(ch){
				case'w':
					shp->phy().set_dpos(shp->forward_vector().scale(1));
					break;
				case'a':
					shp->phy().agl_-=deg_to_rad(5);
					break;
				case's':
					shp->phy().set_dpos({0,1});
					break;
				case'd':
					shp->phy().agl_+=deg_to_rad(5);
					break;
				case'x':
					delete shp;
					shp=nullptr;
					break;
				case' ':
					shp->fire();
					break;
				default:
					break;
				}
			}
		}

		if(deg>360)
			deg-=360;
		deg+=5;
		const float rotation=deg_to_rad(deg);
//		shp3->set_angle(rotation);
		R.set_transform(5,rotation,{160,100});
		// dot axis
		dot(db,160,100,0xf);
		const Vector2D xaxis=R.axis_x().normalize().scale(7);
		dot(db,xaxis.x+160,xaxis.y+100,4);
		const Vector2D yaxis=R.axis_y().normalize().scale(7);
		dot(db,yaxis.x+160,yaxis.y+100,2);

		osca_yield();
	}
}
//
//static unsigned char bmp0[]{
//	0,1,1,0,
//	1,1,1,1,
//	1,1,1,1,
//	0,1,1,0,
//};
//static unsigned char bmp1[]{
//	0,2,2,0,
//	2,2,2,2,
//	2,2,2,2,
//	0,2,2,0,
//};
//static unsigned char bmp2[]{
//	0,3,3,0,
//	3,3,3,3,
//	3,3,3,3,
//	0,3,3,0,
//};
//static unsigned char bmp3[]{
//	0,4,4,0,
//	4,4,4,4,
//	4,4,4,4,
//	0,4,4,0,
//};
//extern "C" void tsk4(){
//	static Vga13h dsp;
//	static const Bitmap&dbmp=dsp.bmp();
//	static PrinterToBitmap pb{dbmp};
//	static PrinterToBitmap pb2{dbmp};
//	static Bitmap bitmaps[]{
//		{Address{bmp0},DimensionPx{4,4}},
//		{Address{bmp1},DimensionPx{4,4}},
//		{Address{bmp2},DimensionPx{4,4}},
//		{Address{bmp3},DimensionPx{4,4}},
//	};
//	static Sprite sprites[]{
//		{bitmaps[0],Position{20.0f,25.f},Velocity{5,0.5f},Acceleration{0,0.5f}},
//		{bitmaps[1],Position{30.0f,25.f},Velocity{5,1.0f},Acceleration{0,0.5f}},
//		{bitmaps[2],Position{40.0f,25.f},Velocity{5,1.5f},Acceleration{0,0.5f}},
//		{bitmaps[3],Position{50.0f,25.f},Velocity{5,2.0f},Acceleration{0,0.5f}},
//	};
//
//	CoordPx x=24;
//	CoordPx x_prv=x;
//	pb.pos(1,30);
//	for(int i=0;i<16;i++){
//		pb.p_hex(i);
//	}
//	pb.pos(2,10).fg(5);
//	for(char i='0';i<='9';i++){
//		pb.p(i);
//	}
//	for(char i='a';i<='z';i++){
//		pb.p(i);
//	}
//	pb.p(' ');
//	pb.fg(6).p("hello world!").nl();
//	pb.p("\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
//	pb.fg(7).p(' ').p_hex_32b(sizeof(table_ascii_to_font)/sizeof(int));
//
//	while(true){
//		bitmaps[0].to(dbmp,CoordsPx{x_prv,44});
//		bitmaps[1].to(dbmp,CoordsPx{x,44});
//		bitmaps[2].to(dbmp,CoordsPx{x,36});
//		bitmaps[3].to(dbmp,CoordsPx{x,28});
//		x_prv=x;
//		x+=8;
//		if(x>180){
//			x=24;
//		}
//		for(const auto&s:sprites){
//			s.to(dbmp);
//		}
//		for(auto&s:sprites){
//			s.update();
//		}
//		for(auto&s:sprites){
//			if(s.pos().y()>140){
//				Position p{s.pos().x(),140};
//				if(p.x()>static_cast<Coord>(dsp.bmp().dim().width())){
//					p.set_x(0);
//				}
//				s.set_pos(p);
//				Velocity v{s.velocity()};
//				v.set_y(-8);
//				s.set_velocity(v);
//			}
//		}
//		pb.fg(7).pos(5,10).transparent(true).p("transparent");
////		osca_yield();
//	}
//}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

