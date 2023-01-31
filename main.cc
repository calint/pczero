#include "pc.h"
#include "std.h"
//extern "C" void __gxx_personality_v0(){}

asm(".global tsk0,tsk1,tsk2,tsk3,tsk4");

asm("tsk0:");
asm("  mov $0x00007c00,%esi");
asm("  mov $0x000a8000,%edi");
asm("  mov $0x00001000,%ecx");
asm("  rep movsl");
asm("  hlt");
asm("  jmp tsk0");

asm(".align 16");
asm("tsk1:");
asm("  addl $2,0xa0144");
asm("  hlt");
asm("  jmp tsk1");
asm(".align 16");

asm("tsk2:");
asm("  addl $2,0xa0148");
asm("  hlt");
asm("  jmp tsk2");
asm(".align 16");

asm("tsk3:");
asm("  addl $2,0xa014c");
asm("  hlt");
asm("  jmp tsk3");
asm(".align 16");

asm("tsk4:");
asm("  addl $2,0xa0150");
asm("  hlt");
asm("  jmp tsk4");

extern "C" void tsk5(){
	while(true){
		osca_yield();
		*(int*)0xa0154+=3;
	}
}
extern "C" void tsk6(){
	static char*p=(char*)0xa0400;
	static char*nl=(char*)0xa0400;
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

extern "C" void tsk7(){
	while(true){
		osca_yield();
		int*p=(int*)0xa0080;
		int c=0x010;
		while(c--)
			*p++=osca_t;
	}
}

extern "C" void tsk8(){
	Bitmap screen{Addr(0xa0000),320,200};
	Bitmap sprite{Addr(0xa0000),4,4};
	while(true){
		osca_yield();
		sprite.to(screen,Coords{25,25});
		//*(int*)0xa0480=osca_t;
	}
}

extern "C" void tsk9(){
	while(true){
		osca_yield();
		for(int n=0;n<8;n++)
			*(int*)(0xa0100+n*4)=osca_t;
	}
}

extern "C" void tsk10(){
	while(true){
		osca_yield();
		File src=File(Addr(0x07c00),1*1024);
		File dst=File(Addr(0xa0800),1*1024);
		src.to(dst);
	}
}

extern "C" void osca_keyb_ev(){
	static char*p=(char*)0xa4000;
	*p++=(char)osca_key;
}
