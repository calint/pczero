#pragma once

// built-in functions replacements (used by clang++ -O0 and -Os)
extern "C" void*memcpy(void*dest,const void*src,unsigned n);
extern "C" void*memcpy(void*dest,const void*src,unsigned n){
	char*d=static_cast<char*>(dest);
	const char*s=static_cast<const char*>(src);
	while(n--)
		*d++=*s++;
	return dest;
}
extern "C" void*memset(void*str,int c,unsigned n);
extern "C" void*memset(void*str,int c,unsigned n){
	unsigned char ch=static_cast<unsigned char>(c);
	unsigned char*d=static_cast<unsigned char*>(str);
	while(n--)
		*d++=ch;
	return str;
}
//extern "C" void*memcpy(void*to,void*from,unsigned n);
//extern "C" void*memcpy(void*to,void*from,unsigned n){
//	// ? optimize movsb a times,movsd b times,movsb c times
//	asm("mov %0,%%esi;"
//		"mov %1,%%edi;"
//		"mov %2,%%ecx;"
//		"rep movsb;"
//		:
//		:"r"(from),"r"(to),"r"(n)
//		:"%esi","%edi","%ecx","memory" // ? clobbers memory?
//	);
//	return to;
//}
//extern "C" void*memset(void*to,unsigned c,unsigned n);
//extern "C" void*memset(void*to,unsigned c,unsigned n){
//	unsigned char ch=static_cast<unsigned char>(c);
//	// ? optimize stosb a times,stosd b times,stosb c times
//	asm("mov %0,%%edi;"
//		"mov %1,%%al;"
//		"mov %2,%%ecx;"
//		"rep stosb;"
//		:
//		:"r"(to),"r"(ch),"r"(n)
//		:"%edi","%al","%ecx","memory" // ? clobbers memory?
//	);
//	return to;
//}

