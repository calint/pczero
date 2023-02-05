#include "osca.h"
#include "lib.h"
#include "lib2d.h"

using namespace osca;

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
	static char*p=(char*)0xa4000;
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
	pb.fg(5).pos(2,10);
	for(char i='0';i<='9';i++){
		pb.p(i);
	}
	pb.fg(6);
	for(char i='a';i<='z';i++){
		pb.p(i);
	}
	pb.fg(7).pos(3,10+10);
	for(char i='A';i<='Z';i++){
		pb.p(i);
	}

	pb.fg(2).pos(4,15);
	pb.p("hello world!").nl();
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
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
		Data dst1=Data(Address(0x100000),512*3);// to odd meg testing a20 enabled line
		Data dst2=Data(Address(0xa0000+320*150),512*3);// on screen line 150
//		Data dst2=Data(Address(0xabb80),512*3);// on screen line 150
		src.to(dst1);
//		osca_yield();
		dst1.to(dst2);
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk3(){
	const Vga13h dsp;
	while(true){
//		osca_yield();
//		*(int*)(0xa0000+320-4)=osca_t;
		dsp.bmp().data().pointer().offset(8).write(osca_t);
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
static Vector2D dots_src[]{
	{ 0, 0},
	{-2,-1},
	{ 2,-1},
	{ 2, 1},
	{-2, 1},
};
//static Vector2D dots_src[]{
//	{ 0, 0},
//	{ 0,-1},
//	{-1,.5f},
//	{ 1,.5f},
//};
//static Vector2D vectors_axis[]{
//	{1,0},
//	{0,1},
//};

static Vector2D dots_dst[sizeof(dots_src)/sizeof(Vector2D)];

static void dot(const Bitmap&bmp,const float x,const float  y,const unsigned char color){
	const int xi=static_cast<int>(x);
	const int yi=static_cast<int>(y);
	bmp.data().pointer().offset(yi*bmp.dim().width()+xi).write(color);
}
extern "C" void tsk4(){
	Vga13h dsp;
	const Bitmap&db=dsp.bmp();
	PrinterToBitmap pb{db};
	Degrees deg=0;
	unsigned char colr=1;
	Matrix2D R;
	Vector2D translation={100,100};
	Scale scale{10};
	const Address clear_start=db.data().pointer().offset(50*320).address();
	const SizeBytes clear_n=320*100;
	while(true){
		pz_memset(clear_start,0x11,clear_n);
		if(deg>360)
			deg-=360;
		const float rotation=deg_to_rad(deg);
		R.set_transform(scale,rotation,translation);
		R.transform(dots_src,dots_dst,sizeof(dots_src)/sizeof(Vector2D));
		for(const auto&d:dots_dst){
			dot(dsp.bmp(),d.x,d.y,colr);
		}

		// dot axis
		dot(db,140,100,1);
		const Vector2D xaxis=R.axis_x().normalize().scale(15);
		dot(db,xaxis.x+140,xaxis.y+100,4);
		const Vector2D yaxis=R.axis_y().normalize().scale(15);
		dot(db,yaxis.x+140,yaxis.y+100,5);

		deg+=5;
		colr++;
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
