#pragma once
namespace osca{

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
	inline auto set_transform(const Radians r,const Vector2D&t,const Vector2D&s){
		const float cs=s.x*cos(r);
		const float sn=s.y*sin(r);
		xx=cs;yx=-sn;tx=t.x;
		xy=sn,yy= cs,ty=t.y;
		xu= 0,yu=  0,i=1;
	}
	auto transform(const Vector2D src[],Vector2D dst[],const int n)const{
		for(int j=0;j<n;j++){
			dst->x=xx*src->x+yx*src->y+tx;
			dst->y=xy*src->x+yy*src->y+ty;
			src++;
			dst++;
		}
	}
	inline auto axis_x()const->Vector2D{
		return{xx,yx};
	}
	inline auto axis_y()const->Vector2D{
		return{xy,yy};
	}
};

} // end namespace osca
