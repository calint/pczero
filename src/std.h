#pragma once

typedef void*Addr;
typedef int Size;

inline void pz_memcpy(Addr from,Addr to,Size nbytes){
	asm("movl %0,%%esi;"
		"movl %1,%%edi;"
		"movl %2,%%ecx;"
		"rep movsb;"
		:
		:"r"(from),"r"(to),"r"(nbytes)
		:"%esi","%edi","%ecx"
	);
}

class Ref{
	Addr addr;
public:
	inline Ref(Addr a):addr{a}{}
	inline auto get_addr()const->Addr{return addr;}
	inline auto write_byte(char b)->void{*static_cast<char*>(addr)=b;}
	inline auto write_int(int i)->void{*static_cast<int*>(addr)=i;}
	inline auto get_offset_ref(Size offset_B)->Ref{return Ref{static_cast<char*>(addr)+offset_B};}
};

class File:public Ref{
	Size size_B;
public:
	inline File(Addr a,Size nbytes):Ref{a},size_B{nbytes}{}
	inline auto to(File f)->void{pz_memcpy(get_addr(),f.get_addr(),size_B);}
	inline auto to(File f,Size nbytes)->void{pz_memcpy(get_addr(),f.get_addr(),nbytes);}
	inline auto get_size_B()const->Size{return size_B;}
};

typedef int Coord;

class Coords{
	Coord x;
	Coord y;
public:
	inline Coords(Coord x_,Coord y_):x{x_},y{y_}{}
	inline auto get_x()const->Coord{return x;}
	inline auto get_y()const->Coord{return y;}
	inline auto set_x(Coord x_){x=x_;}
	inline auto set_y(Coord y_){y=y_;}
	inline auto set(Coord x_,Coord y_){set_x(x_);set_y(y_);}
};

typedef int Width;
typedef int Height;

class Bitmap:public File{
	Width width_px;
	Height height_px;
public:
	inline Bitmap(Addr a,Width w_px,Height h_px):
		File{a,w_px*h_px},width_px{w_px},height_px{h_px}{}
	inline auto get_width_px()const->Width{return width_px;}
	inline auto get_height_px()const->Height{return height_px;}
	auto to(Bitmap&b,const Coords&c)->void{
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
};

class Vga13h:public Bitmap{
public:
	Vga13h():Bitmap{Addr(0xa0000),320,200}{}
};
