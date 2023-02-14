#pragma once

// built-in functions replacements (used by clang++ -O0)
extern "C" void memcpy(void*to,void*from,unsigned n);
extern "C" void memcpy(void*to,void*from,unsigned n){
	asm("mov %0,%%esi;"
		"mov %1,%%edi;"
		"mov %2,%%ecx;"
		"rep movsb;"
		:
		:"r"(from),"r"(to),"r"(n)
		:"%esi","%edi","%ecx" // ? clobbers memory?
	);
}
extern "C" void*memset(void*s,unsigned char c,unsigned n);
extern "C" void*memset(void*s,unsigned char c,unsigned n){
	asm("mov %0,%%edi;"
		"mov %1,%%al;"
		"mov %2,%%ecx;"
		"rep stosb;"
		:
		:"r"(s),"r"(c),"r"(n)
		:"%edi","%al","%ecx" // ? clobbers memory?
	);
	return s;
}

namespace osca{

using Address=void*;
using Size=int;
using SizeBytes=Size;

inline auto pz_memcpy(Address to,Address from,SizeBytes n)->void{
	asm("mov %0,%%esi;"
		"mov %1,%%edi;"
		"mov %2,%%ecx;"
		"rep movsb;"
		:
		:"r"(from),"r"(to),"r"(n)
		:"%esi","%edi","%ecx" // ? clobbers memory?
	);
}

//inline void pz_memset(Address to,unsigned char v,SizeBytes n){
inline auto pz_memset(Address to,char v,SizeBytes n)->void{
	asm("mov %0,%%edi;"
		"mov %1,%%al;"
		"mov %2,%%ecx;"
		"rep stosb;"
		:
		:"r"(to),"r"(v),"r"(n)
		:"%edi","%al","%ecx" // ? clobbers memory?
	);
}

using OffsetBytes=int;

class Pointer{ // ? can/should be removed
	Address a_;
public:
	inline constexpr Pointer(const Address a):a_{a}{}
	inline constexpr auto address()const->Address{return a_;}
	inline constexpr auto offset(const OffsetBytes ob)const->Pointer{return Pointer{static_cast<char*>(a_)+ob};}
	inline constexpr auto write(const char v)->void{*static_cast<char*>(a_)=v;}
	inline constexpr auto write(const unsigned char v)->void{*static_cast<unsigned char*>(a_)=v;}
	inline constexpr auto write(const short v)->void{*static_cast<short*>(a_)=v;}
	inline constexpr auto write(const unsigned short v)->void{*static_cast<unsigned short*>(a_)=v;}
	inline constexpr auto write(const int v)->void{*static_cast<int*>(a_)=v;}
	inline constexpr auto write(const unsigned v)->void{*static_cast<unsigned*>(a_)=v;}
	inline constexpr auto write(const long v)->void{*static_cast<long*>(a_)=v;}
	inline constexpr auto write(const unsigned long v)->void{*static_cast<unsigned long*>(a_)=v;}
};

class Data{
	Address a_;
	SizeBytes s_;
public:
//	inline constexpr Data():a_{nullptr},s_{0}{}
	inline constexpr Data(const Address a,const SizeBytes n):a_{a},s_{n}{}
	inline constexpr auto address()const->Address{return a_;}
	inline constexpr auto size()const->SizeBytes{return s_;}
	inline constexpr auto pointer()const->Pointer{return{a_};}
	inline auto to(const Data&d)const->void{pz_memcpy(d.address(),a_,s_);} // ? bounds check
	inline auto to(const Data&d,const SizeBytes sb)const->void{pz_memcpy(d.address(),a_,sb);} // ? bounds check
	inline auto clear(char byte=0)const->void{pz_memset(a_,byte,s_);}
	inline constexpr auto limit()const->Address{return static_cast<char*>(a_)+s_;}
};

template<typename T>
class CoordsT{
	T x_;
	T y_;
public:
	inline constexpr CoordsT(const T&x,const T&y):x_{x},y_{y}{}
	inline constexpr auto x()const->const T&{return x_;}
	inline constexpr auto y()const->const T&{return y_;}
	inline constexpr auto set_x(const T&x)->void{x_=x;}
	inline constexpr auto set_y(const T&y)->void{y_=y;}
	inline constexpr auto set(const T&x,const T&y)->void{set_x(x);set_y(y);}
	inline constexpr auto inc_x(const T&dx)->void{x_+=dx;}
	inline constexpr auto inc_y(const T&dy)->void{y_+=dy;}
	inline constexpr auto inc_by(const CoordsT<T>&delta)->void{x_+=delta.x_;y_+=delta.y_;}
};
using Real=float;
using CoordPx=short;
using CoordsPx=CoordsT<CoordPx>;

using Coord=Real;
using Coords=CoordsT<Coord>;

template<typename T>
class DimensionT{
	T w_;
	T h_;
public:
	inline constexpr DimensionT(const T&width,const T&height):w_{width},h_{height}{}
	inline constexpr auto width()const->const T&{return w_;}
	inline constexpr auto height()const->const T&{return h_;}
};

using SizePx=int;
using DimensionPx=DimensionT<SizePx>;
using Color8b=char;

template<typename T>
class Bitmap{
	DimensionPx d_;
	Data dt_;
public:
	constexpr Bitmap(const Address a,const DimensionPx&px):d_{px},dt_{a,px.width()*px.height()*Size(sizeof(T))}{}
	inline constexpr auto dim()const->const DimensionPx&{return d_;}
	inline constexpr auto data()const->const Data&{return dt_;}
	inline constexpr auto pointer_offset(const CoordsPx p)const->Pointer{return dt_.pointer().offset(p.y()*d_.width()*Size(sizeof(T))+p.x()*Size(sizeof(T)));}
	constexpr auto to(const Bitmap&dst,const CoordsPx&c)const->void{
		T*si=static_cast<T*>(dt_.address());
		T*di=static_cast<T*>(dst.dt_.address());
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
	constexpr auto to_transparent(const Bitmap&dst,const CoordsPx&c)const->void{
		T*si=static_cast<T*>(dt_.address());
		T*di=static_cast<T*>(dst.dt_.address());
		di+=c.y()*dst.dim().width()+c.x();
		const SizePx ln=dst.dim().width()-d_.width();
		const SizePx h=d_.height();
		const SizePx w=d_.width();
		for(SizePx y=0;y<h;y++){
			for(SizePx x=0;x<w;x++){
				T v=*si;
				if(v)
					*di=v;
				si++;
				di++;
			}
			di+=ln;
		}
	}
};

using Bitmap8b=Bitmap<Color8b>;

static constexpr unsigned table_hex_to_font[]{
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
static constexpr char table_scancode_to_ascii[256]{
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

static constexpr unsigned table_ascii_to_font[]{
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

using CoordsChar=CoordsT<int>;
class PrinterToBitmap{
	Color8b*di_; // current pixel in bitmap
	Color8b*dil_; // beginning of current line
	Bitmap8b&b_;
	SizePx bmp_wi_;
	SizePx ln_;
	Color8b fg_{2};
	Color8b bg_{0};
	bool transparent_{false};
	const char padding1{0};
	static constexpr SizePx font_wi_{5};
	static constexpr SizePx font_hi_{6};
	static constexpr SizePx line_padding_{2};
public:
	constexpr PrinterToBitmap(Bitmap8b&b):
		di_{static_cast<Color8b*>(b.data().address())},
		dil_{di_},
		b_{b},
		bmp_wi_{b.dim().width()},
		ln_{SizePx(bmp_wi_-font_wi_)}
	{}
	constexpr auto operator=(const PrinterToBitmap&o)->PrinterToBitmap&{
		if(this==&o)
			return*this;
		di_=o.di_;
		dil_=o.dil_;
		b_=o.b_;
		bmp_wi_=o.bmp_wi_;
		ln_=o.ln_;
		fg_=o.fg_;
		bg_=o.bg_;
		transparent_=o.transparent_;
//		padding1=o.padding1;
		return*this;
	}

	constexpr auto pos(const CoordsChar p)->PrinterToBitmap&{
		di_=static_cast<Color8b*>(b_.data().address());
		di_+=bmp_wi_*p.y()*(font_hi_+line_padding_)+p.x()*font_wi_;
		dil_=di_;
		return*this;
	}
	inline constexpr auto nl()->PrinterToBitmap&{di_=dil_+bmp_wi_*(font_hi_+line_padding_);dil_=di_;return*this;}
	inline constexpr auto cr()->PrinterToBitmap&{di_=dil_;return*this;}
	inline constexpr auto fg(const Color8b c)->PrinterToBitmap&{fg_=c;return*this;}
	inline constexpr auto bg(const Color8b c)->PrinterToBitmap&{bg_=c;return*this;}
	inline constexpr auto transparent(const bool b)->PrinterToBitmap&{transparent_=b;return*this;}
	constexpr auto draw(unsigned bmp_5x6)->PrinterToBitmap&{
		if(transparent_){
			draw_transparent(bmp_5x6);
		}else{
			draw_with_bg(bmp_5x6);
		}
		return*this;
	}
	constexpr auto p_hex(const int hex_number_4b)->PrinterToBitmap&{
		draw(table_hex_to_font[hex_number_4b&0xf]); // ? error if &0xf not 0
		return*this;
	}
	constexpr auto p_hex_8b(unsigned char v)->PrinterToBitmap&{
		const int ch1=v&0xf;
		const int ch2=(v>>4)&0xf;
		p_hex(ch2);
		p_hex(ch1);
		return*this;
	}
	constexpr auto p_hex_16b(unsigned short v)->PrinterToBitmap&{
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
	constexpr auto p_hex_32b(unsigned v)->PrinterToBitmap&{
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
		p(':');
		p_hex(ch4);
		p_hex(ch3);
		p_hex(ch2);
		p_hex(ch1);
		return*this;
	}
	constexpr auto p(const char ch)->PrinterToBitmap&{
		draw(table_ascii_to_font[unsigned(ch)]);
		return*this;
	}
	constexpr auto p(const char*s)->PrinterToBitmap&{
		while(*s){
			p(*s);
			s++;
		}
		return*this;
	}
	constexpr auto backspace()->PrinterToBitmap&{
		di_-=font_wi_;
		p(' ');
		di_-=font_wi_;
		return*this;
	}
	auto spc()->PrinterToBitmap&{p(' ');return*this;}
private:
	constexpr auto draw_with_bg(unsigned bmp_5x6)->PrinterToBitmap&{ // make inline assembler?
		constexpr unsigned mask=1u<<31;
		for(SizePx y=0;y<font_hi_;y++){
			for(SizePx x=0;x<font_wi_;x++){
				const bool px=bmp_5x6&mask; // ? !=0
				bmp_5x6<<=1;
				*di_=px?fg_:bg_;
				di_++;
			}
			di_+=ln_;
		}
		di_=di_-bmp_wi_*font_hi_+font_wi_;
		return*this;
	}
	constexpr auto draw_transparent(unsigned bmp_5x6)->PrinterToBitmap&{ // make inline assembler?
		constexpr unsigned mask=1u<<31;
		for(SizePx y=0;y<font_hi_;y++){
			for(SizePx x=0;x<font_wi_;x++){
				const bool px=bmp_5x6&mask; // ? !=0
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
	Bitmap8b b_;
public:
	Vga13h():b_{Address(0xa0000),DimensionPx{320,200}}{}
	inline constexpr auto bmp()->Bitmap8b&{return b_;}
};

// the vga 13h bitmap
extern Vga13h vga13h;
Vga13h vga13h;

class PrinterToVga:public PrinterToBitmap{
public:
	constexpr PrinterToVga():PrinterToBitmap{vga13h.bmp()}{}
};

// debugging to vga13h
extern PrinterToVga out;
PrinterToVga out;
extern PrinterToVga err;
PrinterToVga err;
//
//using PositionPx=CoordsPx;
//using Position=Coords;
//using Velocity=Coords;
//using Acceleration=Coords;
//
//class Sprite{
//	const Bitmap&b_;
//	Position p_;
//	Velocity v_;
//	Acceleration a_;
//public:
//	constexpr Sprite(const Bitmap&b,const Position&p,const Velocity&v,const Acceleration&a):
//		b_{b},p_{p},v_{v},a_{a}
//	{}
//	constexpr auto to(const Bitmap&dst)const{
//		const char*si=static_cast<const char*>(b_.data().address());
//		char*di=static_cast<char*>(dst.data().address());
//		PositionPx p{static_cast<CoordPx>(p_.x()),static_cast<CoordPx>(p_.y())};
//		di+=p.y()*dst.dim().width()+p.x();
//		const SizePx ln=dst.dim().width()-b_.dim().width();
//		const SizePx h=b_.dim().height();
//		const SizePx w=b_.dim().width();
//		for(SizePx y=0;y<h;y++){
//			for(SizePx x=0;x<w;x++){
//				const char px=*si;
//				if(px){
//					*di=px;
//				}
//				si++;
//				di++;
//			}
//			di+=ln;
//		}
//	}
//	inline constexpr auto pos()const->const Position&{return p_;}
//	inline constexpr auto set_pos(const Position&p){p_=p;}
//	inline constexpr auto velocity()const->const Velocity&{return v_;}
//	inline constexpr auto set_velocity(const Velocity&v){v_=v;}
//	inline constexpr auto update(){v_.inc_by(a_);p_.inc_by(v_);}
//};

} // end namespace osca