namespace osca{

using Address=void*;
using Size=int;
using SizeBytes=Size;

//inline auto pz_memcpy(Address to,Address from,SizeBytes n)->void{
//	// ? optimize movsb a times,movsd b times,movsb c times
//	asm("mov %0,%%esi;"
//		"mov %1,%%edi;"
//		"mov %2,%%ecx;"
//		"rep movsb;"
//		:
//		:"r"(from),"r"(to),"r"(n)
//		:"%esi","%edi","%ecx","memory" // ? clobbers memory?
//	);
//}
//
//inline auto pz_memset(Address to,char v,SizeBytes n)->void{
//	// ? optimize stosb a times,stosd b times,stosb c times
//	asm("mov %0,%%edi;"
//		"mov %1,%%al;"
//		"mov %2,%%ecx;"
//		"rep stosb;"
//		:
//		:"r"(to),"r"(v),"r"(n)
//		:"%edi","%al","%ecx","memory" // ? clobbers memory?
//	);
//}

auto pz_memcpy(Address to,Address from,SizeBytes n)->void;
auto pz_memcpy(Address to,Address from,SizeBytes n)->void{
	char*d=static_cast<char*>(to);
	char*s=static_cast<char*>(from);
	while(n--)
		*d++=*s++;
}

auto pz_memset(Address to,char v,SizeBytes n)->void;
auto pz_memset(Address to,char v,SizeBytes n)->void{
	char*d=static_cast<char*>(to);
	while(n--)
		*d++=v;
}

using OffsetBytes=int;
class Data{ // ? bounds check on constexpr
	Address a_;
	SizeBytes s_;
public:
	inline constexpr Data(const Address a,const SizeBytes n):a_{a},s_{n}{}
	inline constexpr auto address()const->Address{return a_;}
	inline constexpr auto size()const->SizeBytes{return s_;}
	inline constexpr auto address_offset(const OffsetBytes ob)const->Address{return static_cast<Address>(static_cast<char*>(a_)+ob);}
	inline auto to(const Data&d)const->void{pz_memcpy(d.address(),a_,s_);}
	inline auto to(const Data&d,const SizeBytes sb)const->void{pz_memcpy(d.address(),a_,sb);}
	inline auto clear(char byte=0)const->void{pz_memset(a_,byte,s_);}
	inline constexpr auto end()const->Address{return static_cast<char*>(a_)+s_;}
};

using Real=float;
using Angle=Real;
using AngleRad=Angle;

inline auto sin(const AngleRad radians)->Real{
	Real v;
	asm("fsin"
		:"=t"(v) // "t": first (top of stack) floating point register
		:"0"(radians)
	);
	return v;
}

inline auto cos(const AngleRad radians)->Real{
	Real v;
	asm("fcos"
		:"=t"(v) // "t": first (top of stack) floating point register
		:"0"(radians)
	);
	return v;
}

// puts sin and cos value of 'radians' in 'fsin' and 'fcos'
inline auto sin_and_cos(const AngleRad radians,Real&fsin,Real&fcos)->void{
	asm("fsincos"
		:"=t"(fcos),"=u"(fsin) // "u" : Second floating point register
		:"0"(radians)
	);
}

inline auto sqrt(const Real in)->Real{
	Real v;
	asm("fsqrt"
		:"=t"(v) // "t": first (top of stack) floating point register
		:"0"(in)
	);
	return v;
}

inline auto abs(const Real in)->Real{
	Real v;
	asm("fabs"
		:"=t"(v) // "t": first (top of stack) floating point register
		:"0"(in)
	);
	return v;
}

constexpr Real PI=Real(3.141592653589793);

using AngleDeg=Angle;

constexpr inline auto deg_to_rad(const AngleDeg deg)->AngleRad{
	constexpr Real deg_to_rad{PI/180};
	return deg*deg_to_rad;
}

using Scale=Real;

template<typename T>
struct VectorT{
	T x{},y{};
	// normalizes and returns this vector
	inline auto normalize()->VectorT&{const Real len=sqrt(x*x+y*y);x/=len;y/=len;return*this;}
	// scales and returns this vector
	inline constexpr auto scale(const Scale s)->VectorT&{x*=s;y*=s;return*this;}
	// increases and returns this vector by v
	inline constexpr auto inc_by(const VectorT&v)->VectorT{x+=v.x;y+=v.y;return*this;}
	// increases and returns this vector by v*scl
	inline constexpr auto inc_by(const VectorT&v,const Scale s)->void{x+=v.x*s;y+=v.y*s;}
	// negates and returns this vector
	inline constexpr auto negate()->VectorT&{x=-x;y=-y;return*this;}
	// sets and returns this vector to absolute value of itself
	inline auto absolute()->VectorT&{x=abs(x);y=abs(y);return*this;}
	// returns dot product of this vector and 'v'
	inline constexpr auto dot(const VectorT&v)const->T{return x*v.x+y*v.y;}
	// returns the normal of this vector
	inline constexpr auto normal()const->VectorT{return{-y,x};}
	inline auto magnitude()const->T{return sqrt(x*x+y*y);}
	// magnitude squared
	inline constexpr auto magnitude2()const->T{return x*x+y*y;}
	// inline constexpr auto operator<=>(const VectorT&)const=default; // ? does not compile in clang++ without includes from std
	inline constexpr auto operator==(const VectorT&)const->bool=default; // bitwise equality relevant
	inline constexpr auto operator-(const VectorT&other)const->VectorT{return{x-other.x,y-other.y};}
	inline constexpr auto operator+(const VectorT&other)const->VectorT{return{x+other.x,y+other.y};}
};

using Coord=Real; // a coordinate in real space
using Vector=VectorT<Coord>; // a vector in real space
using Point=Vector; // a point in 2D
using PointIx=short; // index into a list of points
using CoordPx=short; // a coordinate in pixel space
using PointPx=VectorT<CoordPx>; // a point in pixel space
using Count=Size;
using Position=Point;

template<typename T>
class DimensionT{
	T w_;
	T h_;
public:
	inline constexpr DimensionT(const T width,const T height):w_{width},h_{height}{}
	inline constexpr auto width()const->const T{return w_;}
	inline constexpr auto height()const->const T{return h_;}
};

using SizePx=int;
using DimensionPx=DimensionT<SizePx>;
using Color8b=unsigned char;

namespace enable{
	constexpr static bool draw_polygons_fill{false};
	constexpr static bool draw_polygons_edges{true};
}

template<typename T>
class Bitmap{ // ? bounds check on constexpr
	DimensionPx d_;
	Data dt_;
public:
	constexpr Bitmap(const Address a,const DimensionPx&px):d_{px},dt_{a,px.width()*px.height()*Size(sizeof(T))}{}
	inline constexpr auto dim()const->const DimensionPx&{return d_;}
	inline constexpr auto data()const->const Data&{return dt_;}
	inline constexpr auto address_offset(const PointPx p)const->Address{
		return dt_.address_offset(p.y*d_.width()*Size(sizeof(T))+p.x*Size(sizeof(T)));
	}
	constexpr auto to(const Bitmap&dst,const PointPx&c)const->void{
		T*si=static_cast<T*>(dt_.address());
		T*di=static_cast<T*>(dst.dt_.address());
		di+=c.y*dst.dim().width()+c.x;
		const SizePx ln=dst.dim().width()-d_.width();
		const SizePx h=d_.height();
		const SizePx w=d_.width();
		for(SizePx y=0;y<h;y++){
			for(SizePx x=0;x<w;x++){ // ? pz_memcpy
				*di=*si;
				si++;
				di++;
			}
			di+=ln;
		}
	}
	constexpr auto to_transparent(const Bitmap&dst,const PointPx&c)const->void{
		T*si=static_cast<T*>(dt_.address());
		T*di=static_cast<T*>(dst.dt_.address());
		di+=c.y*dst.dim().width()+c.x;
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
	constexpr auto draw_dot(const Point&p,const T value)->void{
		const CoordPx xi=CoordPx(p.x);
		const CoordPx yi=CoordPx(p.y);
		if(xi<0||xi>d_.width())
			return;
		if(yi<0||yi>d_.height())
			return;
		*static_cast<T*>(address_offset({xi,yi}))=value;
	}
	constexpr auto draw_bounding_circle(const Point&p,const Scale r)->void{
		const Count segments=Count(5*r);
		AngleRad th=0;
		AngleRad dth=2*PI/AngleRad(segments);
		for(Count i=0;i<segments;i++){
			Real fsin=0;
			Real fcos=0;
			sin_and_cos(th,fsin,fcos);
			const Coord x=p.x+r*fcos;
			const Coord y=p.y+r*fsin;
			draw_dot({x,y},1);
			th+=dth;
		}
	}
	constexpr auto draw_polygon(const Point pts[],const PointIx npoly_ixs,const PointIx ix[],const T color)->void{
		if(npoly_ixs<2){ // ? what if 0, 2 is a line
			draw_dot(pts[0],color);
			return;
		}
		PointIx topy_ix=0;
		const Point&first_point=pts[ix[0]];
		Coord topx=first_point.x;
		Coord topy=first_point.y;
		// find top
		PointIx i=1;
		while(i<npoly_ixs){
			const Point&p=pts[ix[i]]; // ? use pointer
			const Coord y=p.y;
			if(y<topy){
				topy=y;
				topx=p.x;
				topy_ix=i;
			}
			i++;
		}
		PointIx ix_lft,ix_rht;
		ix_lft=ix_rht=topy_ix;
		Coord x_lft,x_rht;
		x_lft=x_rht=topx;
		bool adv_lft=true,adv_rht=true;
		Coord dxdy_lft,dxdy_rht;
		dxdy_lft=dxdy_rht=0;
		Coord x_nxt_lft=0;
		Coord y_nxt_lft=topy;
		Coord x_nxt_rht=0;
		Coord y_nxt_rht=topy;
		Coord dy_rht=0;
		Coord dy_lft=0;
		Coord y=topy;
		const CoordPx wi=CoordPx(d_.width());
		const CoordPx y_scr=CoordPx(y);
		T*pline=static_cast<T*>(dt_.address())+y_scr*wi;
		const PointIx last_elem_ix=npoly_ixs-1;
		while(true){
			if(adv_lft){
				if(ix_lft==last_elem_ix){
					ix_lft=0;
				}else{
					ix_lft++;
				}
				x_nxt_lft=pts[ix[ix_lft]].x;
				y_nxt_lft=pts[ix[ix_lft]].y; // ? whatif prevy==nxty
				dy_lft=y_nxt_lft-y;
				if(dy_lft!=0){
					dxdy_lft=(x_nxt_lft-x_lft)/dy_lft;
				}else{
					dxdy_lft=x_nxt_lft-x_lft;
				}
			}
			if(adv_rht){
				if(ix_rht==0){
					ix_rht=last_elem_ix;
				}else{
					ix_rht--;
				}
				x_nxt_rht=pts[ix[ix_rht]].x;
				y_nxt_rht=pts[ix[ix_rht]].y;
				dy_rht=y_nxt_rht-y;
				if(dy_rht!=0){
					dxdy_rht=(x_nxt_rht-x_rht)/dy_rht;
				}else{
					dxdy_rht=x_nxt_rht-x_rht;
				}
			}
			CoordPx scan_lines_until_next_turn=0;
			const CoordPx yscr=CoordPx(y);
			if(y_nxt_lft>y_nxt_rht){
//				scan_lines_until_next_turn=static_cast<CoordPx>(y_nxt_rht-y);
				scan_lines_until_next_turn=CoordPx(y_nxt_rht)-yscr;
				adv_lft=false;
				adv_rht=true;
			}else{
//				scan_lines_until_next_turn=static_cast<CoordPx>(y_nxt_lft-y); // this generates more artifacts
				scan_lines_until_next_turn=CoordPx(y_nxt_lft)-yscr;
				adv_lft=true;
				adv_rht=false;
			}
			while(true){
				if(scan_lines_until_next_turn<=0)
					break;
				T*p_lft=pline+CoordPx(x_lft);
				const T*p_rht=pline+CoordPx(x_rht);
				if(p_lft>p_rht) // ? can happen?
					break;
				scan_lines_until_next_turn--;
				const CoordPx npx=CoordPx(p_rht-p_lft);
				if(enable::draw_polygons_fill){
					CoordPx n=npx; // ? npx+1?
					T*p=p_lft;
					while(n--){
						*p++=color;
					}
				}
				if(enable::draw_polygons_edges){
					*p_lft=color;
					*(p_lft+npx)=color;
				}
				pline+=wi;
				x_lft+=dxdy_lft;
				x_rht+=dxdy_rht;
			}
			if(ix_lft==ix_rht) // ? render dot or line?
				break;
			if(adv_lft){
				x_lft=x_nxt_lft;
				y=y_nxt_lft;
			}
			if(adv_rht){
				x_rht=x_nxt_rht;
				y=y_nxt_rht;
			}
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
		0b00000'00000'00110'01000'01000'00000'00, // 114: r
		0b00000'00110'01100'00010'01100'00000'00, // 115: s
		0b01000'11100'01000'01000'00100'00000'00, // 116: t
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

using CoordsChar=VectorT<int>;
class PrinterToBitmap{ // ? bounds check on constexpr
	Color8b*di_; // current pixel in bitmap
	Color8b*dil_; // beginning of current line
	Bitmap8b*b_;
	SizePx bmp_wi_;
	SizePx ln_;
	Color8b fg_{2};
	Color8b bg_{0};
	bool transparent_{false};
	char padding1{0};
	static constexpr SizePx font_wi_{5};
	static constexpr SizePx font_hi_{6};
	static constexpr SizePx line_padding_{2}; // ? attribute
public:
	constexpr explicit PrinterToBitmap(Bitmap8b*b):
		di_{static_cast<Color8b*>(b->data().address())},
		dil_{di_},
		b_{b},
		bmp_wi_{b->dim().width()},
		ln_{SizePx(bmp_wi_-font_wi_)}
	{}
	constexpr auto pos(const CoordsChar p)->PrinterToBitmap&{
		di_=static_cast<Color8b*>(b_->data().address());
		di_+=bmp_wi_*p.y*(font_hi_+line_padding_)+p.x*font_wi_;
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
		draw(table_hex_to_font[hex_number_4b&0xf]); // ? error if not 0..15
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
	Bitmap8b b_;
public:
	Vga13h():b_{Address(0xa'0000),DimensionPx{320,200}}{}
	inline constexpr auto bmp()->Bitmap8b&{return b_;}
};

// the vga 13h bitmap
extern Vga13h vga13h;
Vga13h vga13h; // global initialized by osca_init

class PrinterToVga:public PrinterToBitmap{
public:
	constexpr PrinterToVga():PrinterToBitmap{&vga13h.bmp()}{}
};

// debugging to vga13h
extern PrinterToVga out;
PrinterToVga out; // global initialized by osca_init
extern PrinterToVga err;
PrinterToVga err; // global initialized by osca_init
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
template<typename T>
class MatrixT{
	T xx{0},xy{0},xt{0};
	T yx{0},yy{0},yt{0};
	T ux{0},uy{0},id{0};
public:
	auto set_transform(const Scale scale,const AngleRad rotation,const VectorT<T>&translation)->void{
		T fcos,fsin;
		sin_and_cos(rotation,fsin,fcos);
		const T cs=scale*fcos;
		const T sn=scale*fsin;
		xx=cs;xy=-sn;xt=translation.x;
		yx=sn;yy= cs;yt=translation.y;
		ux= 0;uy=  0;id=1;
	}
	constexpr auto transform(const VectorT<T>src[],VectorT<T>dst[],const Count n)const->void{
		for(Count i=0;i<n;i++){
			dst->x=xx*src->x+xy*src->y+xt;
			dst->y=yx*src->x+yy*src->y+yt;
			src++;
			dst++;
		}
	}
	// does the rotation part of the transform
	constexpr auto rotate(const VectorT<T>src[],VectorT<T>dst[],const Count n)const->void{
		for(Count i=0;i<n;i++){
			dst->x=xx*src->x+xy*src->y;
			dst->y=yx*src->x+yy*src->y;
			src++;
			dst++;
		}
	}
	inline constexpr auto axis_x()const->VectorT<T>{return{xx,yx};} // math correct?
	inline constexpr auto axis_y()const->VectorT<T>{return{xy,yy};} // math correct?
};
using Matrix=MatrixT<Coord>;
} // end namespace osca
