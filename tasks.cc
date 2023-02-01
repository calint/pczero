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
//			0,0,0,0,
//	};
	static char sprite_clear[]{
			0,1,1,0,
			1,1,1,1,
			1,1,1,1,
			0,1,1,0,
	};
	static char sprite_data[]{
			0,2,2,0,
			2,2,2,2,
			2,2,2,2,
			0,2,2,0,
	};
	Bitmap sprite{Addr{sprite_data},4,4};
	Bitmap clear{Addr{sprite_clear},4,4};
	Bitmap screen{Addr(0xa0000),320,200};
	Coord x=24;
	Coord x_prv=x;
	while(true){
//		sprite.to(screen,Coords{24,24});
//		sprite.to(screen,Coords{32,24});
//		sprite.to(screen,Coords{40,24});
		clear.to(screen,Coords{x_prv,24});
		sprite.to(screen,Coords{x,24});
		x_prv=x;
		x+=8;
		if(x>180){
			x=24;
		}
//		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" void tsk6(){
	while(true){
		osca_yield();
		*(int*)(0xa0000+320-4)=osca_t;
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
		File src=File(Addr(0x07c00),512*3);// kernel binary
//		File dst1=File(Addr(0x100000),512*3);// to odd meg testing a20 enabled line
		File dst1=File(Addr(0x1abb80),512*3);// to odd meg testing a20 enabled line
		File dst2=File(Addr(0xabb80),512*3);// on screen line 150
		src.to(dst1);
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
