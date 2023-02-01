#pragma once

using Address=void*;
using Size=int;

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

using Offset=Size;

class Pointer{
	Address a_;
public:
	inline Pointer(const Address a):a_{a}{}
	inline auto address()const->Address{return a_;}
	inline auto write_byte(const char v){*static_cast<char*>(a_)=v;}
	inline auto write_short(const short v){*static_cast<short*>(a_)=v;}
	inline auto write_int(const int v){*static_cast<int*>(a_)=v;}
	inline auto offset(const Offset nbytes)const->Pointer{return Pointer{static_cast<char*>(a_)+nbytes};}
};

class Span{
	Pointer p_;
	Size s_;
public:
	inline Span(const Address a,const Size bytes):p_{a},s_{bytes}{}
	inline auto to(const Span&s){pz_memcpy(p_.address(),s.begin().address(),s_);}
	inline auto to(const Span&s,const Size bytes){pz_memcpy(p_.address(),s.begin().address(),bytes);}
	inline auto size_B()const->Size{return s_;}
	inline auto begin()const->Pointer{return p_;}
	inline auto pointer()const->Pointer{return p_;}
};

using Coord=int;

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

using Height=int;
using Width=int;

class Dimension{
	Width w_;
	Height h_;
public:
	inline Dimension(const Width w,const Height h):w_{w},h_{h}{}
	inline auto width()const->Width{return w_;}
	inline auto height()const->Height{return h_;}
};

class Bitmap{
	Span s_;
	Dimension d_;
public:
	inline Bitmap(const Address a,const Dimension&px):s_{a,px.width()*px.height()},d_{px}{}
	inline auto dim_px()const->const Dimension&{return d_;}
	inline auto span()const->const Span&{return s_;}
	auto to(const Bitmap&dst,const Coords&c){
		char*si=static_cast<char*>(s_.begin().address());
		char*di=static_cast<char*>(dst.s_.begin().address());
		di+=c.y()*dst.dim_px().width()+c.x();
		const int ln=dst.dim_px().width()-d_.width();
		const int h=d_.height();
		const int w=d_.width();
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
	Bitmap b_;
public:
	inline Vga13h():b_{Address(0xa0000),Dimension{320,200}}{}
	inline auto bmp()const->const Bitmap&{return b_;}
};

using Position=Coords;
using Velocity=Coords;

class Sprite{
	Bitmap b_;
	Position p_;
	Velocity v_;
public:
	inline Sprite(const Bitmap&b,const Position&p,const Velocity&v):b_{b},p_{p},v_{v}{}
	auto to(const Bitmap&dst){
		const char*si=static_cast<const char*>(b_.span().begin().address());
		char*di=static_cast<char*>(dst.span().begin().address());
		di+=p_.y()*dst.dim_px().width()+p_.x();
		const int ln=dst.dim_px().width()-b_.dim_px().width();
		const int h=b_.dim_px().height();
		const int w=b_.dim_px().width();
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
	inline auto pos()const->const Position&{return p_;}
	inline auto set_pos(const Position&c){p_=c;}
	inline auto velocity()const->const Velocity&{return v_;}
	inline auto set_velocity(const Velocity&v){v_=v;}
	inline auto update(){p_.inc(v_);}
};
