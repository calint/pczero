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

class Object{
protected:
	Point2D pos_; // ? pos,rot,dpos,drot not cache friendly
	Point2D dpos_;
	Radians rot_;
	Radians drot_;
	Scale scl_;
	Point2D Mmw_pos_; // pos in transform matrix
	Radians Mmw_rot_;
	Scale Mmw_scl_;
	const ObjectDef&def_;
	Point2D*pts_wld; // transformed model to world points cache
	Matrix2D Mmw; // model to world transform
	unsigned char color_;
	unsigned char padding1=0;
	unsigned char padding2=0;
	unsigned char padding3=0;
public:
//	Object()=delete;
	Object(const Object&)=delete; // copy ctor
	Object(Object&&)=delete; // move ctor
	Object&operator=(const Object&)=delete; // copy assignment
//	Object&operator=(Object&&)=delete; // move assignment
	Object(const ObjectDef&def,const Scale scl,const Point2D&pos,const Radians rot,const unsigned char color):
		pos_{pos},
		dpos_{0},
		rot_{rot},
		drot_{0},
		scl_{scl},
		Mmw_pos_{0,0},
		Mmw_rot_{0},
		Mmw_scl_{0},
		def_{def},
		pts_wld{new Point2D[def.pts_count]},
		Mmw{},
		color_{color}
	{}
	virtual~Object(){
		delete[]pts_wld;
	}
	inline auto def()->const ObjectDef&{return def_;}
	inline auto pos()->const Point2D&{return pos_;}
	inline auto dpos()->const Point2D&{return dpos_;}
	inline auto set_position(const Point2D&p){pos_=p;}
	inline auto set_dposition(const Point2D&p){dpos_=p;}
	inline auto set_rotation(const Radians r){rot_=r;}
	inline auto set_drotation(const Radians r){drot_=r;}

	virtual auto update()->void{
		// used to print errors at row 1 column 1
//		err.p("uo ");
		pos_.inc_by(dpos_);
		rot_+=drot_;
	}
	virtual auto render(Bitmap&dsp)->void{
		// check if model to world matrix needs update
		if(rot_!=Mmw_rot_||pos_!=Mmw_pos_||scl_!=Mmw_scl_){
//			err.printer().p("um:").p_hex_32b(reinterpret_cast<unsigned>(this)).p(' ');
			Mmw.set_transform(scl_,rot_,pos_);
			Mmw_rot_=rot_;
			Mmw_pos_=pos_;
			Mmw_scl_=scl_;
			Mmw.transform(def_.pts,pts_wld,def_.pts_count);
		}
		Point2D*ptr=pts_wld;
		for(unsigned i=0;i<def_.pts_count;i++){
			dot(dsp,ptr->x,ptr->y,color_);
			ptr++;
		}
	}
};

class Ship:public Object{
public:
	Ship():
		Object{default_object_def_ship,5,{120,100},0,2}
	{}
	virtual auto update()->void override{
		Object::update();
		if(pos_.x>180){
			set_dposition({-dpos_.x,dpos_.y});
		}else if(pos_.x<50){
			set_dposition({-dpos_.x,dpos_.y});
		}
		if(pos_.y>130){
			set_dposition({dpos_.x,-dpos_.y});
		}else if(pos_.y<70){
			set_dposition({dpos_.x,-dpos_.y});
		}
	}
};
