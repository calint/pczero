#pragma once
// reviewed: 2024-03-09

//
// osca game engine library
//

namespace osca{

class ObjectDef final{
public:
	// number of elements in 'points'
	PointIx points_size{};
	// number of elements in 'indexes'
	PointIx indexes_size{};
	Point*points{}; // list of points used for rendering and bounding shape
	// list of indexes in 'points' that defines the bounding shape as a convex polygon CCW
	PointIx*indexes{};
	// list of normals to the lines defined by 'indexes'. calculated by 'calculate_normals'
	Vector*normals{};
	
	// destructor cannot happen because object life time is program lifetime
	// void operator delete(void*)=delete;

	constexpr auto calculate_normals()->void{
		delete[]normals; // in case definition has changed
		if(indexes_size<3){ // not enough points for a shape
			// don't define normals
			return;
		}
		normals=new Vector[indexes_size];
		const PointIx n=indexes_size-1;
		for(PointIx i=0;i<n;i++){
			const Vector d{points[indexes[i+1]]-points[indexes[i]]};
			normals[i]=d.normal().normalize();
		}
		const Vector d{points[indexes[0]]-points[indexes[indexes_size-1]]};
		normals[indexes_size-1]=d.normal().normalize();
	}
};

namespace metrics{
	constexpr static bool enabled{true};
	static Count matrix_set_transforms{};
	static Count collisions_checks{};
	static Count collisions_checks_bounding_shapes{};
	
	static auto reset()->void{
		matrix_set_transforms=0;
		collisions_checks=0;
		collisions_checks_bounding_shapes=0;
	}
}

class Object;

// maximum number of objects (note: also used in kernel.h when initiating heap)
constexpr Size objects_size_max{256};

using Velocity=Vector;
using Acceleration=Vector;
using AngularVelocityRad=AngleRad;
using Time=double;
using TimeSec=Time;
using TimeStepSec=Real;

// physics states are kept in their own buffer for better cpu cache utilization at update
class PhysicsState final{
public:
	Point position{};
	Velocity velocity{};
	Acceleration acceleration{};
	AngleRad angle{};
	AngularVelocityRad angular_velocity{};
	// the object that this physics state belongs to
	Object*owner{};

	inline auto update(const TimeStepSec dt)->void{
		velocity.inc_by(acceleration,dt);
		position.inc_by(velocity,dt);
		angle+=angular_velocity*dt;
	}

	//-----------------------------------------------------------
	//--- statics
	//-----------------------------------------------------------

	inline static PhysicsState*ls_all{};
	inline static PhysicsState*ls_all_pos{};
	inline static PhysicsState*ls_all_end{};
	
