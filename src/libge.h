#pragma once
#include "lib2d.h"

using namespace osca;

using Point2D=Vector2D;

class ObjectDef{
public:
	Point2D*pts=nullptr;
	unsigned pts_count=0;
};

class ObjectDefRectangle:public ObjectDef{
public:
	ObjectDefRectangle(){
		pts_count=5;
		pts=new Point2D[pts_count]{
			{ 0, 0},
			{-2,-1},
			{ 2,-1},
			{ 2, 1},
			{-2, 1},
		};
	}
}default_object_def_rectangöe;

class ObjectDefShip:public ObjectDef{
public:
	ObjectDefShip(){
		pts_count=4;
		pts=new Point2D[pts_count]{
			{ 0, 0},
			{ 0,-1},
			{-1,.5},
			{ 1,.5},
		};
	}
}default_object_def_ship;

static void dot(const Bitmap&bmp,const float x,const float y,const unsigned char color){
	const int xi=static_cast<int>(x);
	const int yi=static_cast<int>(y);
	bmp.pointer_offset({xi,yi}).write(color);
}

class Object;
static Object*objects[16];
static unsigned short objects_free_indexes[16];
static unsigned short objects_free_indexes_pos=15;
//static auto slot_object(Object*o)->void;
//static auto unslot_object(Object*o)->void;
class Object{
protected:
	Point2D pos_; // ? pos,rot,dpos,drot not cache friendly
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
public:
	unsigned short slot=0;
//	Object()=delete;
	Object(const Object&)=delete; // copy ctor
	Object(Object&&)=delete; // move ctor
	Object&operator=(const Object&)=delete; // copy assignment
//	Object&operator=(Object&&)=delete; // move assignment
	Object(const ObjectDef&def,const Scale scl,const Point2D&pos,const Angle rad,const unsigned char color):
		pos_{pos},
		dpos_{0},
		agl_{rad},
		dagl_{0},
		scl_{scl},
		def_{def},
		pts_wld_{new Point2D[def.pts_count]},
		Mmw_{},
		Mmw_pos_{0,0},
		Mmw_agl_{0},
		Mmw_scl_{0},
		color_{color}
	{
//		slot_object(this);
	}
	virtual~Object(){
//		out.printer().p(" d ").p_hex_16b(slot);
//		unslot_object(this);
		objects[slot]=nullptr;
		delete[]pts_wld_;
	}
	inline auto def()const->const ObjectDef&{return def_;}
	inline auto pos()const->const Point2D&{return pos_;}
	inline auto dpos()const->const Point2D&{return dpos_;}
	inline auto angle()const->Angle{return agl_;}
	inline auto scale()const->Scale{return scl_;}
	inline auto set_pos(const Point2D&p){pos_=p;}
	inline auto set_dpos(const Point2D&p){dpos_=p;}
	inline auto set_angle(const Angle rad){agl_=rad;}
	inline auto set_dangle(const Angle rad){dagl_=rad;}
	auto forward_vector()->Vector2D{
		refresh_Mmw_if_invalid();
		return Mmw_.axis_y();
	}
	virtual auto die()->void{
//		unslot_object(this);
//		delete this;
	}
	virtual auto update()->void{
		pos_.inc_by(dpos_);
		agl_+=dagl_;
	}
	virtual auto render(Bitmap&dsp)->void{
		if(refresh_Mmw_if_invalid()){
			// matrix is refreshed
			Mmw_.transform(def_.pts,pts_wld_,def_.pts_count);
		}
		// check if model to world matrix needs update
		Point2D*ptr=pts_wld_;
		for(unsigned i=0;i<def_.pts_count;i++){
			dot(dsp,ptr->x,ptr->y,color_);
			ptr++;
		}
	}
	auto fire(){

	}
private:
	auto refresh_Mmw_if_invalid()->bool{
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

//static auto slot_object(Object*o)->void{
////	o->slot=0;
////	return;
////	if(objects_free_indexes_pos==0){
////		out.printer().p("oos");
////		return;
////	}
//	unsigned short slot=objects_free_indexes[objects_free_indexes_pos];
//	o->slot=slot;
//	out.printer().p("a:").p_hex_16b(slot).p(' ');
//	objects_free_indexes_pos--;
//}
//
//static auto unslot_object(Object*o)->void{
//	objects_free_indexes_pos++;
//	objects_free_indexes[objects_free_indexes_pos]=o->slot;
//	out.printer().p("u ").p_hex_16b(o->slot).p(' ');
//	o->slot=0;
//}

class Ship:public Object{
public:
	Ship():
		Object{default_object_def_ship,5,{120,100},0,2}
	{}
	virtual auto update()->void override{
		Object::update();
		if(pos_.x>300){
			set_dpos({-dpos_.x,dpos_.y});
		}else if(pos_.x<20){
			set_dpos({-dpos_.x,dpos_.y});
		}
		if(pos_.y>130){
			set_dpos({dpos_.x,-dpos_.y});
		}else if(pos_.y<70){
			set_dpos({dpos_.x,-dpos_.y});
		}
	}
};

//class Bullet:public Object{
//public:
//	Bullet():
//		Object{default_object_def_ship,3,{0,0},0,4}
//	{}
//	virtual auto update()->void override{
//		Object::update();
//		if(pos_.x>300){
//			die();
//		}else if(pos_.x<20){
//			die();
//		}
//		if(pos_.y>130){
//			die();
//		}else if(pos_.y<70){
//			die();
//		}
//	}
//};
