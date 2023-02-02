#pragma once

using Address=void*;
using Size=int;
using SizeBytes=Size;

inline void pz_memcpy(Address from,Address to,SizeBytes nbytes){
	asm("movl %0,%%esi;"
		"movl %1,%%edi;"
		"movl %2,%%ecx;"
		"rep movsb;"
		:
		:"r"(from),"r"(to),"r"(nbytes)
		:"%esi","%edi","%ecx" // ? clobbers memory?
	);
}

using OffsetBytes=int;

class Pointer{
	Address a_;
public:
	inline Pointer(const Address a):a_{a}{}
	inline auto address()const->Address{return a_;}
	inline auto write_byte(const unsigned char v){*static_cast<unsigned char*>(a_)=v;}
	inline auto write_char(const char v){*static_cast<char*>(a_)=v;}
	inline auto write_short(const short v){*static_cast<short*>(a_)=v;}
	inline auto write_int(const int v){*static_cast<int*>(a_)=v;}
	inline auto write_int(const unsigned v){*static_cast<unsigned*>(a_)=v;}
	inline auto write_long(const long v){*static_cast<long*>(a_)=v;}
	inline auto offset(const OffsetBytes ob)const->Pointer{return Pointer{static_cast<char*>(a_)+ob};}
};

class Data{
	Pointer p_;
	SizeBytes s_;
public:
	inline Data(const Address a,const SizeBytes sb):p_{a},s_{sb}{}
	inline auto to(const Data&d)const{pz_memcpy(p_.address(),d.begin().address(),s_);}
	inline auto to(const Data&d,const SizeBytes bytes)const{pz_memcpy(p_.address(),d.begin().address(),bytes);}
	inline auto size()const->SizeBytes{return s_;}
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

using SizePx=int;
using DimensionPx=DimensionT<SizePx>;

class Bitmap{
	DimensionPx d_;
	Data dt_;
public:
	inline Bitmap(const Address a,const DimensionPx&px):d_{px},dt_{a,px.width()*px.height()}{}
	inline auto dim()const->const DimensionPx&{return d_;}
	inline auto data()const->const Data&{return dt_;}
	auto to(const Bitmap&dst,const CoordsPx&c)const{
		char*si=static_cast<char*>(dt_.begin().address());
		char*di=static_cast<char*>(dst.dt_.begin().address());
		di+=c.y()*dst.dim().width()+c.x();
		const SizePx ln=dst.dim().width()-d_.width();
		const SizePx h=d_.height();
		const SizePx w=d_.width();
		for(SizePx y=0;y<h;y++){
			for(SizePx x=0;x<w;x++){
				*di=*si;
				si++;
				di++;
			}
			di+=ln;
		}
	}
};

static int hexpx[]{
	static_cast<int>(0b0'01100'10010'10010'10010'01100'00000'0),
	static_cast<int>(0b0'00100'01100'00100'00100'01110'00000'0),
	static_cast<int>(0b0'01100'10010'00100'01000'11110'00000'0),
	static_cast<int>(0b0'11100'00010'11100'00010'11100'00000'0),
	static_cast<int>(0b0'00010'00110'01010'11110'00010'00000'0),
	static_cast<int>(0b0'11110'10000'11110'00010'11100'00000'0),
	static_cast<int>(0b0'01100'10000'11100'10010'01100'00000'0),
	static_cast<int>(0b0'11110'00010'00100'01000'01000'00000'0),
	static_cast<int>(0b0'01100'10010'01100'10010'01100'00000'0),
	static_cast<int>(0b0'01100'10010'01110'00010'01100'00000'0),
	static_cast<int>(0b0'01100'10010'11110'10010'10010'00000'0),
	static_cast<int>(0b0'11100'10010'11100'10010'11100'00000'0),
	static_cast<int>(0b0'01110'10000'10000'10000'01110'00000'0),
	static_cast<int>(0b0'11100'10010'10010'10010'11100'00000'0),
	static_cast<int>(0b0'11110'10000'11100'10000'11110'00000'0),
	static_cast<int>(0b0'11110'10000'11100'10000'10000'00000'0),
};


using Row=Size;
using Column=Size;
class BitmapHexPrinter{
	char*di_;
	char*dil_;
	const Bitmap&b_;
	const SizePx bwi_;
	const SizePx ch_wi_;
	const SizePx ch_hi_;
	const SizePx ln_;
	char color_fg;
	char color_bg;
	char padding1;
	char padding2;
public:
	BitmapHexPrinter(const Bitmap&b):
		di_{static_cast<char*>(b.data().begin().address())},
		dil_{di_},
		b_{b},
		bwi_{b.dim().width()},
		ch_wi_{5},
		ch_hi_{6},
		ln_{bwi_-ch_wi_},
		color_fg{2},
		color_bg{0},
		padding1{0},
		padding2{0}
	{}
	auto position(Row r,Column c){
		di_=static_cast<char*>(b_.data().begin().address());
		di_+=bwi_*r*ch_hi_+c*ch_wi_;
		dil_=di_;
	}
	auto next_line(){
		di_=dil_+bwi_*ch_hi_;
	}
	auto print_pixels(int fpx){
		for(int y=0;y<ch_hi_;y++){
			for(int x=0;x<ch_wi_;x++){
				fpx<<=1;
				*di_=fpx<0?color_fg:color_bg;
				di_++;
			}
			di_+=ln_;
		}
		di_=di_-bwi_*ch_hi_+ch_wi_;
	}
};

class Vga13h{
	Bitmap b_;

public:
	inline Vga13h():b_{Address(0xa0000),DimensionPx{320,200}}{}
	inline auto bmp()const->const Bitmap&{return b_;}
	auto print(){
		BitmapHexPrinter p{b_};
		p.position(1,10);
		for(int i=0;i<16;i++){
			p.print_pixels(hexpx[i]);
		}
		p.next_line();
		for(int i=0;i<16;i++){
			p.print_pixels(hexpx[i]);
		}
	}
	auto print2(){
		const SizePx wi=b_.dim().width();
		const int chpx_wi=5;
		const int chpx_hi=6;
		const SizePx ln=wi-chpx_wi;
		char*di=static_cast<char*>(b_.data().begin().offset(wi*10+100).address());
		for(int i=0;i<16;i++){
			di=print_chpx(di,hexpx[i],4,0,chpx_wi,chpx_hi,ln,wi);
		}
	}
private:
	auto print_chpx(char*di,int chpx,const char color_fg,const char color_bg,const SizePx ch_width,const SizePx ch_height,const SizePx line_inc,const SizePx scr_wi)->char*{
		for(int y=0;y<ch_height;y++){
			for(int x=0;x<ch_width;x++){
				chpx<<=1;
				*di=chpx<0?color_fg:color_bg;
				di++;
			}
			di+=line_inc;
		}
		di=di-scr_wi*ch_height+ch_width;
		return di;
	}
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
		const char*si=static_cast<const char*>(b_.data().begin().address());
		char*di=static_cast<char*>(dst.data().begin().address());
		PositionPx p{static_cast<CoordPx>(p_.x()),static_cast<CoordPx>(p_.y())};
		di+=p.y()*dst.dim().width()+p.x();
		const SizePx ln=dst.dim().width()-b_.dim().width();
		const SizePx h=b_.dim().height();
		const SizePx w=b_.dim().width();
		for(SizePx y=0;y<h;y++){
			for(SizePx x=0;x<w;x++){
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
