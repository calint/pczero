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
	inline auto write_byte(char b){*static_cast<char*>(addr)=b;}
	inline auto write_int(int i){*static_cast<int*>(addr)=i;}
	inline auto get_offset_ref(Size offset_B)->Ref{return Ref{static_cast<char*>(addr)+offset_B};}
};

class File:public Ref{
	Size size_B;
public:
	inline File(Addr a,Size nbytes):Ref{a},size_B{nbytes}{}
	inline auto to(File f){pz_memcpy(get_addr(),f.get_addr(),size_B);}
	inline auto to(File f,Size nbytes){pz_memcpy(get_addr(),f.get_addr(),nbytes);}
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
	inline auto increment_x(Coord dx){x+=dx;}
	inline auto increment_y(Coord dy){y+=dy;}
	inline auto increment(const Coords&dc){x+=dc.x;y+=dc.y;}
};

typedef int Width;
typedef int Height;

class Dimension{
	Width wi;
	Height hi;
public:
	inline Dimension(Width w,Height h):wi{w},hi{h}{}
	inline auto get_width()const->Width{return wi;}
	inline auto get_height()const->Height{return hi;}
};

class Bitmap:public File{
	Dimension dim_px;
public:
	inline Bitmap(Addr a,const Dimension&px):File{a,px.get_width()*px.get_height()},dim_px{px}{}
	inline auto get_dimension_px()const->const Dimension&{return dim_px;}
	auto to(Bitmap&dst,const Coords&c){
		char*si=static_cast<char*>(get_addr());
		char*di=static_cast<char*>(dst.get_addr());
		di+=c.get_y()*dst.get_dimension_px().get_width()+c.get_x();
		const int ln=dst.get_dimension_px().get_width()-dim_px.get_width();
		const int h=dim_px.get_height();
		const int w=dim_px.get_width();
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
	inline Vga13h():Bitmap{Addr(0xa0000),Dimension{320,200}}{}
};

typedef Coords Position;
typedef Coords Velocity;

class Sprite{
	Bitmap bmp;
	Position pos;
	Velocity dpos;
public:
	inline Sprite(Bitmap b,Position p,Velocity v):bmp{b},pos{p},dpos{v}{}
	auto to(Bitmap&dst){
		const char*si=static_cast<const char*>(bmp.get_addr());
		char*di=static_cast<char*>(dst.get_addr());
		di+=pos.get_y()*dst.get_dimension_px().get_width()+pos.get_x();
		const int ln=dst.get_dimension_px().get_width()-bmp.get_dimension_px().get_width();
		const int h=bmp.get_dimension_px().get_height();
		const int w=bmp.get_dimension_px().get_width();
		for(int y1=0;y1<h;y1++){
			for(int x1=0;x1<w;x1++){
				const char px=*si;
				if(px){
					*di=px;
				}
				si++;
				di++;
			}
			di+=ln;
		}
	}
	inline auto get_position()const->const Position&{return pos;}
	inline auto set_position(const Position&c){pos=c;}
	inline auto update(){pos.increment(dpos);}
};
