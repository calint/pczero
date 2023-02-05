#include "osca.h"
#include "lib.h"
#include "lib2d.h"
#include "libge.h"

using namespace osca;
static char*freemem_ptr=reinterpret_cast<char*>(0x10'0000);
static char*freemem_start=reinterpret_cast<char*>(0x10'0000);;
//static char*freemem_ptr=reinterpret_cast<char*>(0xa0000+320); // put heap on screen

// called by C++ to allocate and free memory
void*operator new[](unsigned count){
	char*p=freemem_ptr;
//	err.printer().p_hex_32b(count).p(':').p_hex_32b(reinterpret_cast<unsigned int>(p)).p(' ');
	freemem_ptr+=count;
	return reinterpret_cast<void*>(p);
}
void*operator new(unsigned count){
	char*p=freemem_ptr;
//	err.printer().p_hex_32b(count).p(':').p_hex_32b(reinterpret_cast<unsigned int>(p)).p(' ');
	freemem_ptr+=count;
	return reinterpret_cast<void*>(p);
}
void operator delete(void*ptr)noexcept{
	out.printer().p("D:").p_hex_32b(reinterpret_cast<unsigned int>(ptr)).p(' ');
}
void operator delete(void*ptr,unsigned size)noexcept{
	out.printer().p("DS:").p_hex_32b(size).p(' ').p_hex_32b(reinterpret_cast<unsigned int>(ptr)).p(' ');
}
void operator delete[](void*ptr)noexcept{
	out.printer().p("DA:").p_hex_32b(reinterpret_cast<unsigned int>(ptr)).p(' ');
}
void operator delete[](void*ptr,unsigned size)noexcept{
	out.printer().p("DSA:").p_hex_32b(size).p(' ').p_hex_32b(reinterpret_cast<unsigned int>(ptr)).p(' ');
}

// called by osca from the keyboard interrupt
extern "C" void osca_init(){
	*(int*)(0xa0000)=0x02;
	// initiate statics
	out=PrinterToVga();
}


extern "C" void tsk0();
extern "C" void tsk2();
extern "C" void tsk3();
extern "C" void tsk4();

class{
	unsigned char buf[2<<4]; // minimum size 2 and a power of 2, max size 256
	unsigned char s; // next event index
	unsigned char e; // last event index +1 & roll
public:
	// called by osca_keyb_ev
	auto on_key(unsigned char ch){
		const unsigned char ne=(e+1)&(sizeof(buf)-1);// next "end" index
		if(ne==s)// check overrun
			return;// write would overwrite. display on status line?
		buf[e]=ch;
		e=ne;
	}
	// returns keyboard scan code or 0 if no more events.
	auto get_next_scan_code()->unsigned char{
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
	*(int*)(0xa0000+4)=osca_key;
	osca_keyb.on_key(osca_key);
	static char*p=(char*)(0xa0000+320*49+100);
	*p++=(char)osca_key;
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk0(){
	Vga13h dsp;
	PrinterToBitmap pb{dsp.bmp()};

	pb.pos(1,30);
	for(int i=0;i<16;i++){
		pb.p_hex(i);
	}
	pb.fg(5).pos(2,13);
	for(char i='0';i<='9';i++){
		pb.p(i);
	}
	pb.fg(6);
	for(char i='a';i<='z';i++){
		pb.p(i);
	}
	pb.fg(7).pos(3,13+10);
	for(char i='A';i<='Z';i++){
		pb.p(i);
	}

	pb.fg(2).pos(4,15).p("hello world!").nl();
	pb.fg(6).p("\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
	pb.fg(7).p(' ').p_hex_32b(sizeof(table_ascii_to_font)/sizeof(int));

	pb.pos(7,5).fg(2).p('_');

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
extern "C" void tsk2(){
	while(true){
//		osca_yield();
		// copy kernel to screen
		Data src=Data(Address(0x07c00),512*3);// kernel binary
		Data dst1=Data(Address(0x11'0000),512*3);// to odd meg testing a20 enabled line
		Data dst2=Data(Address(0xa'0000+320*150),512*3);// on screen line 150
//		Data dst2=Data(Address(0xabb80),512*3);// on screen line 150
		src.to(dst1);
//		osca_yield();
		dst1.to(dst2);
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk3(){
	Vga13h dsp;
	while(true){
//		osca_yield();
//		*(int*)(0xa0000+320-4)=osca_t;
		dsp.bmp().data().pointer().offset(8).write(osca_t);
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk4(){
	// init statics
	default_object_def_rectangöe=ObjectDefRectangle();
	default_object_def_ship=ObjectDefShip();
	unsigned n=sizeof(objects_free_indexes)/sizeof(unsigned short);
	out.printer().p_hex_32b(n).spc().p_hex_16b(objects_free_indexes_pos).spc();
	for(unsigned short i=0;i<n;i++){
		objects_free_indexes[i]=i;
	}

//	static Object*objects[16];

	Ship*shp=new Ship();
	objects[1]=new Object{default_object_def_rectangöe,10,{100,100},0,4};
	objects[2]=shp;
	objects[3]=new Ship();

//	Ship*shp=static_cast<Ship*>(objects[1]); // dynamic_cast not available du to '-no-rtti'
	objects[1]->slot=1;
	objects[1]->set_dangle(deg_to_rad(5));
	objects[2]->slot=2;
	objects[2]->set_dangle(deg_to_rad(-5));
	objects[2]->set_dpos({1,1});
	objects[3]->slot=3;
	objects[3]->set_dangle(deg_to_rad(7));
	objects[3]->set_dpos({-1,0});

	// init stack
	Vga13h dsp;
	Bitmap&db=dsp.bmp();
	PrinterToBitmap pb{db};
	Degrees deg=0;
	Matrix2D R;
	const Address clear_start=db.data().pointer().offset(50*320).address();
	const SizeBytes clear_n=320*100;
	const Address heap_address=Address(freemem_start);

	// start task
	while(true){
//		pz_memset(clear_start,0x11,clear_n);
		pz_memcpy(heap_address,clear_start,clear_n);
//		obj1.update();
//		obj3->update();
//		obj1.render(db);
//		obj3->render(db);
		pb.pos(2,1).fg(2).p("t=").p_hex_32b(osca_t);
//		pb.pos(2,1).fg(2).p("t=").p_hex_32b(reinterpret_cast<unsigned>(freemem_start));

		for(Object*o:objects){
			if(!o)
				continue;
			o->update();
		}
		for(Object*o:objects){
			if(!o)
				continue;
			o->render(db);
		}

		if(deg>360)
			deg-=360;
		deg+=5;

		const char ch=table_scancode_to_ascii[osca_key];
		if(ch){
			const Point2D&dp=objects[1]->dpos();
			switch(ch){
			case'w':
				shp->set_dpos({dp.x,-1});
				break;
			case'a':
				shp->set_dpos({-1,dp.y});
				break;
			case's':
				shp->set_dpos({dp.x,1});
				break;
			case'd':
				shp->set_dpos({1,dp.y});
				break;
			case' ':
				if(shp){
					delete shp;
					shp=nullptr;
				}
				break;
			default:
				break;
			}
		}

		const float rotation=deg_to_rad(deg);
		R.set_transform(5,rotation,{160,100});
		// dot axis
		dot(db,160,100,0xf);
		const Vector2D xaxis=R.axis_x().normalize().scale(7);
		dot(db,xaxis.x+160,xaxis.y+100,2);
		const Vector2D yaxis=R.axis_y().normalize().scale(7);
		dot(db,yaxis.x+160,yaxis.y+100,4);

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

