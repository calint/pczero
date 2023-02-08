#pragma once
#include "lib2d.h"

namespace osca{

using Point2D=Vector2D;

using PointIx=unsigned short;

class ObjectDef final{
public:
	unsigned short npts=0; // number of points in pts
	unsigned short nbnd=0; // number of indexes in bnd
	Point2D*pts=nullptr; // array of points used for rendering and bounding shape
	PointIx*bnd=nullptr; // indexes in pts that defines the bounding shape as a convex polygon
	Vector2D*nmls=nullptr; // normals to the lines defined in bnd

	auto init_normals(){
		nmls=new Vector2D[nbnd];
		const unsigned n=nbnd-1;
		for(unsigned i=0;i<n;i++){
			const Vector2D d=pts[bnd[i+1]]-pts[bnd[i]];
			nmls[i]={-d.y,d.x}; // normal to he line d
			nmls[i].normalize();
		}
		const Vector2D d=pts[bnd[0]]-pts[bnd[nbnd-1]];
		nmls[nbnd-1]={-d.y,d.x}; // normal to he line d
		nmls[nbnd-1].normalize();
	}
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
	Point2D pos{0,0};
	Point2D dpos{0,0};
	Point2D ddpos{0,0};
	Angle agl=0;
	Angle dagl=0;
	Object*obj=nullptr; // pointer to the object to which this physics state belongs to

//	inline constexpr auto pos()const->const Point2D&{return pos_;}
//	inline constexpr auto dpos()const->const Point2D&{return dpos_;}
//	inline constexpr auto ddpos()const->const Point2D&{return ddpos_;}
//	inline constexpr auto angle()const->Angle{return agl_;}
//	inline constexpr auto dangle()const->Angle{return dagl_;}
//	inline constexpr auto set_pos(const Point2D&p){pos_=p;}
//	inline constexpr auto set_dpos(const Point2D&p){dpos_=p;}
//	inline constexpr auto set_ddpos(const Point2D&p){ddpos_=p;}
//	inline constexpr auto set_angle(const Angle rad){agl_=rad;}
//	inline constexpr auto set_dangle(const Angle rad){dagl_=rad;}
	constexpr inline auto update()->void{
		dpos.inc_by(ddpos);
		pos.inc_by(dpos);
		agl+=dagl;
	}

	//-----------------------------------------------------------
	//-----------------------------------------------------------
	//-----------------------------------------------------------

