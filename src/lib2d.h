#pragma once

namespace osca{

using Radians=float;
using Degrees=float;

inline auto sin(const Radians radians)->float{
	float v;
	asm("fsin"
		:"=t"(v) // "t": first (top of stack) floating point register
		:"0"(radians)
	);
	return v;
}

inline auto cos(const Radians radians)->float{
	float v;
	asm("fcos"
		:"=t"(v) // "t": first (top of stack) floating point register
		:"0"(radians)
	);
	return v;
}

inline auto sin_and_cos(const Radians radians,float&fsin,float&fcos){
	asm("fsincos"
		:"=t"(fcos),"=u"(fsin)
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

constexpr auto deg_to_rad(const Degrees deg)->Radians{
	constexpr float deg_to_rad=PI/180.f;
	return deg*deg_to_rad;
}

const float float_dif=0.000001f;

class Vector2D{
public:
	float x=0,y=0;
//	Vector2D(){}
//	Vector2D(const float x_,const float y_):x{x_},y{y_}{}
	// normalizes this vector
	inline auto normalize()->Vector2D&{
		const float len=sqrt(x*x+y*y);
		x/=len;
		y/=len;
		return*this;
	}
	// scales this vector
	inline auto scale(float s)->Vector2D&{
		x*=s;
		y*=s;
		return*this;
	}
	inline auto inc_by(const Vector2D&v){
		x+=v.x;
		y+=v.y;
	}
//	auto operator<=>(const Vector2D&)const=default; // ? does not compile in clang++ without includes from std
	auto operator==(const Vector2D&)const->bool=default;
};

using Scale=float;
using Count=int;

class Matrix2D{
	float	xx=1,xy=0,xt=0,
			yx=0,yy=1,yt=0,
			ux=0,uy=0,id=1;
public:
//	inline auto set_identity(){
//		xx=1;yx=0;tx=0;
//		xy=0,yy=1,ty=0;
//		xu=0,yu=0, i=1;
//	}
//	inline auto set_rotation(const Radians r){
//		const float cs=cos(r);
//		const float sn=sin(r);
//		xx=cs;yx=-sn;tx=0;
//		xy=sn,yy= cs,ty=0;
//		xu= 0,yu=  0,i=1;
//	}
//	inline auto set_translation(const Vector2D&v){
//		tx=v.x;
//		ty=v.y;
//	}

	inline auto set_transform(const Scale scale,const Radians rotation,const Vector2D&translation){
		// ! implement fsincos
		float fcos,fsin;
		sin_and_cos(rotation,fsin,fcos);
//		const float cs=scale*cos(rotation);
//		const float sn=scale*sin(rotation);
		const float cs=scale*fcos;
		const float sn=scale*fsin;
		xx=cs;xy=-sn;xt=translation.x;
		yx=sn,yy= cs,yt=translation.y;
		ux= 0,uy=  0,id=1;
	}
	auto transform(const Vector2D src[],Vector2D dst[],const unsigned n)const{
		for(unsigned i=0;i<n;i++){
			dst->x=xx*src->x+xy*src->y+xt;
			dst->y=yx*src->x+yy*src->y+yt;
			src++;
			dst++;
		}
	}
	inline auto axis_x()const->Vector2D{
		return{xx,xy};
	}
	inline auto axis_y()const->Vector2D{
		return{yx,yy};
	}
};

} // end namespace osca
