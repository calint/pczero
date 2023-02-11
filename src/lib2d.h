#pragma once

namespace osca{

using Angle=Real;
using AngleRad=Angle;
using AngleDeg=Angle;
using Scale=Real;
using Scalar=Real;

inline auto sin(const AngleRad radians)->float{
	float v;
	asm("fsin"
		:"=t"(v) // "t": first (top of stack) floating point register
		:"0"(radians)
	);
	return v;
}

inline auto cos(const AngleRad radians)->float{
	float v;
	asm("fcos"
		:"=t"(v) // "t": first (top of stack) floating point register
		:"0"(radians)
	);
	return v;
}

// puts sin and cos value of 'radians' in 'fsin' and 'fcos'
inline auto sin_and_cos(const AngleRad radians,float&fsin,float&fcos){
	asm("fsincos"
		:"=t"(fcos),"=u"(fsin) // "u" : Second floating point register
		:"0"(radians)
	);
}

inline auto sqrt(const float s)->float{
	float v;
	asm("fsqrt"
		:"=t"(v) // "t": first (top of stack) floating point register
		:"0"(s)
	);
	return v;
}

constexpr Real PI=3.141592653589793f;

constexpr auto deg_to_rad(const AngleDeg deg)->AngleRad{
	constexpr Real deg_to_rad=PI/180.f;
	return deg*deg_to_rad;
}

class Vector2D{
public:
	Real x=0,y=0;
	// normalizes this vector
	inline auto normalize()->Vector2D&{
		const Coord len=sqrt(x*x+y*y);
		x/=len;
		y/=len;
		return*this;
	}
	// scales this vector
	inline constexpr auto scale(Scale s)->Vector2D&{
		x*=s;
		y*=s;
		return*this;
	}
	inline constexpr auto inc_by(const Vector2D&v){
		x+=v.x;
		y+=v.y;
	}
	inline constexpr auto inc_by(const Vector2D&v,const float dt_s){
		x+=v.x*dt_s;
		y+=v.y*dt_s;
	}
	// negates this vector
	inline constexpr auto negate()->Vector2D&{
		x=-x;
		y=-y;
		return*this;
	}
	inline constexpr auto dot(const Vector2D&v)const->Real{
		return x*v.x+y*v.y;
	}
//	auto operator<=>(const Vector2D&)const=default; // ? does not compile in clang++ without includes from std
	constexpr inline auto operator==(const Vector2D&)const->bool=default;
	constexpr inline auto operator-(const Vector2D&other)const->Vector2D{return{x-other.x,y-other.y};}
	constexpr inline auto operator+(const Vector2D&other)const->Vector2D{return{x+other.x,y+other.y};}
};

using Count=Size;

class Matrix2D{
	Real xx=1,xy=0,xt=0;
	Real yx=0,yy=1,yt=0;
	Real ux=0,uy=0,id=1;
public:
	auto set_transform(const Scale scale,const AngleRad rotation,const Vector2D&translation){
		float fcos,fsin;
		sin_and_cos(rotation,fsin,fcos);
		const Real cs=scale*fcos;
		const Real sn=scale*fsin;
		xx=cs;xy=-sn;xt=translation.x;
		yx=sn;yy= cs;yt=translation.y;
		ux= 0;uy=  0;id=1;
	}
	constexpr auto transform(const Vector2D src[],Vector2D dst[],const Count n)const{
		for(Count i=0;i<n;i++){
			dst->x=xx*src->x+xy*src->y+xt;
			dst->y=yx*src->x+yy*src->y+yt;
			src++;
			dst++;
		}
	}
	// does the rotation part of the transform
	constexpr auto rotate(const Vector2D src[],Vector2D dst[],const Count n)const{
		for(Count i=0;i<n;i++){
			dst->x=xx*src->x+xy*src->y;
			dst->y=yx*src->x+yy*src->y;
			src++;
			dst++;
		}
	}
	inline constexpr auto axis_x()const->Vector2D{return{xx,yx};} // math correct?
	inline constexpr auto axis_y()const->Vector2D{return{xy,yy};} // math correct?
};

} // end namespace osca
