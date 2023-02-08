#pragma once
#include "lib2d.h"

namespace osca{

using Point2D=Vector2D;

class ObjectDef final{
public:
	unsigned npts_=0; // number of points
	Point2D*pts_=nullptr; // array of points
};

static constexpr void dot(const Bitmap&bmp,const float x,const float y,const unsigned char color){
	const int xi=static_cast<int>(x);
	const int yi=static_cast<int>(y);
	bmp.pointer_offset({xi,yi}).write(color);
}

constexpr static unsigned objects_max=64; // maximum number of objects

class Object;
// physics states are kept in their own buffer for better CPU cache utilization at update
class PhysicsState final{
public:
	Point2D pos_{0,0};
	Point2D dpos_{0,0};
	Point2D ddpos_{0,0};
	Angle agl_=0;
	Angle dagl_=0;
	Object*obj_=nullptr; // pointer to the object to which this physics state belongs to

	inline constexpr auto pos()const->const Point2D&{return pos_;}
	inline constexpr auto dpos()const->const Point2D&{return dpos_;}
	inline constexpr auto ddpos()const->const Point2D&{return ddpos_;}
	inline constexpr auto angle()const->Angle{return agl_;}
	inline constexpr auto dangle()const->Angle{return dagl_;}
	inline constexpr auto set_pos(const Point2D&p){pos_=p;}
	inline constexpr auto set_dpos(const Point2D&p){dpos_=p;}
	inline constexpr auto set_ddpos(const Point2D&p){ddpos_=p;}
	inline constexpr auto set_angle(const Angle rad){agl_=rad;}
	inline constexpr auto set_dangle(const Angle rad){dagl_=rad;}
	constexpr inline auto update()->void{
		dpos_.inc_by(ddpos_);
		pos_.inc_by(dpos_);
		agl_+=dagl_;
	}

	//-----------------------------------------------------------
	//-----------------------------------------------------------
	//-----------------------------------------------------------