	static auto init_statics()->void{
		ls_all=new PhysicsState[objects_size_max];
		ls_all_pos=ls_all;
		ls_all_end=ls_all+objects_size_max;
	}
	static auto alloc()->PhysicsState*{
		// check buffer overrun
		if(ls_all_pos==ls_all_end){
			err.p("PhysicsState:e1");
			osca_hang();
		}
		PhysicsState*next_free=ls_all_pos;
		ls_all_pos++;
		return next_free;
	}
	// returns reference to the object that has a new address for physics state
	// this function works with '~Object()' to relocate physics state
	static auto free(PhysicsState*phy)->Object*{
		ls_all_pos--;
		Object*obj=ls_all_pos->owner;
		*phy=*ls_all_pos;
		pz_memset(ls_all_pos,0x0f,sizeof(PhysicsState)); // debugging (can be removed)
		return obj;
	}
	static auto update_all(const float dt_s)->void{
		for(PhysicsState*it=ls_all;it<ls_all_pos;++it){
			it->update(dt_s);
		}
	}
	static auto clear(uint8 b=0)->void{
		const Address from=Address(ls_all);
		const SizeBytes n=SizeBytes(ls_all_end)-SizeBytes(ls_all);
		pz_memset(from,b,n);
	}
};

namespace enable{
	constexpr static bool draw_dots{true};
	constexpr static bool draw_polygons{true};
	constexpr static bool draw_normals{true};
	constexpr static bool draw_collision_check{};
	constexpr static bool draw_bounding_circle{true};
}

using TypeBits=uint32;
using Flags8b=uint8;
using Scalar=Real;

constexpr Scale sqrt_of_2=Real(1.414213562);

class Object{
	// pointer to element in 'all' list containing pointer to this object
	// used in ~Object()
	Object**ls_all_pos_{};
	// object type that is usually a bit (32 object types supported)
	TypeBits type_bits_{};
	// bits used to bitwise 'and' with other object's 'type_bits_'
	// if true then collision detection is done
	TypeBits type_bits_collision_mask_{};
	// physics state kept in own buffer of states for better cpu cache utilization at update
	// may change between frames (when objects are deleted and 'phy_' relocated)
	PhysicsState*phy_{};
	// scale that is used in model to world transform
	Scale scale_{};
	// contains the model definition
	const ObjectDef&def_;
	// transformed model to world points cache
	Point*points_world_{};
	// normals of bounding shape rotated to the world coordinates (not normalized if scale!=1)
	Vector*normals_world_{};
	// model to world transform
	Matrix Mmw_{};
	// position used in 'Mmw_'
	Point Mmw_position_{};
	// angle used in 'Mmw_'
	AngleRad Mmw_angle_{};
	// scale used in 'Mmw_'
	Scale Mmw_scale_{};
	// bounding radius that includes all points. usually: scale_*sqrt(2)
	Scale bounding_radius_{};
	Color8b color_{}; // shape color
	Flags8b flags_{}; // bit 1: is dead
	                  // bit 2: 'points_world_' need update
	uint8 padding[2]{};
public:
	Object(
		const TypeBits type_bits,
		const TypeBits type_bits_collision_mask,
		const ObjectDef&def,
		const Scale scale,
		const Scalar bounding_radius,
		const Point&position,
		const AngleRad angle,
		const Color8b color
	):
		type_bits_{type_bits},
		type_bits_collision_mask_{type_bits_collision_mask},
		phy_{PhysicsState::alloc()},
		scale_{scale},
		def_{def},
		points_world_{new Point[def.points_size]},
		// object def might be a dot or a line and not have normals -> no bounding shape
		normals_world_{def.normals?new Vector[def.indexes_size]:nullptr},
		bounding_radius_{bounding_radius},
		color_{color}
	{
		// initiate physics state
		*phy_=PhysicsState{};
		phy_->position=position;
		phy_->angle=angle;
		phy_->owner=this;

		// allocate index in 'all[]' from free slots
		if(ls_all_pos==ls_all_end){
			err.p("Object:e1");
			osca_hang();
		}
		
		*ls_all_pos=this;
		ls_all_pos_=ls_all_pos;
		ls_all_pos++;
	}
	// called only in 'commit_deleted()'
	virtual~Object(){
		// free returns a pointer to the object that has had it's
		// physics state moved to the newly freed physics location.
		// set the pointer of that object's 'phy_' to the new location
		PhysicsState::free(this->phy_)->phy_=phy_;

		// move the last object pointer in list to the slot this object had
		ls_all_pos--;
		*ls_all_pos_=*ls_all_pos;
		*ls_all_pos=nullptr; // debugging (can be removed)
		// adjust pointer to the freed slot in 'all' list
		(*ls_all_pos_)->ls_all_pos_=ls_all_pos_;

		// delete cache
		delete[]points_world_;
		delete[]normals_world_;
	}

	constexpr Object()=delete;
	constexpr Object(const Object&)=delete;
	constexpr Object&operator=(const Object&)=delete;
	constexpr Object(Object&&)=delete;
	constexpr Object&operator=(Object&&)=delete;

	inline constexpr auto type_bits()const->TypeBits{return type_bits_;}
	inline constexpr auto type_bits_collision_mask()const->TypeBits{return type_bits_collision_mask_;}
	inline constexpr auto phy()->PhysicsState&{return*phy_;}
	inline constexpr auto phy_const()const->const PhysicsState&{return*phy_;}
	inline constexpr auto scale()const->Scale{return scale_;}
	inline constexpr auto set_scale(const Scale s)->void{scale_=s;}
	inline constexpr auto def()const->const ObjectDef&{return def_;}
	inline constexpr auto is_alive()const->bool{return!(flags_&1);}
	inline constexpr auto bounding_radius()const->Scale{return bounding_radius_;}

	inline auto forward_vector()->Vector{
		refresh_Mmw_if_invalid();
		// negated because of object defintion uses 'forward' y negative
		return Mmw_.axis_y().negate().normalize();
	}
	
	// returns false if object died
	virtual auto update()->bool{return true;}
	
	virtual auto draw(Bitmap8b&dsp)->void{
		refresh_wld_points();

		if(enable::draw_dots){
			const Point*pt=points_world_;
			for(PointIx i=0;i<def_.points_size;i++){
				dsp.draw_dot(*pt,0xe); // yellow dot
				pt++;
			}
		}

		if(enable::draw_polygons){
			if(def_.indexes_size==1){
				// special case when definition is a point
				dsp.draw_dot(points_world_[0],color_);
			}else{
				dsp.draw_polygon(points_world_,def_.indexes_size,def_.indexes,color_);
			}
		}

		if(enable::draw_bounding_circle){
			dsp.draw_bounding_circle(phy_const().position,bounding_radius());
		}

		if(enable::draw_normals){
			if(normals_world_){ // check if there are any normals defined
				const Point*nml=normals_world_;
				for(PointIx i=0;i<def_.indexes_size;i++){
					Vector v=*nml;
					v.normalize().scale(3);
					Point p=points_world_[def_.indexes[i]];
					Vector v1{p+v};
					dsp.draw_dot(v1,0xf); // white dot
					nml++;
				}
			}
		}
	}

