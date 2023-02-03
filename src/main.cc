#include "osca.h"
#include "lib.h"

extern "C" void tsk0();
extern "C" void tsk5();
extern "C" void tsk6();
extern "C" void tsk7();
extern "C" void tsk8();
extern "C" void tsk9();
extern "C" void tsk10();
extern "C" void tsk11();
extern "C" void tsk12();
extern "C" void tsk13();
extern "C" void tsk14();
extern "C" void tsk15();

static struct {
	unsigned char buf[0x10]; // minimum size 2
	unsigned char s;
	unsigned char e;
	auto on_key(){
		buf[e]=osca_key;
		e++;
		e&=sizeof(buf)-1;// roll
		if(e==s){ // check overrun
			e--;
			e&=sizeof(buf)-1;
		}
	}
	auto get_next_key()->unsigned char{
//		asm("nop"); // ?! sometimes breaks if not a nop here when -O2,-O3,-Os,-Osize. works with -O0,-O1
					// it breaks in both clang++ and g++. probably a bug in pczero
		if(s==e)
			return 0;
		unsigned char ch=buf[s];
		s++;
		s&=sizeof(buf)-1;// roll
		return ch;
	}
}osca_keyb;

extern "C" void osca_keyb_ev(){
	osca_keyb.on_key();
	*(int*)(0xa0000+320-8)=osca_key;
	static char*p=(char*)0xa4000;
	*p++=(char)osca_key;
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk0(){
	Vga13h dsp;
	PrinterToBitmap pb{dsp.bmp()};
	pb.pos(10,10);
	while(true){
		// read keyboard
		while(true){
			const unsigned char sc=osca_keyb.get_next_key();
			if(!sc)
				break;
			const char ch=table_scancode_to_ascii[sc];
			if(!ch) // not an ascii. probably key release
				continue;
			pb.p(ch);
		}
//		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

asm(".global tsk1,tsk2,tsk3,tsk4");
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
asm(".align 16");
asm("tsk1:");
asm("  addl $2,0xa0044");
asm("  hlt");
asm("  jmp tsk1");
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
asm(".align 16");
asm("tsk2:");
asm("  addl $2,0xa0048");
asm("  hlt");
asm("  jmp tsk2");
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
asm(".align 16");
asm("tsk3:");
asm("  addl $2,0xa004c");
asm("  hlt");
asm("  jmp tsk3");
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
asm(".align 16");
asm("tsk4:");
asm("  addl $2,0xa0050");
asm("  hlt");
asm("  jmp tsk4");
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
static char bmp0[]{
	0,1,1,0,
	1,1,1,1,
	1,1,1,1,
	0,1,1,0,
};
static char bmp1[]{
	0,2,2,0,
	2,2,2,2,
	2,2,2,2,
	0,2,2,0,
};
static char bmp2[]{
	0,3,3,0,
	3,3,3,3,
	3,3,3,3,
	0,3,3,0,
};
static char bmp3[]{
	0,4,4,0,
	4,4,4,4,
	4,4,4,4,
	0,4,4,0,
};
extern "C" void tsk5(){
	static Vga13h dsp;
	static const Bitmap&dbmp=dsp.bmp();
	static PrinterToBitmap pb{dbmp};
	static PrinterToBitmap pb2{dbmp};
	static Bitmap bitmaps[]{
		{Address{bmp0},DimensionPx{4,4}},
		{Address{bmp1},DimensionPx{4,4}},
		{Address{bmp2},DimensionPx{4,4}},
		{Address{bmp3},DimensionPx{4,4}},
	};
	static Sprite sprites[]{
		{bitmaps[0],Position{20.0f,25.f},Velocity{5,0.5f},Acceleration{0,0.5f}},
		{bitmaps[1],Position{30.0f,25.f},Velocity{5,1.0f},Acceleration{0,0.5f}},
		{bitmaps[2],Position{40.0f,25.f},Velocity{5,1.5f},Acceleration{0,0.5f}},
		{bitmaps[3],Position{50.0f,25.f},Velocity{5,2.0f},Acceleration{0,0.5f}},
	};

	CoordPx x=24;
	CoordPx x_prv=x;
	pb.pos(1,30);
	for(int i=0;i<16;i++){
		pb.p_hex(i);
	}
	pb.pos(2,10).fg(5);
	for(char i='0';i<='9';i++){
		pb.p(i);
	}
	for(char i='a';i<='z';i++){
		pb.p(i);
	}
	pb.p(' ');
	pb.fg(6).p("hello world!").nl();
	pb.p("\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
	pb.fg(7).p(' ').p_hex_32b(sizeof(table_ascii_to_font)/sizeof(int));

	while(true){
		bitmaps[0].to(dbmp,CoordsPx{x_prv,44});
		bitmaps[1].to(dbmp,CoordsPx{x,44});
		bitmaps[2].to(dbmp,CoordsPx{x,36});
		bitmaps[3].to(dbmp,CoordsPx{x,28});
		x_prv=x;
		x+=8;
		if(x>180){
			x=24;
		}
		for(const auto&s:sprites){
			s.to(dbmp);
		}
		for(auto&s:sprites){
			s.update();
		}
		for(auto&s:sprites){
			if(s.pos().y()>140){
				Position p{s.pos().x(),140};
				if(p.x()>static_cast<Coord>(dsp.bmp().dim().width())){
					p.set_x(0);
				}
				s.set_pos(p);
				Velocity v{s.velocity()};
				v.set_y(-5);
				s.set_velocity(v);
			}
		}
		pb.fg(7).pos(5,10).transparent(true).p("transparent");
		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk6(){
	const Vga13h dsp;
	while(true){
		osca_yield();
//		*(int*)(0xa0000+320-4)=osca_t;
		dsp.bmp().data().begin().offset(320-4).write(osca_t);
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk7(){
	while(true){
		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk8(){
	while(true){
		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk9(){
	while(true){
		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk10(){
	while(true){
		osca_yield();
		// copy kernel to screen
		Data src=Data(Address(0x07c00),512*3);// kernel binary
		Data dst1=Data(Address(0x100000),512*3);// to odd meg testing a20 enabled line
		Data dst2=Data(Address(0xa0000+320*150),512*3);// on screen line 150
//		Data dst2=Data(Address(0xabb80),512*3);// on screen line 150
		src.to(dst1);
		osca_yield();
		dst1.to(dst2);
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk11(){
	while(true){
		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk12(){
	while(true){
		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk13(){
	while(true){
		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk14(){
	while(true){
		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk15(){
	while(true){
		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
//extern "C" void tsk16(){
//	while(true){
//		osca_yield();
//	}
//}
//// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
//extern "C" void tsk17(){
//	while(true){
//		osca_yield();
//	}
//}
//// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
//extern "C" void tsk18(){
//	while(true){
//		osca_yield();
//	}
//}
//// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
//extern "C" void tsk19(){
//	while(true){
//		osca_yield();
//	}
//}
//// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