	static PhysicsState*mem_start;
	static PhysicsState*next_free;
	static PhysicsState*mem_limit;
	static auto init_statics(){
		mem_start=new PhysicsState[objects_max];
//		mem_start=reinterpret_cast<PhysicsState*>(bufferStart);
		next_free=mem_start;
		mem_limit=reinterpret_cast<PhysicsState*>(mem_start+objects_max);
	}
	static auto alloc()->PhysicsState*{
		// check buffer overrun
		if(next_free==mem_limit){
			err.p("PhysicsState:e1");
			osca_halt();
		}
		PhysicsState*p=next_free;
		next_free++;
		return p;
	}
	// returns pointer to object that has a new address for physics state
	static auto free(PhysicsState*phy)->Object*{
		// decrement next_free to point to last state in heap
		// copy last state to the freed area
		// return pointer to object that has had it's phy_ moved

		// check buffer underflow
//		if(next_free==mem_start){
//			out.printer().p("PhysicsState:e2");
//		}
		next_free--;
		Object*o=next_free->obj_;
		*phy=*next_free;
//		pz_memset(next_free,3,sizeof(PhysicsState)); // ? debugging
		return o;
	}
	static auto update_physics_states(){
		PhysicsState*ptr=mem_start;
		while(ptr<next_free){
			ptr->update();
			ptr++;
		}
	}
	static auto clear_buffer(unsigned char b=0){
		const Address from=Address(mem_start);
		const SizeBytes n=reinterpret_cast<SizeBytes>(mem_limit)-reinterpret_cast<SizeBytes>(mem_start);
		pz_memset(from,b,n);
	}
};
PhysicsState*PhysicsState::mem_start;
PhysicsState*PhysicsState::mem_limit;
PhysicsState*PhysicsState::next_free;

class Object{
protected:
	PhysicsState*phy_; // kept in own buffer of states for better CPU cache utilization at update
	                   // may change between frames (when objects are deleted)
	Scale scl_;
	const ObjectDef&def_;
	Point2D*pts_wld_; // transformed model to world points cache
	Matrix2D Mmw_; // model to world transform
	Point2D Mmw_pos_; // current position used in transform matrix
	Angle Mmw_agl_; // current angle used in transform matrix
	Scale Mmw_scl_;  // current scale used in transform matrix
	unsigned char color_;
	unsigned char padding1=0;
	unsigned short slot_=0; // index in objects pointer array
public:
//	constexpr Object()=delete;
	constexpr Object(const Object&)=delete; // copy ctor
//	constexpr Object(Object&&)=delete; // move ctor
	constexpr Object&operator=(const Object&)=delete; // copy assignment
//	Object&operator=(Object&&)=delete; // move assignment
	Object(const ObjectDef&def,const Scale scl,const Point2D&pos,const Angle rad,const unsigned char color):
		phy_{PhysicsState::alloc()},
		scl_{scl},
		def_{def},
		pts_wld_{new Point2D[def.npts_]},
		Mmw_{},
		Mmw_pos_{0,0},
		Mmw_agl_{0},
		Mmw_scl_{0},
		color_{color}
	{
		phy_->pos_=pos;
		phy_->dpos_={0,0};
		phy_->ddpos_={0,0};
		phy_->agl_=rad;
		phy_->dagl_=0;
		phy_->obj_=this;

		if(!freeSlots_ix){
			err.pos({1,1}).p("out of free slots");
			osca_halt();
			return;
		}
		slot_=freeSlots[freeSlots_ix];
		all[slot_]=this;
		freeSlots_ix--;
	}
	virtual~Object(){
		// free returns a pointer to the object that has had it's
		// physics state moved to the newly freed physics location.
		// set the pointer of that object's phy to the freed one
		PhysicsState::free(this->phy_)->phy_=phy_;
		all[slot_]=nullptr;
		freeSlots_ix++;
		freeSlots[freeSlots_ix]=slot_;
		delete[]pts_wld_;
	}
	inline constexpr auto phy()->PhysicsState&{return*phy_;}
	inline constexpr auto scale()const->Scale{return scl_;}
	inline constexpr auto def()const->const ObjectDef&{return def_;}
	auto forward_vector()->Vector2D{
		refresh_Mmw_if_invalid();
		return Mmw_.axis_y().negate().normalize();
	}
	// returns false if object is to be deleted
	constexpr virtual auto update()->bool{
		return true;
	}
	constexpr virtual auto render(const Bitmap&dsp)->void{
		if(refresh_Mmw_if_invalid()){
			// matrix has been updated, update cached points
			Mmw_.transform(def_.pts_,pts_wld_,def_.npts_);
		}
		Point2D*ptr=pts_wld_;
		for(unsigned i=0;i<def_.npts_;i++){
			dot(dsp,ptr->x,ptr->y,color_);
			ptr++;
		}
	}
private:
	constexpr auto refresh_Mmw_if_invalid()->bool{
		if(phy().agl_==Mmw_agl_&&phy().pos_==Mmw_pos_&&scl_==Mmw_scl_)
			return false;
		Mmw_.set_transform(scl_,phy().agl_,phy().pos_);
		Mmw_scl_=scl_;
		Mmw_agl_=phy().agl_;
		Mmw_pos_=phy().pos_;
		return true;
	}
	//----------------------------------------------------------------
	// statics
	//----------------------------------------------------------------
public:
	static Object*all[objects_max]; // array of pointers to allocated objects
	static unsigned short freeSlots[objects_max]; // free indexes in all[]
	static unsigned short freeSlots_ix; // index in freeSlots[] of next free slot
	static inline auto hasFreeSlot()->bool{return freeSlots_ix!=0;}
	static auto init_statics(){
		const unsigned n=sizeof(freeSlots)/sizeof(unsigned short);
	//	out.printer().p_hex_32b(n).spc().p_hex_16b(objects_free_indexes_pos).spc();
		for(unsigned short i=0;i<n;i++){
			freeSlots[i]=i;
		}
		freeSlots_ix=objects_max-1;
	}
	static auto update_all(){
		static Object*deleted[objects_max]; // ? temporary impl
		int deleted_ix=0;
		for(Object*o:Object::all){
			if(!o)
				continue;
			if(!o->update()){
				deleted[deleted_ix]=o;
				deleted_ix++;
			}
		}
	//	out.printer().pos(1,1);
		for(int i=0;i<deleted_ix;i++){
	//		out.printer().p_hex_16b(static_cast<unsigned short>(i)).p(' ');
			delete deleted[i];
		}
	}
	static auto render_all(Bitmap&bmp){
		for(Object*o:Object::all){
			if(!o)
				continue;
			o->render(bmp);
		}
	}
};
Object*Object::all[objects_max];
unsigned short Object::freeSlots[objects_max];
unsigned short Object::freeSlots_ix=objects_max-1;

}// end namespace