	static PhysicsState*mem_start;
	static PhysicsState*next_free;
	static PhysicsState*mem_limit;
	static auto init_statics(){
		mem_start=new PhysicsState[objects_max];
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
		Object*o=next_free->obj;
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

namespace enable{
	constexpr bool draw_normals=false;
	constexpr bool draw_collision_check=false;
	constexpr bool draw_bounding_circle=false;
}
namespace metrics{
	unsigned short collisions_check=0;
	unsigned short collisions_check_bounding_shapes=0;
	auto reset();
	auto reset(){
		collisions_check=0;
		collisions_check_bounding_shapes=0;
	}
}

using SlotIx=unsigned short; // index in Object::freeSlots[]
using ObjectIx=unsigned short; // index in Object::all[]
using TypeBits=unsigned;
struct SlotInfo{
	Object**oix=nullptr; // pointer to element in Object::all[]
	Object*obj=nullptr; // object owning this slot
};
class Object{
protected:
	TypeBits tb_;
	TypeBits colchk_tb_; // bits used to and with other object type_bits and if true then wants collision check
	PhysicsState*phy_; // kept in own buffer of states for better CPU cache utilization at update
	                   // may change between frames (when objects are deleted)
	Scale scl_;
	const ObjectDef&def_;
	Point2D*pts_wld_; // transformed model to world points cache
	Vector2D*nmls_wld_; // normals of bounding shape rotated to the world coordinates (not normalized if scale!=1)
	Matrix2D Mmw_; // model to world transform
	Point2D Mmw_pos_; // current position used in transform matrix
	Angle Mmw_agl_; // current angle used in transform matrix
	Scale Mmw_scl_;  // current scale used in transform matrix
//	Object**obj_ix_=nullptr; // pointer to element int all[]
	SlotIx used_ix_=0; // index in used_ixes array
	unsigned char color_=1;
	char padding1=0;
public:
//	constexpr Object()=delete;
	constexpr Object(const Object&)=delete; // copy ctor
//	constexpr Object(Object&&)=delete; // move ctor
	constexpr Object&operator=(const Object&)=delete; // copy assignment
//	Object&operator=(Object&&)=delete; // move assignment
	Object(const TypeBits tb,const TypeBits colchk_tb,const ObjectDef&def,const Scale scl,const Point2D&pos,const Angle rad,const unsigned char color):
		tb_{tb},
		colchk_tb_{colchk_tb},
		phy_{PhysicsState::alloc()},
		scl_{scl},
		def_{def},
		pts_wld_{new Point2D[def.npts]},
		nmls_wld_{new Vector2D[def.nbnd]},
		Mmw_{},
		Mmw_pos_{0,0},
		Mmw_agl_{0},
		Mmw_scl_{0},
		color_{color}
	{
		// initiate physics state
		*phy_=PhysicsState{};
		phy_->pos=pos;
		phy_->agl=rad;
		phy_->obj=this;

		// allocate index in all[] from free slots
		if(!free_ixes_i){
			err.p("out of free slots");
			osca_halt();
		}
		// get the next free slot
		Object**obj_ix_=free_ixes[free_ixes_i]; // pointer to element in all[]
		free_ixes_i--;
		// assign slot to this object
		*obj_ix_=this;
		// add the new slot to used objects
		used_ixes[used_ixes_i]={obj_ix_,this};
		used_ix_=used_ixes_i;
		used_ixes_i++;
	}
	virtual~Object(){
		// free returns a pointer to the object that has had it's
		// physics state moved to the newly freed physics location.
		// set the pointer of that object's phy to the freed one
		PhysicsState::free(this->phy_)->phy_=phy_;

		// get slot info for this object
		SlotInfo this_slot=used_ixes[used_ix_]; // ? this lookup can be optimized with a **Object in a field. speed vs space
		*this_slot.oix=nullptr;
		// add slot to free slots
		free_ixes_i++;
		free_ixes[free_ixes_i]=this_slot.oix;
		// move last slot in 'used' array to the freed slot
		used_ixes_i--;
		SlotInfo movedSlot=used_ixes[used_ixes_i];
		// update object used_ix_ to the freed slot index
		movedSlot.obj->used_ix_=used_ix_;
		// store it in the freed slot
		used_ixes[used_ix_]=movedSlot;
		// delete cached points
		delete[]pts_wld_;
		delete[]nmls_wld_;
	}
	inline constexpr auto type_bits()const->TypeBits{return tb_;}	// returns false if object is to be deleted
	inline constexpr auto phy()->PhysicsState&{return*phy_;}
	inline constexpr auto scale()const->Scale{return scl_;}
	inline constexpr auto def()const->const ObjectDef&{return def_;}
	auto forward_vector()->Vector2D{
		refresh_Mmw_if_invalid();
		return Mmw_.axis_y().negate().normalize(); // ? not negated (if pos y is up)
	}
	// returns false if object is to be deleted
	constexpr virtual auto update()->bool{
		return true;
	}
	constexpr virtual auto render(const Bitmap&dsp)->void{
		refresh_wld_points();
		Point2D*pt=pts_wld_;
		for(unsigned i=0;i<def_.npts;i++){
			dot(dsp,pt->x,pt->y,color_);
			pt++;
		}
		if(enable::draw_normals){
			Point2D*nml=nmls_wld_;
			for(unsigned i=0;i<def_.nbnd;i++){
				Vector2D v=*nml;
				v.normalize().scale(3);
				Point2D p=pts_wld_[def_.bnd[i]];
				Vector2D v1={p.x+nml->x,p.y+nml->y};
				dot(dsp,v1.x,v1.y,0xf);
				nml++;
			}
		}
	}
	// returns false if object is to be deleted
	constexpr virtual auto on_collision(Object&other)->bool{
		return true;
	}
private:
	constexpr auto refresh_wld_points()->void{
		if(!refresh_Mmw_if_invalid())
			return;
		// matrix has been updated, update cached points
		Mmw_.transform(def_.pts,pts_wld_,def_.npts);
		Mmw_.rotate(def_.nmls,nmls_wld_,def_.nbnd);
	}
	constexpr auto refresh_Mmw_if_invalid()->bool{
		if(phy().agl==Mmw_agl_&&phy().pos==Mmw_pos_&&scl_==Mmw_scl_)
			return false;
		Mmw_.set_transform(scl_,phy().agl,phy().pos);
		Mmw_scl_=scl_;
		Mmw_agl_=phy().agl;
		Mmw_pos_=phy().pos;
		return true;
	}
	//----------------------------------------------------------------
	// statics
	//----------------------------------------------------------------
public:
	static Object*all[objects_max]; // array of pointers to allocated objects
	static Object**free_ixes[objects_max]; // free indexes in all[]
	static SlotIx free_ixes_i; // index in freeSlots[] of next free slot
	static SlotInfo used_ixes[objects_max]; // free indexes in all[]
	static SlotIx used_ixes_i; // index in freeSlots[] of next free slot
	static inline auto hasFreeSlot()->bool{return free_ixes_i!=0;}
	static auto init_statics(){
		const unsigned n=sizeof(free_ixes)/sizeof(Object**);
		for(SlotIx i=0;i<n;i++){
			free_ixes[i]=&all[i];
		}
		free_ixes_i=objects_max-1;
	}
	static inline auto object_for_used_slot(const SlotIx i)->Object*{
		Object*o=used_ixes[i].obj;
		if(!o){
			err.p("null-pointer-exception [e1]");
			osca_halt();
		}
		return o;
	}
	static auto update_all(){
		static Object*deleted[objects_max]; // ? todo temporary for now
		int deleted_ix=0;

		for(SlotIx i=0;i<used_ixes_i;i++){
			Object*o=object_for_used_slot(i);
			if(!o->update()){
				deleted[deleted_ix]=o;
				deleted_ix++;
			}
		}

		for(int i=0;i<deleted_ix;i++){
			delete deleted[i];
		}
	}
	static auto render_all(Bitmap&dsp){
		for(SlotIx i=0;i<used_ixes_i;i++){
			Object*o=object_for_used_slot(i);
			o->render(dsp);
		}
	}
	static auto check_collision_bounding_circles(Object&o1,Object&o2)->bool{
		if(enable::draw_bounding_circle){
			draw_bounding_circle(vga13h.bmp(),o1);
			draw_bounding_circle(vga13h.bmp(),o2);
		}
		const float r1=o1.scale();
		const float r2=o2.scale();
		const Point2D p1=o1.phy().pos;
		const Point2D p2=o2.phy().pos;

		// check if: sqrt(dx*dx+dy*dy)<r1+r2
		const float dist2=r1*r1+2*r1*r2+r2*r2; // distance^2
		Vector2D v{p2.x-p1.x,p2.y-p1.y};
		v.x*=v.x;
		v.y*=v.y;
		const float d2=v.x+v.y;
		if(d2>dist2)
			return false;
//		out.p("bounds ");
		return true;
	}
	// checks if o1 points in o2 bounding shape
	static auto is_in_collision(Object&o1,Object&o2)->bool{
		// check bounding spheres
		o1.refresh_wld_points();
		o2.refresh_wld_points();
		// for each point in o1 check if behind every normal of o2
		// if behind every normal then within the convex polygon thus collision
		bool is_collision=false;
		// for each point in o1
		for(unsigned i=0;i<o1.def_.npts;i++){
			const Point2D&p1=o1.pts_wld_[i];
			is_collision=true; // assume is collision
			// for each normal in o2
			for(unsigned j=0;j<o2.def_.nbnd;j++){
				const Vector2D&p2=o2.pts_wld_[o2.def_.bnd[j]];
				if(enable::draw_collision_check){
					dot(vga13h.bmp(),p2.x,p2.y,5);
				}
				const Vector2D&nl=o2.nmls_wld_[j];
				const Vector2D v=p1-p2;
				if(v.dot(nl)>=0){ // ? use abs(v)<0.0001f (example)
					// p "in front" of v, cannot be collision
					is_collision=false;
					break;
				}
			}
			// if point within all lines then p1 is within o2 bounding shape
			if(is_collision)
				return true;
		}
		return false;
	}
	static auto draw_bounding_circle(Bitmap&dsp,Object&o)->void{
		Point2D p=o.phy().pos;
		float scl=o.scale();
		const unsigned segments=static_cast<unsigned>(4.f*o.scale());
		Angle th=0;
		Angle dth=2*PI/static_cast<Angle>(segments);
		for(unsigned i=0;i<segments;i++){
			const Coord x=p.x+scl*cos(th);
			const Coord y=p.y+scl*sin(th);
			dot(dsp,x,y,1);
			th+=dth;
		}
	}
	static auto check_collisions(){
		static Object*deleted[objects_max]; // ? todo temporary for now
		int deleted_ix=0;

		for(unsigned i=0;i<used_ixes_i-1u;i++){
			for(unsigned j=i+1;j<used_ixes_i;j++){
				Object*o1=used_ixes[i].obj;
				Object*o2=used_ixes[j].obj;
				// check if objects are interested in collision check
				const TypeBits tb1=o1->type_bits();
				const TypeBits tb2=o2->type_bits();
				const bool o1_check_col_with_o2=o1->colchk_tb_&o2->tb_;
				const bool o2_check_col_with_o1=o2->colchk_tb_&o1->tb_;
				if(!o1_check_col_with_o2&&!o2_check_col_with_o1)
					continue;
//				out.p("chk ").p_hex_8b(static_cast<unsigned char>(tb1)).p(' ').p_hex_8b(static_cast<unsigned char>(tb2)).p(' ');
				metrics::collisions_check++;
				if(!Object::check_collision_bounding_circles(*o1,*o2))
					continue;
				metrics::collisions_check_bounding_shapes++;
				// check if o1 points in o2 bounding shape
				if(Object::is_in_collision(*o1,*o2)){
					if(o1_check_col_with_o2){
						// o1 type wants to handle collisions with o2 type
						if(!o1->on_collision(*o2)){
							deleted[deleted_ix++]=o1;
						}
					}
					if(o2_check_col_with_o1){
						// o2 type wants to handle collisions with o1 type
						if(!o2->on_collision(*o1)){
							deleted[deleted_ix++]=o2;
						}
					}
					continue;
				}
				// check if o2 points in o1 bounding shape
				if(Object::is_in_collision(*o2,*o1)){
					if(o1_check_col_with_o2){
						if(!o1->on_collision(*o2)){
							deleted[deleted_ix++]=o1;
						}
					}
					if(o2_check_col_with_o1){
						if(!o2->on_collision(*o1)){
							deleted[deleted_ix++]=o2;
						}
					}
				}
			}
		}
		for(int i=0;i<deleted_ix;i++){
			delete deleted[i];
		}
	}
};
Object*Object::all[objects_max];
Object**Object::free_ixes[objects_max];
SlotIx Object::free_ixes_i=objects_max-1;
SlotInfo Object::used_ixes[objects_max];
SlotIx Object::used_ixes_i=0;

}// end namespace
