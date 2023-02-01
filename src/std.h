#pragma once

typedef void*Address;
typedef int Size;

inline void pz_memcpy(Address from,Address to,Size nbytes){
	asm("movl %0,%%esi;"
		"movl %1,%%edi;"
		"movl %2,%%ecx;"
		"rep movsb;"
		:
		:"r"(from),"r"(to),"r"(nbytes)
		:"%esi","%edi","%ecx"
	);
}

class Pointer{
	Address addr;
public:
	inline Pointer(const Address a):addr{a}{}
	inline auto address()const->Address{return addr;}
	inline auto write_byte(const char b){*static_cast<char*>(addr)=b;}
	inline auto write_int(const int i){*static_cast<int*>(addr)=i;}
	inline auto offset(const Size offset_B)const->Pointer{return Pointer{static_cast<char*>(addr)+offset_B};}
};

class Span{
	Pointer bgn;
	Size sz_B;
public:
	inline Span(const Address a,const Size nbytes):bgn{a},sz_B{nbytes}{}
	inline auto to(const Span&s){pz_memcpy(bgn.address(),s.begin().address(),sz_B);}
	inline auto to(const Span&s,const Size nbytes){pz_memcpy(bgn.address(),s.begin().address(),nbytes);}
	inline auto size_B()const->Size{return sz_B;}
	inline auto begin()const->Pointer{return bgn;}
};

typedef int Coord;

class Coords{
	Coord x_;
	Coord y_;
public:
	inline Coords(const Coord x,const Coord y):x_{x},y_{y}{}
	inline auto x()const->Coord{return x_;}
	inline auto y()const->Coord{return y_;}
	inline auto set_x(const Coord x){x_=x;}
	inline auto set_y(const Coord y){y_=y;}
	inline auto set(const Coord x,const Coord y){set_x(x);set_y(y);}
	inline auto inc_x(const Coord dx){x_+=dx;}
	inline auto inc_y(const Coord dy){y_+=dy;}
	inline auto inc(const Coords&dc){x_+=dc.x_;y_+=dc.y_;}
};

typedef int Width;
typedef int Height;

class Dimension{
	Width wi;
	Height hi;
public:
	inline Dimension(const Width w,const Height h):wi{w},hi{h}{}
	inline auto width()const->Width{return wi;}
	inline auto height()const->Height{return hi;}
};

class Bitmap{
	Span spn;
	Dimension dpx;
public:
	inline Bitmap(const Address a,const Dimension&px):spn{a,px.width()*px.height()},dpx{px}{}
	inline auto dim_px()const->const Dimension&{return dpx;}
	inline auto span()const->const Span&{return spn;}
	auto to(const Bitmap&dst,const Coords&c){
		char*si=static_cast<char*>(spn.begin().address());
		char*di=static_cast<char*>(dst.spn.begin().address());
		di+=c.y()*dst.dim_px().width()+c.x();
		const int ln=dst.dim_px().width()-dpx.width();
		const int h=dpx.height();
		const int w=dpx.width();
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

class Vga13h{
	Bitmap b;
public:
	inline Vga13h():b{Address(0xa0000),Dimension{320,200}}{}
//	inline auto bmp_for_write()->Bitmap&{return b;}
	inline auto bmp()const->const Bitmap&{return b;}
};

typedef Coords Position;
typedef Coords Velocity;

class Sprite{
	Bitmap bp;
	Position ps;
	Velocity dps;
public:
	inline Sprite(const Bitmap&b,const Position&p,const Velocity&v):bp{b},ps{p},dps{v}{}
	auto to(const Bitmap&dst){
		const char*si=static_cast<const char*>(bp.span().begin().address());
		char*di=static_cast<char*>(dst.span().begin().address());
		di+=ps.y()*dst.dim_px().width()+ps.x();
		const int ln=dst.dim_px().width()-bp.dim_px().width();
		const int h=bp.dim_px().height();
		const int w=bp.dim_px().width();
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
	inline auto pos()const->const Position&{return ps;}
	inline auto set_pos(const Position&c){ps=c;}
	inline auto velocity()const->const Velocity&{return dps;}
	inline auto set_velocity(const Velocity&v){dps=v;}
	inline auto update(){ps.inc(dps);}
};
