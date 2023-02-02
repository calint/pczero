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

static const int table_hex_to_font[]{
		static_cast<int>(0b0'01100'10010'10010'10010'01100'00000'0), // 0
		static_cast<int>(0b0'00100'01100'00100'00100'01110'00000'0), // 1
		static_cast<int>(0b0'01100'10010'00100'01000'11110'00000'0), // 2
		static_cast<int>(0b0'11100'00010'11100'00010'11100'00000'0), // 3
		static_cast<int>(0b0'00010'00110'01010'11110'00010'00000'0), // 4
		static_cast<int>(0b0'11110'10000'11110'00010'11100'00000'0), // 5
		static_cast<int>(0b0'01100'10000'11100'10010'01100'00000'0), // 6
		static_cast<int>(0b0'11110'00010'00100'01000'01000'00000'0), // 7
		static_cast<int>(0b0'01100'10010'01100'10010'01100'00000'0), // 8
		static_cast<int>(0b0'01100'10010'01110'00010'01100'00000'0), // 9
		static_cast<int>(0b0'01100'10010'11110'10010'10010'00000'0), // A
		static_cast<int>(0b0'11100'10010'11100'10010'11100'00000'0), // B
		static_cast<int>(0b0'01110'10000'10000'10000'01110'00000'0), // C
		static_cast<int>(0b0'11100'10010'10010'10010'11100'00000'0), // D
		static_cast<int>(0b0'11110'10000'11100'10000'11110'00000'0), // E
		static_cast<int>(0b0'11110'10000'11100'10000'10000'00000'0), // F
};

// from https://stackoverflow.com/questions/61124564/convert-scancodes-to-ascii
static const char table_scancode_to_ascii[]{
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

static const int table_ascii_to_font[]{
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,
		static_cast<int>(0b0'00000'00000'00000'00000'00000'00000'0), // 32: space
		static_cast<int>(0b0'00100'00100'00100'00000'00100'00000'0), // 33: !
		static_cast<int>(0b0'01010'01010'00000'00000'00000'00000'0), // 34: "
		static_cast<int>(0b0'01010'11110'01010'11110'01010'00000'0), // 35: #
		static_cast<int>(0b0'01110'10100'01100'00110'11100'00000'0), // 36: $
		static_cast<int>(0b0'00000'10010'00100'01010'10000'00000'0), // 37: %
		static_cast<int>(0b0'00110'01000'00100'10010'01101'00000'0), // 38: &
		static_cast<int>(0b0'00100'00100'00000'00000'00000'00000'0), // 39: '
		static_cast<int>(0b0'00100'01000'01000'01000'00100'00000'0), // 40: (
		static_cast<int>(0b0'01000'00100'00100'00100'01000'00000'0), // 41: )
		static_cast<int>(0b0'00000'01010'00100'01010'00000'00000'0), // 42: *
		static_cast<int>(0b0'00000'00100'01110'00100'00000'00000'0), // 43: +
		static_cast<int>(0b0'00000'00000'00000'00000'00100'01000'0), // 44: ,
		static_cast<int>(0b0'00000'00000'01110'00000'00000'00000'0), // 45: -
		static_cast<int>(0b0'00000'00000'00000'00000'00100'00000'0), // 46: .
		static_cast<int>(0b0'00010'00010'00100'01000'01000'00000'0), // 47: /
		static_cast<int>(0b0'01100'10010'11010'10110'01100'00000'0), // 48: 0
		static_cast<int>(0b0'00100'01100'00100'00100'01110'00000'0), // 49: 1
		static_cast<int>(0b0'01100'10010'00100'01000'11110'00000'0), // 50: 2
		static_cast<int>(0b0'11100'00010'11100'00010'11100'00000'0), // 51: 3
		static_cast<int>(0b0'00010'00110'01010'11110'00010'00000'0), // 52: 4
		static_cast<int>(0b0'11110'10000'11110'00010'11100'00000'0), // 53: 5
		static_cast<int>(0b0'01100'10000'11100'10010'01100'00000'0), // 54: 6
		static_cast<int>(0b0'11110'00010'00100'01000'01000'00000'0), // 55: 7
		static_cast<int>(0b0'01100'10010'01100'10010'01100'00000'0), // 56: 8
		static_cast<int>(0b0'01100'10010'01110'00010'01100'00000'0), // 57: 9
		static_cast<int>(0b0'00000'00100'00000'00100'00000'00000'0), // 58: :
		static_cast<int>(0b0'00000'00100'00000'00100'01000'00000'0), // 59: ;
		static_cast<int>(0b0'00100'01000'10000'01000'00100'00000'0), // 60: <
		static_cast<int>(0b0'00000'01110'00000'01110'00000'00000'0), // 61: =
		static_cast<int>(0b0'01000'00100'00010'00100'01000'00000'0), // 62: >
		static_cast<int>(0b0'00100'00010'00100'00000'00100'00000'0), // 63: ?
		static_cast<int>(0b0'01100'10010'10110'10000'01100'00000'0), // 64: @
		static_cast<int>(0b0'01100'10010'11110'10010'10010'00000'0), // 65: A
		static_cast<int>(0b0'11100'10010'11100'10010'11100'00000'0), // 66: B
		static_cast<int>(0b0'01110'10000'10000'10000'01110'00000'0), // 67: C
		static_cast<int>(0b0'11100'10010'10010'10010'11100'00000'0), // 68: D
		static_cast<int>(0b0'11110'10000'11100'10000'11110'00000'0), // 69: E
		static_cast<int>(0b0'11110'10000'11100'10000'10000'00000'0), // 70: F
		static_cast<int>(0b0'01110'10000'10010'10010'01110'00000'0), // 71: G
		static_cast<int>(0b0'10010'10010'11110'10010'10010'00000'0), // 72: H
		static_cast<int>(0b0'01110'00100'00100'00100'01110'00000'0), // 73: I
		static_cast<int>(0b0'01110'00010'00010'00010'01100'00000'0), // 74: J
		static_cast<int>(0b0'10010'10010'11100'10010'10010'00000'0), // 75: K
		static_cast<int>(0b0'10000'10000'10000'10000'11110'00000'0), // 76: L
		static_cast<int>(0b0'10010'11110'11110'10010'10010'00000'0), // 77: M
		static_cast<int>(0b0'10010'11010'10110'10010'10010'00000'0), // 78: N
		static_cast<int>(0b0'01100'10010'10010'10010'01100'00000'0), // 79: O
		static_cast<int>(0b0'11100'10010'11100'10000'10000'00000'0), // 80: P
		static_cast<int>(0b0'01100'10010'10010'10110'01110'00000'0), // 81: Q
		static_cast<int>(0b0'11100'10010'11100'10010'10010'00000'0), // 82: R
		static_cast<int>(0b0'01100'10000'01100'00010'11100'00000'0), // 83: S
		static_cast<int>(0b0'11100'01000'01000'01000'01000'00000'0), // 84: T
		static_cast<int>(0b0'10010'10010'10010'10010'01100'00000'0), // 85: U
		static_cast<int>(0b0'10010'10010'10010'01010'00100'00000'0), // 86: V
		static_cast<int>(0b0'10010'10010'11110'11110'10010'00000'0), // 87: W
		static_cast<int>(0b0'10010'10010'01100'10010'10010'00000'0), // 88: X
		static_cast<int>(0b0'10010'10010'01100'01000'01000'00000'0), // 89: Y
		static_cast<int>(0b0'11110'00100'01000'10000'11110'00000'0), // 90: Z
		static_cast<int>(0b0'01100'01000'01000'01000'01100'00000'0), // 91: [
		static_cast<int>(0b0'01000'01000'00100'00010'00010'00000'0), // 92: backslash
		static_cast<int>(0b0'00110'00010'00010'00010'00110'00000'0), // 93: ]
		static_cast<int>(0b0'00100'01010'00000'00000'00000'00000'0), // 94: ^
		static_cast<int>(0b0'00000'00000'00000'00000'11110'00000'0), // 95: _
		static_cast<int>(0b0'00100'00010'00000'00000'00000'00000'0), // 96: `
		static_cast<int>(0b0'01100'10010'11110'10010'10010'00000'0), // 97: a
		static_cast<int>(0b0'11100'10010'11100'10010'11100'00000'0), // 98: b
		static_cast<int>(0b0'01110'10000'10000'10000'01110'00000'0), // 99: c
		static_cast<int>(0b0'11100'10010'10010'10010'11100'00000'0), // 100: d
		static_cast<int>(0b0'11110'10000'11100'10000'11110'00000'0), // 101: e
		static_cast<int>(0b0'11110'10000'11100'10000'10000'00000'0), // 102: f
		static_cast<int>(0b0'01110'10000'10010'10010'01110'00000'0), // 103: g
		static_cast<int>(0b0'10010'10010'11110'10010'10010'00000'0), // 104: h
		static_cast<int>(0b0'01110'00100'00100'00100'01110'00000'0), // 105: i
		static_cast<int>(0b0'01110'00010'00010'00010'01100'00000'0), // 106: j
		static_cast<int>(0b0'10010'10010'11100'10010'10010'00000'0), // 107: k
		static_cast<int>(0b0'10000'10000'10000'10000'11110'00000'0), // 108: l
		static_cast<int>(0b0'10010'11110'11110'10010'10010'00000'0), // 109: m
		static_cast<int>(0b0'10010'11010'10110'10010'10010'00000'0), // 110: n
		static_cast<int>(0b0'01100'10010'10010'10010'01100'00000'0), // 111: o
		static_cast<int>(0b0'11100'10010'11100'10000'10000'00000'0), // 112: p
		static_cast<int>(0b0'01100'10010'10010'10110'01110'00000'0), // 113: q
		static_cast<int>(0b0'11100'10010'11100'10010'10010'00000'0), // 114: r
		static_cast<int>(0b0'01100'10000'01100'00010'11100'00000'0), // 115: s
		static_cast<int>(0b0'11100'01000'01000'01000'01000'00000'0), // 116: t
		static_cast<int>(0b0'10010'10010'10010'10010'01100'00000'0), // 117: u
		static_cast<int>(0b0'10010'10010'10010'01010'00100'00000'0), // 118: v
		static_cast<int>(0b0'10010'10010'11110'11110'10010'00000'0), // 119: w
		static_cast<int>(0b0'10010'10010'01100'10010'10010'00000'0), // 120: x
		static_cast<int>(0b0'10010'10010'01100'01000'01000'00000'0), // 121: y
		static_cast<int>(0b0'11110'00100'01000'10000'11110'00000'0), // 122: z
		static_cast<int>(0b0'01100'01000'11000'01000'01100'00000'0), // 123: {
		static_cast<int>(0b0'00100'00100'00100'00100'00100'00000'0), // 123: |
		static_cast<int>(0b0'01100'00100'00110'00100'01100'00000'0), // 123: }
		static_cast<int>(0b0'00000'10110'01100'00000'00000'00000'0), // 124: ~
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
	Color8b color_fg_;
	Color8b color_bg_;
	bool transparent_; // ? implement
	char padding2_;
public:
	inline PrinterToBitmap(const Bitmap&b):
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
	inline auto pos(Row r,Column c)->PrinterToBitmap&{
		di_=static_cast<char*>(b_.data().begin().address());
		di_+=bmp_wi_*r*font_hi_+c*font_wi_;
		dil_=di_;
		return*this;
	}
	inline auto pos_next_line()->PrinterToBitmap&{di_=dil_+bmp_wi_*font_hi_;return*this;}
	inline auto pos_start_of_line()->PrinterToBitmap&{di_=dil_;return*this;}
	inline auto foreground(Color8b c)->PrinterToBitmap&{color_fg_=c;return*this;}
	inline auto background(Color8b c)->PrinterToBitmap&{color_bg_=c;return*this;}
	auto print_pixels(int fpx)->PrinterToBitmap&{
		for(int y=0;y<font_hi_;y++){
			for(int x=0;x<font_wi_;x++){
				fpx<<=1;
				*di_=fpx<0?color_fg_:color_bg_;
				di_++;
			}
			di_+=ln_;
		}
		di_=di_-bmp_wi_*font_hi_+font_wi_;
		return*this;
	}
//	auto print_space()->BitmapHexPrinter&{print_char(' ');return*this;}
	auto print_hex_char(int hex_number_4b)->PrinterToBitmap&{print_pixels(table_hex_to_font[hex_number_4b&15]);return*this;}
	auto print_hex_8b(unsigned char v)->PrinterToBitmap&{
		const int ch1=v&0xf;
		const int ch2=(v>>4)&0xf;
		print_hex_char(ch2);
		print_hex_char(ch1);
		return*this;
	}
	auto print_hex_16b(unsigned short v)->PrinterToBitmap&{
		const int ch1=v&0xf;v>>=4;
		const int ch2=v&0xf;v>>=4;
		const int ch3=v&0xf;v>>=4;
		const int ch4=v&0xf;v>>=4;
		print_hex_char(ch4);
		print_hex_char(ch3);
		print_hex_char(ch2);
		print_hex_char(ch1);
		return*this;
	}
	auto print_hex_32b(unsigned int v)->PrinterToBitmap&{
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
		return*this;
	}
	auto print_char(char ch)->PrinterToBitmap&{
		int pixels=table_ascii_to_font[static_cast<unsigned int>(ch)];
		print_pixels(pixels);
		return*this;
	}
	auto print_string(const char*s)->PrinterToBitmap&{
		while(*s){
			print_char(*s);
			s++;
		}
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