	// returns false if object has died
	virtual auto on_collision(Object&other)->bool{return true;}

private:
	// set to false at 'add_deleted'
	inline constexpr auto flag_as_dead()->void{flags_|=1;}
	inline constexpr auto world_points_need_update()const->bool{return flags_&2;}
	inline constexpr auto set_world_points_need_update(const bool b)->void{
		if(b){
			flags_|=2;
		}else{
			flags_&=Flags8b(~2);
		}
	}
	constexpr auto refresh_wld_points()->void{
		refresh_Mmw_if_invalid();

		if(!world_points_need_update()){
			return;
		}

		if(metrics::enabled){
			metrics::matrix_set_transforms++;
		}

		// matrix has been updated, update cached points
		Mmw_.transform(def_.points,points_world_,def_.points_size);

		if(def_.normals){ // check if there are any meaningful normals
			Mmw_.rotate(def_.normals,normals_world_,def_.indexes_size);
		}
	
		set_world_points_need_update(false);
	}
	constexpr auto refresh_Mmw_if_invalid()->void{
		if(phy_->angle==Mmw_angle_ && 
		   phy_->position==Mmw_position_ && 
		   scale_==Mmw_scale_
		){
			return;
		}

		set_world_points_need_update(true);

		Mmw_.set_transform(scale_,phy_->angle,phy_->position);
		Mmw_scale_=scale_;
		Mmw_angle_=phy_->angle;
		Mmw_position_=phy_->position;
	}

	//----------------------------------------------------------------
	//-- statics
	//----------------------------------------------------------------
	inline static Object**ls_all{}; // list of pointers to allocated objects
	inline static Object**ls_all_pos{}; // next free slot
	inline static Object**ls_all_end{}; // end of list (1 past last)
	inline static Object**ls_deleted{}; // list of pointers to deleted objects
	inline static Object**ls_deleted_pos{}; // next free slot
	inline static Object**ls_deleted_end{}; // end of list (1 past last)

	inline static uint64 timer_tick{}; // current time in osca ticks
	inline static uint64 timer_tick_prv{}; // previous frame time
	inline static uint64 fps_timer_tick_prv{}; // previous calculation tick
	inline static Count fps_frame_counter{}; // frames rendered during this interval

public:
	inline static TimeSec time{}; // time in seconds with decimals
	inline static TimeStepSec dt{}; // time step for this frame
	inline static Count fps{}; // this interval frames per second

