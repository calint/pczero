#pragma once

namespace osca{

using Angle=Real;
using AngleRad=Angle;
using AngleDeg=Angle;
using Scale=Real;
using Scalar=Real;

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

constexpr inline auto deg_to_rad(const AngleDeg deg)->AngleRad{
	constexpr Real deg_to_rad{PI/180};
	return deg*deg_to_rad;
}

class Vector{
public:
	Real x{},y{};
	// normalizes and returns this vector
	inline auto normalize()->Vector&{
		const Real len=sqrt(x*x+y*y);
		x/=len;
		y/=len;
		return*this;
	}
	// scales and returns this vector
	inline constexpr auto scale(Scale s)->Vector&{
		x*=s;
		y*=s;
		return*this;
	}
	// increases and returns this vector by v
	inline constexpr auto inc_by(const Vector&v)->void{
		x+=v.x;
		y+=v.y;
	}
	// increases and returns this vector by v*scl
	inline constexpr auto inc_by(const Vector&v,const Real scl)->void{
		x+=v.x*scl;
		y+=v.y*scl;
	}
	// negates and returns this vector
	inline constexpr auto negate()->Vector&{
		x=-x;
		y=-y;
		return*this;
	}
	// sets and returns this vector to absolute value of itself
	inline auto absolute()->Vector&{
		x=abs(x);
		y=abs(y);
		return*this;
	}
	// returns dot product of this vector and v
	inline constexpr auto dot(const Vector&v)const->Real{
		return x*v.x+y*v.y;
	}
	// returns the normal of this vector
	inline constexpr auto normal()const->Vector{return{-y,x};}
	inline auto magnitude()const->Real{return sqrt(x*x+y*y);}
	inline constexpr auto magnitude2()const->Real{return x*x+y*y;}
//	auto operator<=>(const Vector2D&)const=default; // ? does not compile in clang++ without includes from std
	inline constexpr auto operator==(const Vector&)const->bool=default; // bitwise equality relevant
	inline constexpr auto operator-(const Vector&other)const->Vector{return{x-other.x,y-other.y};}
	inline constexpr auto operator+(const Vector&other)const->Vector{return{x+other.x,y+other.y};}

//	inline static constexpr auto from_to(const Vector&from,const Vector&to)->Vector{
//		return to-from;
//	}
};

using Count=Size;

class Matrix2D{
	Real xx{},xy{},xt{};
	Real yx{},yy{},yt{};
	Real ux{},uy{},id{};
public:
	auto set_transform(const Scale scale,const AngleRad rotation,const Vector&translation)->void{
		Real fcos,fsin;
		sin_and_cos(rotation,fsin,fcos);
		const Real cs=scale*fcos;
		const Real sn=scale*fsin;
		xx=cs;xy=-sn;xt=translation.x;
		yx=sn;yy= cs;yt=translation.y;
		ux= 0;uy=  0;id=1;
	}
	constexpr auto transform(const Vector src[],Vector dst[],const Count n)const->void{
		for(Count i=0;i<n;i++){
			dst->x=xx*src->x+xy*src->y+xt;
			dst->y=yx*src->x+yy*src->y+yt;
			src++;
			dst++;
		}
	}
	// does the rotation part of the transform
	constexpr auto rotate(const Vector src[],Vector dst[],const Count n)const->void{
		for(Count i=0;i<n;i++){
			dst->x=xx*src->x+xy*src->y;
			dst->y=yx*src->x+yy*src->y;
			src++;
			dst++;
		}
	}
	inline constexpr auto axis_x()const->Vector{return{xx,yx};} // math correct?
	inline constexpr auto axis_y()const->Vector{return{xy,yy};} // math correct?
};

} // end namespace osca
