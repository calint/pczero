#include "std.h"

extern "C" void pz_memcpy(Addr from,Addr to,Size nbytes){
//	int c=size>>2;
//	int*s=(int*)from;
//	int*d=(int*)to;
//	while(c--)
//		*d++=*s++;
	asm("movl %0,%%esi;"
		"movl %1,%%edi;"
		"movl %2,%%ecx;"
		"rep movsb;":
			:"r"(from),"r"(to),"r"(nbytes):"%esi","%edi","%ecx");
}

inline void pz_write(Addr a,const char b){
	*static_cast<char*>(a)=b;
}

/*
kcp     push ebp
		mov ebp,esp
		push esi
		push edi
		push ecx
		mov esi,[ebp+8]
		mov edi,[ebp+12]
		mov ecx,[ebp+16]
		rep movsd
		pop ecx
		pop edi
		pop esi
		mov esp,ebp
		pop ebp
        ret

extern "C" void kcp(int*src,int*dst,int dwords);
*/


//auto File::to(File f)->void{
//	// ? check buffer overrrun
//	pz_memcpy(static_cast<char*>(get_addr()),static_cast<char*>(f.get_addr()),size_B);
//}
//auto File::to(File f,Size nbytes)->void{
//	// ? check buffer overrrun
//	pz_memcpy(static_cast<char*>(get_addr()),static_cast<char*>(f.get_addr()),nbytes);
//}

auto File::to(File f)->void{
	// ? check buffer overrrun
//	pz_memcpy(reinterpret_cast<int>(get_addr()),reinterpret_cast<int>(f.get_addr()),size_B);
	pz_memcpy(get_addr(),f.get_addr(),size_B);
}
auto File::to(File f,Size nbytes)->void{
	// ? check buffer overrrun
//	pz_memcpy(reinterpret_cast<int>(get_addr()),reinterpret_cast<int>(f.get_addr()),nbytes);
	pz_memcpy(get_addr(),f.get_addr(),nbytes);
}

auto Bitmap::to(Bitmap&b,const Coords&c)->void{
//	Ref p=b.get_offset_ref(c.get_y()*b.get_width_px()+c.get_x());
//	p.write_int(0x04040404);

	char*si=static_cast<char*>(get_addr());
	char*di=static_cast<char*>(b.get_addr());
	di+=c.get_y()*b.get_width_px()+c.get_x();
	const int ln=b.get_width_px()-get_width_px();
	const int h=get_height_px();
	const int w=get_width_px();
	for(int y=0;y<h;y++){
		for(int x=0;x<w;x++){
			*di=*si;
			si++;
			di++;
		}
		di+=ln;
	}
}
//File screen=File(Addr(0xa0000),Size(100*320));
