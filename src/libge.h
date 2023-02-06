#pragma once
#include "lib2d.h"

namespace osca{

using Point2D=Vector2D;

class ObjectDef{
public:
	Point2D*pts_=nullptr;
	unsigned npts_=0; // number of points
};

static constexpr void dot(const Bitmap&bmp,const float x,const float y,const unsigned char color){
	const int xi=static_cast<int>(x);
	const int yi=static_cast<int>(y);
	bmp.pointer_offset({xi,yi}).write(color);
}

class Object;

class Object{
public:
	constexpr static unsigned all_len=8; // maximum number of objects
	static Object*all[]; // array of pointers to allocated objects
	static unsigned short freeSlots[all_len]; // free indexes in all[]
	static unsigned short freeSlots_pos; // index in freeSlots[] of next free slot
	static inline auto hasFreeSlot()->bool{return freeSlots_pos!=0;}
	static auto init_statics(){
		const unsigned n=sizeof(freeSlots)/sizeof(unsigned short);
	//	out.printer().p_hex_32b(n).spc().p_hex_16b(objects_free_indexes_pos).spc();
		for(unsigned short i=0;i<n;i++){
			freeSlots[i]=i;
		}
		freeSlots_pos=all_len-1;
	}
	static auto all_update();
	static auto all_render(Bitmap&bmp);
protected:
	Point2D pos_; // ? [pos,dpos,agl,dagle] object size trashes cache when doing update
	Point2D dpos_;
	Angle agl_;
	Angle dagl_;
	Scale scl_;
	const ObjectDef&def_;
	Point2D*pts_wld_; // transformed model to world cache
	Matrix2D Mmw_; // model to world transform
	Point2D Mmw_pos_; // pos in transform matrix
	Angle Mmw_agl_;
	Scale Mmw_scl_;
	unsigned char color_;
	unsigned char padding1=0;
	unsigned short slot_=0; // index in objects pointer array
public:
	constexpr Object()=delete;
	constexpr Object(const Object&)=delete; // copy ctor
	constexpr Object(Object&&)=delete; // move ctor
	constexpr Object&operator=(const Object&)=delete; // copy assignment
//	Object&operator=(Object&&)=delete; // move assignment
	Object(const ObjectDef&def,const Scale scl,const Point2D&pos,const Angle rad,const unsigned char color):
		pos_{pos},
		dpos_{0,0},
		agl_{rad},
		dagl_{0},
		scl_{scl},
		def_{def},
		pts_wld_{new Point2D[def.npts_]},
		Mmw_{},
		Mmw_pos_{0,0},
		Mmw_agl_{0},
		Mmw_scl_{0},
		color_{color}
	{
		if(!freeSlots_pos){
			out.printer().p("e ");
			return;
		}
		slot_=freeSlots[freeSlots_pos];
		all[slot_]=this;
		freeSlots_pos--;
//		out.printer().p_hex_16b(freeSlots_pos).spc();
	}
	virtual~Object(){
		all[slot_]=nullptr;
		freeSlots_pos++;
		freeSlots[freeSlots_pos]=slot_;
		delete[]pts_wld_;
//		out.printer().p_hex_16b(freeSlots_pos).spc();
	}
	inline constexpr auto def()const->const ObjectDef&{return def_;}
	inline constexpr auto pos()const->const Point2D&{return pos_;}
	inline constexpr auto dpos()const->const Point2D&{return dpos_;}
	inline constexpr auto angle()const->Angle{return agl_;}
	inline constexpr auto scale()const->Scale{return scl_;}
	inline constexpr auto set_pos(const Point2D&p){pos_=p;}
	inline constexpr auto set_dpos(const Point2D&p){dpos_=p;}
	inline constexpr auto set_angle(const Angle rad){agl_=rad;}
	inline constexpr auto set_dangle(const Angle rad){dagl_=rad;}
	constexpr auto forward_vector()->Vector2D{
		refresh_Mmw_if_invalid();
		return Mmw_.axis_y().negate();
	}
	constexpr virtual auto die()->void{
//		unslot_object(this);
//		delete this;
	}
	constexpr virtual auto update()->void{
		pos_.inc_by(dpos_);
		agl_+=dagl_;
	}
	constexpr virtual auto render(const Bitmap&dsp)->void{
		if(refresh_Mmw_if_invalid()){
			// matrix is refreshed
			Mmw_.transform(def_.pts_,pts_wld_,def_.npts_);
		}
		// check if model to world matrix needs update
		Point2D*ptr=pts_wld_;
		for(unsigned i=0;i<def_.npts_;i++){
			dot(dsp,ptr->x,ptr->y,color_);
			ptr++;
		}
	}
	auto fire(){

	}
private:
	constexpr auto refresh_Mmw_if_invalid()->bool{
		if(agl_==Mmw_agl_&&pos_==Mmw_pos_&&scl_==Mmw_scl_)
			return false;
//			err.printer().p("um:").p_hex_32b(reinterpret_cast<unsigned>(this)).p(' ');
		Mmw_.set_transform(scl_,agl_,pos_);
		Mmw_agl_=agl_;
		Mmw_pos_=pos_;
		Mmw_scl_=scl_;
		return true;
	}
};
Object*Object::all[all_len];
unsigned short Object::freeSlots_pos=all_len-1;
unsigned short Object::freeSlots[all_len];
auto Object::all_update(){
	for(Object*o:Object::all){
		if(!o)
			continue;
		o->update();
	}
}
auto Object::all_render(Bitmap&bmp){
	for(Object*o:Object::all){
		if(!o)
			continue;
		o->render(bmp);
	}
}

}// end namespace