	static auto init_statics()->void{
		ls_all=new Object*[objects_size_max];
		ls_all_pos=ls_all;
		ls_all_end=ls_all+objects_size_max;

		ls_deleted=new Object*[objects_size_max];
		ls_deleted_pos=ls_deleted;
		ls_deleted_end=ls_deleted+objects_size_max;

		fps_timer_tick_prv=timer_tick=timer_tick_prv=osca_tick;
		time=TimeSec(timer_tick)*TimeSec(osca_tick_per_sec);
	}
	static auto tick()->void{
		metrics::reset();

		// time
		timer_tick_prv=timer_tick;
		timer_tick=osca_tick;
		dt=TimeStepSec(timer_tick-timer_tick_prv)*osca_tick_per_sec;
		time=TimeSec(timer_tick)*TimeSec(osca_tick_per_sec);

		// fps calculations
		fps_frame_counter++;
		const TimeStepSec fps_dt=TimeStepSec(timer_tick-fps_timer_tick_prv)*osca_tick_per_sec;
		if(fps_dt>1){ // recalculate fps every second
			fps=Count(TimeStepSec(fps_frame_counter)/fps_dt);
			fps_frame_counter=0;
			fps_timer_tick_prv=timer_tick;
		}

		// draw and update
		draw_all(vga13h.bmp());
		PhysicsState::update_all(dt);
		update_all();
		check_collisions();
		commit_deleted();
	}
	static auto allocated_objects_count()->uint32{
		return uint32(ls_all_pos-ls_all);
	}
private:
	static auto add_deleted(Object*obj)->void{
		if(!obj->is_alive()){ // debugging (can be removed)
			err.p("Object::add_deleted:1");
			osca_hang();
		}

		obj->flag_as_dead();
		if(ls_deleted_pos==ls_deleted_end){
			err.p("Object::add_deleted:2");
			osca_hang();
		}
		*ls_deleted_pos=obj;
		ls_deleted_pos++;
	}
	static auto commit_deleted()->void{
		for(Object**it=ls_deleted;it<ls_deleted_pos;++it){
			delete *it;
			*it=nullptr; // debugging (can be removed)
		}
		ls_deleted_pos=ls_deleted;
	}
	static auto update_all()->void{
		for(Object**it=ls_all;it<ls_all_pos;++it){
			Object*o=*it;
			if(!o->update()){
				add_deleted(o);
			}
		}
	}
	static auto draw_all(Bitmap8b&dsp)->void{
		for(Object**it=ls_all;it<ls_all_pos;++it){
			Object*o=*it;
			o->draw(dsp);
		}
	}
	static auto check_collisions()->void{
		for(Object**it1=ls_all;it1<ls_all_pos-1;++it1){
			for(Object**it2=it1+1;it2<ls_all_pos;++it2){
				Object*o1=*it1;
				Object*o2=*it2;

				// check if objects are interested in collision check
				const bool o1_check_collision_with_o2=
				           o1->type_bits_collision_mask_&o2->type_bits_ &&
				           o1->is_alive();
				const bool o2_check_collision_with_o1=
				           o2->type_bits_collision_mask_&o1->type_bits_ &&
						   o2->is_alive();
				
				if(!o1_check_collision_with_o2 && !o2_check_collision_with_o1){
					continue;
				}

				if(metrics::enabled){
					metrics::collisions_checks++;
				}

				if(!Object::are_bounding_circles_in_collision(*o1,*o2)){
					continue;
				}

				if(metrics::enabled){
					metrics::collisions_checks_bounding_shapes++;
				}

				// refresh world coordinates
				o1->refresh_wld_points();
				o2->refresh_wld_points();

				// check if any o1 points in o2 bounding shape or 
				// any o2 points in o1 bounding shape
				if(!Object::objects_are_in_collision(*o1,*o2) && 
				   !Object::objects_are_in_collision(*o2,*o1)
				){
					continue;
				}

				if(o1_check_collision_with_o2){
					// o1 type wants to handle collisions with o2 type
					if(!o1->on_collision(*o2)){
						add_deleted(o1);
					}
				}
				if(o2_check_collision_with_o1){
					// o2 type wants to handle collisions with o1 type
					if(!o2->on_collision(*o1)){
						add_deleted(o2);
					}
				}
			}
		}
	}
	static auto are_bounding_circles_in_collision(Object&o1,Object&o2)->bool{
		const Scalar r1=o1.bounding_radius();
		const Scalar r2=o2.bounding_radius();
		
		const Point p1=o1.phy_const().position;
		const Point p2=o2.phy_const().position;

		// check if: sqrt(dx*dx+dy*dy)<=r1+r2
		//                dx*dx+dy*dy <=(r1+r2)²
		const Real dist_check=r1+r2;
		const Real dist2_check=dist_check*dist_check; // (r1+r2)²
		const Vector v=p2-p1; // dx,dy
		const Real dist2=v.dot(v);
		return dist2<=dist2_check;
	}
	// checks if any o1 bounding points are in o2 bounding shape
	static auto objects_are_in_collision(const Object&o1,const Object&o2)->bool{
		// for each point in 'o1' check if behind every normal of 'o2'
		// if behind every normal then within the convex bounding shape thus collision

		// if o2 has no bounding shape (at least 3 points) return false
		if(!o2.def_.normals){
			return false;
		}

		// for each point in 'o1' bounding shape
		const PointIx*ix=o1.def_.indexes; // bounding points indexes of 'o1'
		PointIx n=o1.def_.indexes_size;
		while(n--){
			const Point&p1=o1.points_world_[*ix];
			if(is_point_in_bounding_shape(p1,o2)){
				return true;
			}
			ix++;
		}
		return false;
	}
	inline static auto is_point_in_bounding_shape(const Point&p0,const Object&o)->bool{
		const PointIx*ix=o.def_.indexes; // bounding points indexes
		const Vector*nml=o.normals_world_; // normals
		PointIx n=o.def_.indexes_size;
		while(n--){
			const Vector&p{o.points_world_[*ix]};
			ix++;

			if(enable::draw_collision_check){
				vga13h.bmp().draw_dot(p,5);
			}
			// vector from point on line to point to check
			const Vector v=p0-p;
			if(v.dot(*nml)>0){
				// p is 'in front' of line, cannot be collision
				return false;
			}

			nml++;

			if(enable::draw_collision_check){
				vga13h.bmp().draw_dot(p,0xe);
			}
		}
		return true;
	}
};

} // end namespace osca
