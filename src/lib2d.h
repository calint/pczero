#pragma once

namespace osca{

using Angle=float;
using Degrees=float;

inline auto sin(const Angle radians)->float{
	float v;
	asm("fsin"
		:"=t"(v) // "t": first (top of stack) floating point register
		:"0"(radians)
	);
	return v;
}

inline auto cos(const Angle radians)->float{
	float v;
	asm("fcos"
		:"=t"(v) // "t": first (top of stack) floating point register
		:"0"(radians)
	);
	return v;
}

// puts sin and cos value of 'radians' in 'fsin' and 'fcos'
inline auto sin_and_cos(const Angle radians,float&fsin,float&fcos){
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

constexpr float PI=3.141592653589793f;

constexpr auto deg_to_rad(const Degrees deg)->Angle{
	constexpr float deg_to_rad=PI/180.f;
	return deg*deg_to_rad;
}

class Vector2{
public:
	float x=0,y=0;
//	Vector2D(){}
//	Vector2D(const float x_,const float y_):x{x_},y{y_}{}
	// normalizes this vector
	inline auto normalize()->Vector2&{
		const float len=sqrt(x*x+y*y);
		x/=len;
		y/=len;
		return*this;
	}
	// scales this vector
	inline constexpr auto scale(float s)->Vector2&{
		x*=s;
		y*=s;
		return*this;
	}
	inline constexpr auto inc_by(const Vector2&v){
		x+=v.x;
		y+=v.y;
	}
	inline constexpr auto inc_by(const Vector2&v,const float dt_s){
		x+=v.x*dt_s;
		y+=v.y*dt_s;
	}
	// negates this vector
	inline constexpr auto negate()->Vector2&{
		x=-x;
		y=-y;
		return*this;
	}
	inline constexpr auto dot(const Vector2&v)const->float{
		return x*v.x+y*v.y;
	}
//	auto operator<=>(const Vector2D&)const=default; // ? does not compile in clang++ without includes from std
	constexpr inline auto operator==(const Vector2&)const->bool=default;
	constexpr inline auto operator-(const Vector2&other)const->Vector2{return{x-other.x,y-other.y};}
	constexpr inline auto operator+(const Vector2&other)const->Vector2{return{x+other.x,y+other.y};}
};

using Scale=float;
using Count=int;

class Matrix2D{
	float xx=1,xy=0,xt=0;
	float yx=0,yy=1,yt=0;
	float ux=0,uy=0,id=1;
public:
	auto set_transform(const Scale scale,const Angle rotation,const Vector2&translation){
		float fcos,fsin;
		sin_and_cos(rotation,fsin,fcos);
		const float cs=scale*fcos;
		const float sn=scale*fsin;
		xx=cs;xy=-sn;xt=translation.x;
		yx=sn;yy= cs;yt=translation.y;
		ux= 0;uy=  0;id=1;
	}
	constexpr auto transform(const Vector2 src[],Vector2 dst[],const unsigned n)const{
		for(unsigned i=0;i<n;i++){
			dst->x=xx*src->x+xy*src->y+xt;
			dst->y=yx*src->x+yy*src->y+yt;
			src++;
			dst++;
		}
	}
	// does the rotation part of the transform
	constexpr auto rotate(const Vector2 src[],Vector2 dst[],const unsigned n)const{
		for(unsigned i=0;i<n;i++){
			dst->x=xx*src->x+xy*src->y;
			dst->y=yx*src->x+yy*src->y;
			src++;
			dst++;
		}
	}
	inline constexpr auto axis_x()const->Vector2{return{xx,yx};} // math correct?
	inline constexpr auto axis_y()const->Vector2{return{xy,yy};} // math correct?
};

} // end namespace osca
