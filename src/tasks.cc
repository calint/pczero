#include "boot.h"
#include "std.h"

extern "C" void osca_keyb_ev(){
	*(int*)(0xa0000+320-8)=osca_key;
	static char*p=(char*)0xa4000;
	*p++=(char)osca_key;
}

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
//extern "C" void tsk16();
//extern "C" void tsk17();
//extern "C" void tsk18();
//extern "C" void tsk19();

asm(".global tsk0,tsk1,tsk2,tsk3,tsk4");
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
asm(".align 16");
asm("tsk0:");
asm("  hlt");
asm("  jmp tsk0");
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
extern "C" void tsk5(){
//	static char sprite_clear[]{
//			0,0,0,0,
//			0,0,0,0,
//			0,0,0,0,
//			0,0,0,1,
//	};
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
//	static Bitmap screens[]{
//		{Addr(0xa0000),320,200},
//	};
	static Bitmap bitmaps[]{
		{Address{bmp0},Dimension{4,4}},
		{Address{bmp1},Dimension{4,4}},
		{Address{bmp2},Dimension{4,4}},
		{Address{bmp3},Dimension{4,4}},
	};
	static Sprite sprites[]{
		{bitmaps[0],Coords{20,20},Velocity{0,1}},
		{bitmaps[1],Coords{30,20},Velocity{0,2}},
		{bitmaps[2],Coords{40,20},Velocity{0,3}},
		{bitmaps[3],Coords{50,20},Velocity{0,4}},
	};
	Coord x=24;
	Coord x_prv=x;
	Vga13h dsp;
	const Bitmap&dbmp=dsp.bmp();
	while(true){
		bitmaps[0].to(dbmp,Coords{x_prv,44});
		bitmaps[1].to(dbmp,Coords{x,44});
		bitmaps[2].to(dbmp,Coords{x,36});
		bitmaps[3].to(dbmp,Coords{x,28});
		x_prv=x;
		x+=8;
		if(x>180){
			x=24;
		}
		const unsigned n=sizeof(sprites)/sizeof(Sprite);
		for(unsigned i=0;i<n;i++){
			Sprite&s=sprites[i];
			s.update();
			if(s.pos().y()>100){
				Position p{s.pos().x()+5,20};
				if(p.x()>dsp.bmp().dim_px().width()){
					p.set_x(Coord{0});
				}
				sprites[i].set_pos(p);
			}
		}
		for(unsigned i=0;i<n;i++){
			sprites[i].to(dbmp);
		}
		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk6(){
	const Vga13h dsp;
	while(true){
		osca_yield();
//		*(int*)(0xa0000+320-4)=osca_t;
		dsp.bmp().span().begin().offset(320-4).write_int(osca_t);
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk7(){
	static char*p=(char*)(0xa0000+320);
	static char*nl=(char*)(0xa0000+320);
	static char key_prev=0;
	while(true){
		osca_yield();
		if(key_prev==(char)osca_key){
			char c=*p;
			*p=c+1;
		}else{
			*p=(char)osca_key;
			if(osca_key==1){
				nl+=320;
				p=nl;
				/*p+=320;
				int r=((int)p-0xa0280)%320;
				p-=r;*/
			}else{
				p++;
				*p=0;
			}
			key_prev=(char)osca_key;
		}
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk8(){
	while(true){
		osca_yield();
//		int*p=(int*)0xa0080;
//		int c=0x010;
//		while(c--)
//			*p++=osca_t;
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk9(){
	while(true){
		osca_yield();
//		for(int n=0;n<8;n++)
//			*(int*)(0xa0100+n*4)=osca_t;
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk10(){
	while(true){
		osca_yield();
		// copy kernel to screen
		Span src=Span(Address(0x07c00),512*3);// kernel binary
//		File dst1=File(Addr(0x100000),512*3);// to odd meg testing a20 enabled line
		Span dst1=Span(Address(0x100000),512*3);// to odd meg testing a20 enabled line
		Span dst2=Span(Address(0xabb80),512*3);// on screen line 150
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
