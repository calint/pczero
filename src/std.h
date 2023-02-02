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

using Offset=int;

class Pointer{
	Address a_;
public:
	inline Pointer(const Address a):a_{a}{}
	inline auto address()const->Address{return a_;}
	inline auto write_byte(const unsigned char v){*static_cast<unsigned char*>(a_)=v;}
	inline auto write_char(const char v){*static_cast<char*>(a_)=v;}
	inline auto write_short(const short v){*static_cast<short*>(a_)=v;}
	inline auto write_int(const int v){*static_cast<int*>(a_)=v;}
	inline auto write_long(const long v){*static_cast<long*>(a_)=v;}
	inline auto offset(const Offset nbytes)const->Pointer{return Pointer{static_cast<char*>(a_)+nbytes};}
};

class Memmory{
	Pointer p_;
	Size s_;
public:
	inline Memmory(const Address a,const Size bytes):p_{a},s_{bytes}{}
	inline auto to(const Memmory&m)const{pz_memcpy(p_.address(),m.begin().address(),s_);}
	inline auto to(const Memmory&m,const Size bytes)const{pz_memcpy(p_.address(),m.begin().address(),bytes);}
	inline auto size_B()const->Size{return s_;}
	inline auto begin()const->Pointer{return p_;}
	inline auto pointer()const->Pointer{return p_;}
};

template<class T>
class CoordsT{
	T x_;
	T y_;
public:
	inline CoordsT(const T&x,const T&y):x_{x},y_{y}{}
	inline auto x()const->const T&{return x_;}
	inline auto y()const->const T&{return y_;}
	inline auto set_x(const T&x){x_=x;}
	inline auto set_y(const T&y){y_=y;}
	inline auto set(const T&x,const T&y){set_x(x);set_y(y);}
	inline auto inc_x(const T&dx){x_+=dx;}
	inline auto inc_y(const T&dy){y_+=dy;}
	inline auto inc_by(const CoordsT<T>&delta){x_+=delta.x_;y_+=delta.y_;}
};

using CoordPx=int;
using CoordsPx=CoordsT<CoordPx>;

using Coord=float;
using Coords=CoordsT<Coord>;

template<class T>
class DimensionT{
	T w_;
	T h_;
public:
	inline DimensionT(const T&width,const T&height):w_{width},h_{height}{}
	inline auto width()const->const T&{return w_;}
	inline auto height()const->const T&{return h_;}
};

using Pixels=int;
using DimensionPx=DimensionT<Pixels>;

class Bitmap{
	DimensionPx d_;
	Memmory m_;
public:
	inline Bitmap(const Address a,const DimensionPx&px):d_{px},m_{a,px.width()*px.height()}{}
	inline auto dim_px()const->const DimensionPx&{return d_;}
	inline auto mem()const->const Memmory&{return m_;}
	auto to(const Bitmap&dst,const CoordsPx&c)const{
		char*si=static_cast<char*>(m_.begin().address());
		char*di=static_cast<char*>(dst.m_.begin().address());
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
	inline Vga13h():b_{Address(0xa0000),DimensionPx{320,200}}{}
	inline auto bmp()const->const Bitmap&{return b_;}
};

using PositionPx=CoordsPx;
using Position=Coords;
using Velocity=Coords;
using Acceleration=Coords;

class Sprite{
	const Bitmap&b_;
	Position p_;
	Velocity v_;
	Acceleration a_;
public:
	inline Sprite(const Bitmap&b,const Position&p,const Velocity&v,const Acceleration&a):b_{b},p_{p},v_{v},a_{a}{}
	auto to(const Bitmap&dst)const{
		const char*si=static_cast<const char*>(b_.mem().begin().address());
		char*di=static_cast<char*>(dst.mem().begin().address());
		PositionPx p{static_cast<CoordPx>(p_.x()),static_cast<CoordPx>(p_.y())};
		di+=p.y()*dst.dim_px().width()+p.x();
		const int ln=dst.dim_px().width()-b_.dim_px().width();
		const int h=b_.dim_px().height();
		const int w=b_.dim_px().width();
		for(int y=0;y<h;y++){
			for(int x=0;x<w;x++){
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
	inline auto set_pos(const Position&p){p_=p;}
	inline auto velocity()const->const Velocity&{return v_;}
	inline auto set_velocity(const Velocity&v){v_=v;}
	inline auto update(){v_.inc_by(a_);p_.inc_by(v_);}
};
