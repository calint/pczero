#include "osca.h"
#include "lib.h"
#include "lib2d.h"
#include "libge.h"

using namespace osca;

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
				ch&=~0x20; // to upper case
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
		// copy kernel to screen
		constexpr int kernel_size=512*3;
		Data src=Data(Address(0x7c00),kernel_size); // kernel binary
		Data dst=Data(Address(0xa'0000+320*150),kernel_size); // on screen
		src.to(dst);
		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" [[noreturn]] void tsk3(){
	while(true){
		vga13h.bmp().data().pointer().offset(8).write(osca_t);
		osca_yield(); // ? without this dot on screen is optimized away
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
static ObjectDef rectangle_def;
static ObjectDef ship_def;
static ObjectDef bullet_def;

class Wall:public Object{
public:
	// type bits 0b100 check collision with type 'bullet' 0b10
	Wall(const Scale scl,const Point2D&pos,const Angle agl):
		Object{0b100,0b10,rectangle_def,scl,pos,agl,3}
	{}
	// returns false if object is to be deleted
	constexpr virtual auto on_collision(Object&other)->bool{
//		out.p("col ").p_hex_8b(static_cast<unsigned char>(other.type_bits()));
		if(other.type_bits()==2) // collision with type bullet
			return false; // delete
		return true;
	}
};

static unsigned char colr;
class Bullet:public Object{
public:
	Bullet():
		// type bits 0b10 check collision with type 'wall' 0b100
		Object{0b10,0b100,bullet_def,.5f,{0,0},0,++colr}
	{}
	constexpr virtual auto update()->bool override{
		Object::update();
		if(phy().pos.x>300){
			return false;
		}
		if(phy().pos.x<20){
			return false;
		}
		if(phy().pos.y>130){
			return false;
		}
		if(phy().pos.y<70){
			return false;
		}
		return true;
	}
	// returns false if object is to be deleted
	constexpr virtual auto on_collision(Object&other)->bool{
		return false;
	}
};

class Ship:public Object{
public:
	Ship():
		// type bits 0b1 check collision with nothing 'wall' 0b100
		Object{0b1,0b100,ship_def,4,{0,0},0,2}
	{}

	constexpr virtual auto update()->bool override{
		Object::update();
		if(phy().pos.x>300||phy().pos.x<20){
			phy().dpos.x=-phy().dpos.x;
		}
		if(phy().pos.y>130||phy().pos.y<70){
			phy().dpos.y=-phy().dpos.y;
		}
		return true;
	}

	// returns false if object is to be deleted
	constexpr virtual auto on_collision(Object&other)->bool{
		if(other.type_bits()==4) // collision with type wall
			return false; // delete
		return true;
	}

	auto fire(){
		Bullet*b=new Bullet;
		b->phy().pos=phy().pos;
		b->phy().dpos=forward_vector().scale(5);
		b->phy().agl=phy().agl;
	}
};

static auto draw_axis(Bitmap&dsp){
	static Degrees deg=0;
	static Matrix2D R;
	if(deg>360)
		deg-=360;
	deg+=5;
	const float rotation=deg_to_rad(deg);
//		shp3->set_angle(rotation);
	R.set_transform(5,rotation,{160,100});
	// dot axis
	dot(dsp,160,100,0xf);
	const Vector2D xaxis=R.axis_x().normalize().scale(7);
	dot(dsp,xaxis.x+160,xaxis.y+100,4);
	const Vector2D yaxis=R.axis_y().normalize().scale(7);
	dot(dsp,yaxis.x+160,yaxis.y+100,2);
}

extern "C" [[noreturn]] void tsk4(){
	//----------------------------------------------------------
	// init statics
	//----------------------------------------------------------
	// ? read from file
	rectangle_def={5,4,
		new Point2D[]{ // points in model coordinates, negative Y is "forward"
			{ 0,0},
			{-1,-.5f},
			{-1, .5f},
			{ 1, .5f},
			{ 1,-.5f},
		},
		new PointIx[]{1,2,3,4} // bounding convex polygon CCW
	};
	rectangle_def.init_normals();

	ship_def={4,3,
		new Point2D[]{
			{ 0, 0},
			{ 0,-1},
			{-1,.5},
			{ 1,.5},
		},
		new PointIx[]{1,2,3} // bounding convex polygon CCW
	};
	ship_def.init_normals();

	bullet_def={4,3,
		new Point2D[]{
			{ 0,-1},
			{-1,.5},
			{ 1,.5},
		},
		new PointIx[]{0,1,2} // bounding convex polygon CCW
	};
	bullet_def.init_normals();
//----------------------------------------------------------

	// init stack
	const Address heap_address=Heap::data().address();
	const Address heap_disp_at_addr=vga13h.bmp().data().pointer().offset(50*320).address();
	const SizeBytes heap_disp_size=320*100;

	Ship*shp=new Ship;
	shp->phy().pos={160,100};
	shp->phy().agl=deg_to_rad(180);

	for(float i=30;i<300;i+=20){
		new Wall(5,{i,120},deg_to_rad(i));
	}

	// start task
	while(true){
		metrics::reset();
		// copy heap
		pz_memcpy(heap_disp_at_addr,heap_address,heap_disp_size);

		PhysicsState::update_physics_states();
		Object::update_all();
		Object::render_all(vga13h.bmp());
		Object::check_collisions();

		//		out.pos({0,2}).p("                                              ").pos({0,2});
		out.pos({26,2}).fg(2).p("c=").p_hex_16b(metrics::collisions_check);
		out.pos({33,2}).fg(2).p("b=").p_hex_16b(metrics::collisions_check_bounding_shapes);
		out.pos({40,2}).fg(2).p("f=").p_hex_8b(static_cast<unsigned char>(Object::free_ixes_i));
		out.pos({45,2}).fg(2).p("s=").p_hex_8b(static_cast<unsigned char>(Object::used_ixes_i));
		out.pos({50,2}).fg(2).p("t=").p_hex_32b(osca_t);

		shp->phy().dpos={0,0};

		const char ch=table_scancode_to_ascii[osca_key];
		if(ch){
			if(ch=='c'){
				if(Object::hasFreeSlot()){
					shp=new Ship;
					shp->phy().pos={160,100};
					shp->phy().agl=deg_to_rad(180);
				}else{
					err.p("out of free slots");
					osca_halt();
				}
			}
			if(shp){
				switch(ch){
				case'w':
					shp->phy().dpos=shp->forward_vector().scale(.3f);
					break;
				case'a':
					shp->phy().agl-=deg_to_rad(2);
					break;
				case's':
					shp->phy().dpos=shp->forward_vector().negate().scale(.3f);
					break;
				case'd':
					shp->phy().agl+=deg_to_rad(2);
					break;
				case'x':
					for(float i=30;i<300;i+=20){
						new Wall(5,{i,120},deg_to_rad(i));
					}
					break;
				case' ':
					shp->fire();
					break;
				default:
					break;
				}
			}
		}

//		draw_axis(dsp);

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

