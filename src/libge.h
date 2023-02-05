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
		pts=new Point2D[pts_count];
		pts[0]={ 0, 0};
		pts[1]={-2,-1};
		pts[2]={ 2,-1};
		pts[3]={ 2, 1};
		pts[4]={-2, 1};
	}
}default_object_def_rectangöe;

class ObjectDefShip:public ObjectDef{
public:
	ObjectDefShip(){
		pts_count=4;
		pts=new Point2D[pts_count];
		pts[0]={ 0, 0};
		pts[1]={ 0,-1};
		pts[2]={-1,.5};
		pts[3]={ 1,.5};
	}
}default_object_def_ship;

static void dot(const Bitmap&bmp,const float x,const float y,const unsigned char color){
	const int xi=static_cast<int>(x);
	const int yi=static_cast<int>(y);
	bmp.pointer_offset({xi,yi}).write(color);
}

class Object{
protected:
	Point2D pos_;
	Radians rot_;
	Scale scl_;
	const ObjectDef&def_;
	Point2D*cached_pts;
	Matrix2D Mmw; // model to world transform
	bool Mmw_valid=false;// if true Mmw and cached_pts don't need update
	unsigned char color_;
	unsigned char padding1=0;
	unsigned char padding2=0;
public:
//	Object():
//		pos_{0,0},
//		rot_{0},
//		def_{default_object_def},
//		scl_{0},
//		cached_pts{nullptr},
//		color_{0}
//	{}
	Object()=delete;
	Object(const Object&)=delete; // copy ctor
	Object(Object&&)=delete; // move ctor
	Object&operator=(const Object&)=delete; // copy assignment
//	Object&operator=(Object&&)=delete; // move assignment
	Object(const ObjectDef&def,const Scale scl,const Point2D&pos,const Radians rot,const unsigned char color):
		pos_{pos},
		rot_{rot},
		scl_{scl},
		def_{def},
		cached_pts{new Point2D[def.pts_count]},
		Mmw{},
		color_{color}
	{}
	virtual~Object(){
		delete[]cached_pts;
	}
	inline auto def()->const ObjectDef&{return def_;}
	virtual auto update()->void{
		// used to print errors at row 1 column 1
//		err.p("uo ");
		rot_+=deg_to_rad(5);
		Mmw_valid=false;
	}
	virtual auto render(Bitmap&dsp)->void{
		if(!Mmw_valid){
			Mmw.set_transform(scl_,rot_,pos_);
			Mmw_valid=true;
			Mmw.transform(def_.pts,cached_pts,def_.pts_count);
		}
		Point2D*ptr=cached_pts;
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
//		err.p("us ");
		rot_-=deg_to_rad(5);
		Mmw_valid=false;
	}
};
