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

static const int BitmapHexPrinter_chars[]{
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
	char*di_; // current pixel in bitmap
	char*dil_; // beginning of current line
	const Bitmap&b_;
	const SizePx bmp_wi_;
	const SizePx font_wi_;
	const SizePx font_hi_;
	const SizePx ln_;
	char color_fg_;
	char color_bg_;
	bool transparent_; // ? implement
	char padding2_;
public:
	inline BitmapHexPrinter(const Bitmap&b):
		di_{static_cast<char*>(b.data().begin().address())},
		dil_{di_},
		b_{b},
		bmp_wi_{b.dim().width()},
		font_wi_{5},
		font_hi_{6},
		ln_{bmp_wi_-font_wi_},
		color_fg_{2},
		color_bg_{0},
		transparent_{false},
		padding2_{0}
	{}
	inline auto pos(Row r,Column c){
		di_=static_cast<char*>(b_.data().begin().address());
		di_+=bmp_wi_*r*font_hi_+c*font_wi_;
		dil_=di_;
	}
	inline auto pos_next_line(){di_=dil_+bmp_wi_*font_hi_;}
	inline auto pos_start_of_line(){di_=dil_;}
	inline auto set_foreground_color(char c){color_fg_=c;}
	inline auto set_background_color(char c){color_bg_=c;}
	auto print_pixels(int fpx){
		for(int y=0;y<font_hi_;y++){
			for(int x=0;x<font_wi_;x++){
				fpx<<=1;
				*di_=fpx<0?color_fg_:color_bg_;
				di_++;
			}
			di_+=ln_;
		}
		di_=di_-bmp_wi_*font_hi_+font_wi_;
	}
	inline auto print_space(){print_pixels(0b0'00000'00000'00000'00000'00000'00000'0);}
	inline auto print_hex_char(int hex_number_4b){print_pixels(BitmapHexPrinter_chars[hex_number_4b&15]);}
	auto print_hex_8b(unsigned char v){
		const int lower=v&0xf;
		const int higher=(v>>4)&0xf;
		print_hex_char(higher);
		print_hex_char(lower);
	}
	auto print_hex_16b(unsigned short v){
		const int ch1=v&0xf;v>>=4;
		const int ch2=v&0xf;v>>=4;
		const int ch3=v&0xf;v>>=4;
		const int ch4=v&0xf;v>>=4;
		print_hex_char(ch4);
		print_hex_char(ch3);
		print_hex_char(ch2);
		print_hex_char(ch1);
	}
	auto print_hex_32b(unsigned int v){
		const int ch1=v&0xf;v>>=4;
		const int ch2=v&0xf;v>>=4;
		const int ch3=v&0xf;v>>=4;
		const int ch4=v&0xf;v>>=4;
		const int ch5=v&0xf;v>>=4;
		const int ch6=v&0xf;v>>=4;
		const int ch7=v&0xf;v>>=4;
		const int ch8=v&0xf;v>>=4;
		print_hex_char(ch8);
		print_hex_char(ch7);
		print_hex_char(ch6);
		print_hex_char(ch5);
		print_hex_char(ch4);
		print_hex_char(ch3);
		print_hex_char(ch2);
		print_hex_char(ch1);
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
