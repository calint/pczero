#pragma once
namespace osca{

using Address=void*;
using Size=int;
using SizeBytes=Size;

inline void pz_memcpy(Address from,Address to,SizeBytes n){
	asm("mov %0,%%esi;"
		"mov %1,%%edi;"
		"mov %2,%%ecx;"
		"rep movsb;"
		:
		:"r"(from),"r"(to),"r"(n)
		:"%esi","%edi","%ecx" // ? clobbers memory?
	);
}

using OffsetBytes=int;

class Pointer{
	Address a_;
public:
	inline Pointer(const Address a):a_{a}{}
	inline auto address()const->Address{return a_;}
	inline auto offset(const OffsetBytes ob)const->Pointer{return Pointer{static_cast<char*>(a_)+ob};}
	inline auto write(const char v){*static_cast<char*>(a_)=v;}
	inline auto write(const unsigned char v){*static_cast<unsigned char*>(a_)=v;}
	inline auto write(const short v){*static_cast<short*>(a_)=v;}
	inline auto write(const unsigned short v){*static_cast<unsigned short*>(a_)=v;}
	inline auto write(const int v){*static_cast<int*>(a_)=v;}
	inline auto write(const unsigned v){*static_cast<unsigned*>(a_)=v;}
	inline auto write(const long v){*static_cast<long*>(a_)=v;}
	inline auto write(const unsigned long v){*static_cast<unsigned long*>(a_)=v;}
};

class Data{
	Pointer p_;
	SizeBytes s_;
public:
	inline Data(const Address a,const SizeBytes n):p_{a},s_{n}{}
	inline auto to(const Data&d)const{pz_memcpy(p_.address(),d.begin().address(),s_);}
	inline auto to(const Data&d,const SizeBytes sb)const{pz_memcpy(p_.address(),d.begin().address(),sb);}
	inline auto size()const->SizeBytes{return s_;}
	inline auto begin()const->Pointer{return p_;}
	inline auto pointer()const->Pointer{return p_;}
};

template<typename T>
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

template<typename T>
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

static const unsigned int table_hex_to_font[]{
		0b01100'10010'10010'10010'01100'00000'00, // 0
		0b00100'01100'00100'00100'01110'00000'00, // 1
		0b01100'10010'00100'01000'11110'00000'00, // 2
		0b11100'00010'11100'00010'11100'00000'00, // 3
		0b00010'00110'01010'11110'00010'00000'00, // 4
		0b11110'10000'11110'00010'11100'00000'00, // 5
		0b01100'10000'11100'10010'01100'00000'00, // 6
		0b11110'00010'00100'01000'01000'00000'00, // 7
		0b01100'10010'01100'10010'01100'00000'00, // 8
		0b01100'10010'01110'00010'01100'00000'00, // 9
		0b01100'10010'11110'10010'10010'00000'00, // A
		0b11100'10010'11100'10010'11100'00000'00, // B
		0b01110'10000'10000'10000'01110'00000'00, // C
		0b11100'10010'10010'10010'11100'00000'00, // D
		0b11110'10000'11100'10000'11110'00000'00, // E
		0b11110'10000'11100'10000'10000'00000'00, // F
};

// from https://stackoverflow.com/questions/61124564/convert-scancodes-to-ascii
static const char table_scancode_to_ascii[256]{
		0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
		'\t', /* <-- Tab */
		'q','w','e','r','t','y','u','i','o','p','[',']','\n',
		0, /* <-- control key */
		'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,
		'*',
		0,  /* Alt */
		' ',  /* Space bar */
		0,  /* Caps lock */
		0,  /* 59 - F1 key ... > */
		0,   0,   0,   0,   0,   0,   0,   0,
		0,  /* < ... F10 */
		0,  /* 69 - Num lock*/
		0,  /* Scroll Lock */
		0,  /* Home key */
		0,  /* Up Arrow */
		0,  /* Page Up */
		'-',
		0,  /* Left Arrow */
		0,
		0,  /* Right Arrow */
		'+',
		0,  /* 79 - End key*/
		0,  /* Down Arrow */
		0,  /* Page Down */
		0,  /* Insert Key */
		0,  /* Delete Key */
		0,   0,   0,
		0,  /* F11 Key */
		0,  /* F12 Key */
		0,  /* All other keys are undefined */
};
// 0e - backspace
// 1c - return

static const unsigned int table_ascii_to_font[]{
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,
		0b00000'00000'00000'00000'00000'00000'00, // 32: space
		0b00100'00100'00100'00000'00100'00000'00, // 33: !
		0b01010'01010'00000'00000'00000'00000'00, // 34: "
		0b01010'11110'01010'11110'01010'00000'00, // 35: #
		0b01110'10100'01100'00110'11100'00000'00, // 36: $
		0b00000'10010'00100'01010'10000'00000'00, // 37: %
		0b00110'01000'00100'10010'01101'00000'00, // 38: &
		0b00100'00100'00000'00000'00000'00000'00, // 39: '
		0b00100'01000'01000'01000'00100'00000'00, // 40: (
		0b01000'00100'00100'00100'01000'00000'00, // 41:
		0b00000'01010'00100'01010'00000'00000'00, // 42: *
		0b00000'00100'01110'00100'00000'00000'00, // 43: +
		0b00000'00000'00000'00000'00100'01000'00, // 44: ,
		0b00000'00000'01110'00000'00000'00000'00, // 45: -
		0b00000'00000'00000'00000'00100'00000'00, // 46: .
		0b00010'00010'00100'01000'01000'00000'00, // 47: /
		0b01100'10010'10110'11010'01100'00000'00, // 48: 0
		0b00100'01100'00100'00100'01110'00000'00, // 49: 1
		0b01100'10010'00100'01000'11110'00000'00, // 50: 2
		0b11100'00010'11100'00010'11100'00000'00, // 51: 3
		0b00010'00110'01010'11110'00010'00000'00, // 52: 4
		0b11110'10000'11110'00010'11100'00000'00, // 53: 5
		0b01100'10000'11100'10010'01100'00000'00, // 54: 6
		0b11110'00010'00100'01000'01000'00000'00, // 55: 7
		0b01100'10010'01100'10010'01100'00000'00, // 56: 8
		0b01100'10010'01110'00010'01100'00000'00, // 57: 9
		0b00000'00100'00000'00100'00000'00000'00, // 58: :
		0b00000'00100'00000'00100'01000'00000'00, // 59: ;
		0b00100'01000'10000'01000'00100'00000'00, // 60: <
		0b00000'01110'00000'01110'00000'00000'00, // 61: =
		0b01000'00100'00010'00100'01000'00000'00, // 62: >
		0b00100'00010'00100'00000'00100'00000'00, // 63: ?
		0b01100'10010'10110'10000'01100'00000'00, // 64: @
		0b01100'10010'11110'10010'10010'00000'00, // 65: A
		0b11100'10010'11100'10010'11100'00000'00, // 66: B
		0b01110'10000'10000'10000'01110'00000'00, // 67: C
		0b11100'10010'10010'10010'11100'00000'00, // 68: D
		0b11110'10000'11100'10000'11110'00000'00, // 69: E
		0b11110'10000'11100'10000'10000'00000'00, // 70: F
		0b01110'10000'10010'10010'01110'00000'00, // 71: G
		0b10010'10010'11110'10010'10010'00000'00, // 72: H
		0b01110'00100'00100'00100'01110'00000'00, // 73: I
		0b01110'00010'00010'00010'01100'00000'00, // 74: J
		0b10010'10010'11100'10010'10010'00000'00, // 75: K
		0b10000'10000'10000'10000'11110'00000'00, // 76: L
		0b10010'11110'11110'10010'10010'00000'00, // 77: M
		0b10010'11010'10110'10010'10010'00000'00, // 78: N
		0b01100'10010'10010'10010'01100'00000'00, // 79: O
		0b11100'10010'11100'10000'10000'00000'00, // 80: P
		0b01100'10010'10010'10110'01110'00000'00, // 81: Q
		0b11100'10010'11100'10010'10010'00000'00, // 82: R
		0b01100'10000'01100'00010'11100'00000'00, // 83: S
		0b11100'01000'01000'01000'01000'00000'00, // 84: T
		0b10010'10010'10010'10010'01100'00000'00, // 85: U
		0b10010'10010'10010'01010'00100'00000'00, // 86: V
		0b10010'10010'11110'11110'10010'00000'00, // 87: W
		0b10010'10010'01100'10010'10010'00000'00, // 88: X
		0b10010'10010'01100'00100'00100'00000'00, // 89: Y
		0b11110'00100'01000'10000'11110'00000'00, // 90: Z
		0b01100'01000'01000'01000'01100'00000'00, // 91: [
		0b01000'01000'00100'00010'00010'00000'00, // 92: backslash
		0b00110'00010'00010'00010'00110'00000'00, // 93: ]
		0b00100'01010'00000'00000'00000'00000'00, // 94: ^
		0b00000'00000'00000'00000'11110'00000'00, // 95: _
		0b00100'00010'00000'00000'00000'00000'00, // 96: `
		0b00000'01100'01110'10010'01110'00000'00, // 97: a
		0b00000'10000'11100'10010'11100'00000'00, // 98: b
		0b00000'01100'10000'10000'01100'00000'00, // 99: c
		0b00000'00010'01110'10010'01110'00000'00, // 100: d
		0b00000'01100'11110'10000'01110'00000'00, // 101: e
		0b00100'01000'11100'01000'01000'00000'00, // 102: f
		0b00000'01100'10010'10010'01110'01110'00, // 103: g
		0b00000'10000'11100'10010'10010'00000'00, // 104: h
		0b00000'00100'00000'00100'00100'00000'00, // 105: i
		0b00000'00100'00000'00100'00100'01000'00, // 106: j
		0b00000'10000'10010'11000'10010'00000'00, // 107: k
		0b00000'01000'01000'01000'00100'00000'00, // 108: l
		0b00000'00000'11100'11110'11110'00000'00, // 109: m
		0b00000'00000'11100'10010'10010'00000'00, // 110: n
		0b00000'00000'01100'10010'01100'00000'00, // 111: o
		0b00000'00000'11100'10010'11100'10000'00, // 112: p
		0b00000'00000'01100'10010'01110'00010'00, // 113: q
		0b00000'00000'01100'10000'10000'00000'00, // 114: r
		0b00000'00110'01100'00010'01100'00000'00, // 115: s
		0b00100'01110'00100'00100'00010'00000'00, // 116: t
		0b00000'00000'10010'10010'01100'00000'00, // 117: u
		0b00000'00000'10010'01010'00100'00000'00, // 118: v
		0b00000'00000'11110'11110'01100'00000'00, // 119: w
		0b00000'00000'10010'01100'10010'00000'00, // 120: x
		0b00000'00000'10010'01010'00100'01000'00, // 121: y
		0b00000'00000'11110'00100'11110'00000'00, // 122: z
		0b01100'01000'11000'01000'01100'00000'00, // 123: {
		0b00100'00100'00100'00100'00100'00000'00, // 123: |
		0b01100'00100'00110'00100'01100'00000'00, // 123: }
		0b00000'10110'01100'00000'00000'00000'00, // 124: ~
		0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,
};

using Row=Size;
using Column=Size;
using Color8b=char;
class PrinterToBitmap{
	char*di_; // current pixel in bitmap
	char*dil_; // beginning of current line
	const Bitmap&b_;
	const SizePx bmp_wi_;
	const SizePx font_wi_;
	const SizePx font_hi_;
	const SizePx ln_;
	Color8b fg_;
	Color8b bg_;
	bool transparent_; // ? implement
	char padding1=0;
public:
	inline PrinterToBitmap(const Bitmap&b):
		di_{static_cast<char*>(b.data().begin().address())},
		dil_{di_},
		b_{b},
		bmp_wi_{b.dim().width()},
		font_wi_{5},
		font_hi_{6},
		ln_{bmp_wi_-font_wi_},
		fg_{2},
		bg_{0},
		transparent_{false}
	{}
	inline auto pos(const Row r,const Column c)->PrinterToBitmap&{
		di_=static_cast<char*>(b_.data().begin().address());
		di_+=bmp_wi_*r*font_hi_+c*font_wi_;
		dil_=di_;
		return*this;
	}
	inline auto nl()->PrinterToBitmap&{di_=dil_+bmp_wi_*(font_hi_+2);dil_=di_;return*this;}
	inline auto cr()->PrinterToBitmap&{di_=dil_;return*this;}
	inline auto fg(const Color8b c)->PrinterToBitmap&{fg_=c;return*this;}
	inline auto bg(const Color8b c)->PrinterToBitmap&{bg_=c;return*this;}
	inline auto transparent(const bool b)->PrinterToBitmap&{transparent_=b;return*this;}
	auto draw(unsigned int bmp_5x6)->PrinterToBitmap&{
		if(transparent_){
			draw_transparent(bmp_5x6);
		}else{
			draw_with_bg(bmp_5x6);
		}
		return*this;
	}
	auto p_hex(const int hex_number_4b)->PrinterToBitmap&{
		draw(table_hex_to_font[hex_number_4b&15]);
		return*this;
	}
	auto p_hex_8b(unsigned char v)->PrinterToBitmap&{
		const int ch1=v&0xf;
		const int ch2=(v>>4)&0xf;
		p_hex(ch2);
		p_hex(ch1);
		return*this;
	}
	auto p_hex_16b(unsigned short v)->PrinterToBitmap&{
		// ? ugly code. remake
		const int ch1=v&0xf;v>>=4;
		const int ch2=v&0xf;v>>=4;
		const int ch3=v&0xf;v>>=4;
		const int ch4=v&0xf;
		p_hex(ch4);
		p_hex(ch3);
		p_hex(ch2);
		p_hex(ch1);
		return*this;
	}
	auto p_hex_32b(unsigned v)->PrinterToBitmap&{
		// ? ugly code. remake
		const int ch1=v&0xf;v>>=4;
		const int ch2=v&0xf;v>>=4;
		const int ch3=v&0xf;v>>=4;
		const int ch4=v&0xf;v>>=4;
		const int ch5=v&0xf;v>>=4;
		const int ch6=v&0xf;v>>=4;
		const int ch7=v&0xf;v>>=4;
		const int ch8=v&0xf;
		p_hex(ch8);
		p_hex(ch7);
		p_hex(ch6);
		p_hex(ch5);
		p_hex(ch4);
		p_hex(ch3);
		p_hex(ch2);
		p_hex(ch1);
		return*this;
	}
	auto p(const char ch)->PrinterToBitmap&{
		draw(table_ascii_to_font[static_cast<int>(ch)]);
		return*this;
	}
	auto p(const char*s)->PrinterToBitmap&{
		while(*s){
			p(*s);
			s++;
		}
		return*this;
	}
	auto backspace()->PrinterToBitmap&{
		di_-=font_wi_;
		p(' ');
		di_-=font_wi_;
		return*this;
	}
private:
	auto draw_with_bg(unsigned int bmp_5x6)->PrinterToBitmap&{ // make inline assembler?
		const unsigned int mask=1u<<31;
		for(int y=0;y<font_hi_;y++){
			for(int x=0;x<font_wi_;x++){
				const bool px=bmp_5x6&mask;
				bmp_5x6<<=1;
				*di_=px?fg_:bg_;
				di_++;
			}
			di_+=ln_;
		}
		di_=di_-bmp_wi_*font_hi_+font_wi_;
		return*this;
	}
	auto draw_transparent(unsigned int bmp_5x6)->PrinterToBitmap&{ // make inline assembler?
		const unsigned int mask=1u<<31;
		for(int y=0;y<font_hi_;y++){
			for(int x=0;x<font_wi_;x++){
				const bool px=bmp_5x6&mask;
				bmp_5x6<<=1;
				if(px){
					*di_=fg_;
				}
				di_++;
			}
			di_+=ln_;
		}
		di_=di_-bmp_wi_*font_hi_+font_wi_;
		return*this;
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
	inline Sprite(const Bitmap&b,const Position&p,const Velocity&v,const Acceleration&a):
		b_{b},p_{p},v_{v},a_{a}
	{}
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

using Radians=float;
using Degrees=float;
inline auto cos(const Radians radians)->float{
	float v;
	asm("fcos"
		:"=t"(v) // "t": first (top of stack) floating point register
		:"0"(radians)
	);
	return v;
}

inline auto sin(const Radians radians)->float{
	float v;
	asm("fsin"
		:"=t"(v) // "t": first (top of stack) floating point register
		:"0"(radians)
	);
	return v;
}

constexpr float PI=3.141592653589793f;
constexpr auto deg_to_rad(const Degrees deg)->Radians{
	constexpr float deg_to_rad=PI/180.f;
	return deg*deg_to_rad;
}

class Vector2D{
public:
	float x=0,y=0;
};

class Matrix2D{
	float	xx=1,yx=0,tx=0,
			xy=0,yy=1,ty=0,
			xu=0,yu=0, i=1;
public:
	inline auto set_identity(){
		xx=1;yx=0;tx=0;
		xy=0,yy=1,ty=0;
		xu=0,yu=0, i=1;
	}
	inline auto set_rotation(const Radians r){
		const float cs=cos(r);
		const float sn=sin(r);
		xx=cs;yx=-sn;tx=0;
		xy=sn,yy= cs,ty=0;
		xu= 0,yu=  0,i=1;
	}
	inline auto set_translation(const Vector2D&v){
		tx=v.x;
		ty=v.y;
	}
	inline auto set_transform(const Radians r,const Vector2D&t){
		set_rotation(r);
		set_translation(t);
	}
	auto transform(const Vector2D src[],Vector2D dst[],const int n)const{
		for(int j=0;j<n;j++){
			dst->x=xx*src->x+yx*src->y+tx;
			dst->y=xy*src->x+yy*src->y+ty;
			src++;
			dst++;
		}
	}
};

} // end namespace osca
