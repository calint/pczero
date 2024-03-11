#pragma once
// reviewed: 2024-03-09

//
// osca game engine library
//

namespace osca{

class ObjectDef final{
public:
	PointIx npts{}; // number of points in 'pts'
	PointIx nbnd{}; // number of indexes in 'bnd'
	Point*pts{}; // list of points used for rendering and bounding shape
	PointIx*bnd{}; // list of indexes in pts that defines the bounding shape as a convex polygon CCW
	Vector*nmls{}; // list of normals to the lines defined by 'bnd' (calculated by 'init_normals()')
	
	// destructor cannot happen because object life time is program lifetime
	// void operator delete(void*)=delete;

	constexpr auto init_normals()->void{
		if(nbnd<3){ // not enough points for a shape
			// don't define normals
			return;
		}
		nmls=new Vector[unsigned(nbnd)];
		const PointIx n=nbnd-1;
		for(PointIx i=0;i<n;i++){
			const Vector d{pts[bnd[i+1]]-pts[bnd[i]]};
			nmls[i]=d.normal().normalize();
		}
		const Vector d{pts[bnd[0]]-pts[bnd[nbnd-1]]};
		nmls[nbnd-1]=d.normal().normalize();
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
constexpr Size nobjects_max{256};

using Velocity=Vector;
using Acceleration=Vector;
using AngularVelocityRad=AngleRad;
using Time=Real;
using TimeSec=Time;
using TimeStep=Real;

// physics states are kept in their own buffer for better cpu cache utilization at update
class PhysicsState final{
public:
	Point pos{}; // position
	Velocity vel{}; // velocity per second
	Acceleration acc{}; // acceleration per second
	AngleRad agl{}; // angle in radians
	AngularVelocityRad dagl{}; // angular velocity per second
	Object*obj{}; // pointer to the object that owns this physics state
	              // note: circular reference
	inline auto update(const TimeSec dt)->void{
		vel.inc_by(acc,dt);
		pos.inc_by(vel,dt);
		agl+=dagl*dt;
	}

	//-----------------------------------------------------------
	//--- statics
	//-----------------------------------------------------------

	inline static PhysicsState*ls_all{};
	inline static PhysicsState*ls_all_pos{};
	inline static PhysicsState*ls_all_end{};
	
	static auto init_statics()->void{
		ls_all=new PhysicsState[nobjects_max];
		ls_all_pos=ls_all;
		ls_all_end=ls_all+nobjects_max;
	}
	static auto alloc()->PhysicsState*{
		// check buffer overrun
		if(ls_all_pos==ls_all_end){
			err.p("PhysicsState:e1");
			osca_hang();
		}
		PhysicsState*ptr=ls_all_pos;
		ls_all_pos++;
		return ptr;
	}
	// returns reference to the object that has a new address for physics state
	// this function works with '~Object()' to relocate physics state
	static auto free(PhysicsState*phy)->Object*{
		ls_all_pos--;
		Object*o=ls_all_pos->obj;
		*phy=*ls_all_pos;
		pz_memset(ls_all_pos,0x0f,sizeof(PhysicsState)); // debugging (can be removed)
		return o;
	}
	static auto update_all(const float dt_s)->void{
		for(PhysicsState*it=ls_all;it<ls_all_pos;++it){
			it->update(dt_s);
		}
	}
	static auto clear(unsigned char b=0)->void{
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

using TypeBits=unsigned; // used by 'Object' to declare 'type' as a bit and interests in collision with other types
using Flags8b=uint8;
using Scalar=Real;

constexpr Scale sqrt_of_2=Real(1.414213562);

class Object{
	Object**ls_all_pos_{}; // pointer to element in 'all' list containing pointer to this object
	TypeBits tb_{}; // object type that is usually a bit (32 object types supported)
	TypeBits tb_col_msk_{}; // bits used to bitwise 'and' with other object's 'tb_' and if true then collision detection is done
	PhysicsState*phy_{}; // kept in own buffer of states for better cpu cache utilization at update
	                     // may change between frames (when objects are deleted and 'phy_' relocated)
	Scale scl_{}; // scale that is used in model to world transform
	const ObjectDef&def_; // contains the model definition
	Point*pts_wld_{}; // transformed model to world points cache
	Vector*nmls_wld_{}; // normals of bounding shape rotated to the world coordinates (not normalized if scale!=1)
	Matrix Mmw_{}; // model to world transform
	Point Mmw_pos_{}; // position used in 'Mmw_'
	AngleRad Mmw_agl_{}; // angle used in 'Mmw_'
	Scale Mmw_scl_{};  // scale used in 'Mmw_'
	Scale br_{}; // bounding radius scl_*sqrt(2)
	Color8b color_{}; // shape color
	Flags8b flags_{}; // bit 1: is not alive
	                  // bit 2: 'pts_wld_' don't need update
	uint8 padding[2]{};
public:
	Object(
		const TypeBits tb,
		const TypeBits tb_collision_mask,
		const ObjectDef&def,
		const Scale scl,
		const Scalar bounding_radius,
		const Point&pos,
		const AngleRad agl,
		const Color8b color
	):
		tb_{tb},
		tb_col_msk_{tb_collision_mask},
		phy_{PhysicsState::alloc()},
		scl_{scl},
		def_{def},
		pts_wld_{new Point[unsigned(def.npts)]},
		 // def might be a dot or a line and not have normals -> no bounding shape
		nmls_wld_{def.nmls?new Vector[unsigned(def.nbnd)]:nullptr},
		br_{bounding_radius},
		color_{color}
	{
		// initiate physics state
		*phy_=PhysicsState{};
		phy_->pos=pos;
		phy_->agl=agl;
		phy_->obj=this;

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
		delete[]pts_wld_;
		delete[]nmls_wld_;
	}

	constexpr Object()=delete;
	constexpr Object(const Object&)=delete;
	constexpr Object&operator=(const Object&)=delete;
	constexpr Object(Object&&)=delete;
	constexpr Object&operator=(Object&&)=delete;

	inline constexpr auto type_bits()const->TypeBits{return tb_;}
	inline constexpr auto type_bits_collision_mask()const->TypeBits{return tb_col_msk_;}
	inline constexpr auto phy()->PhysicsState&{return*phy_;}
	inline constexpr auto phy_const()const->const PhysicsState&{return*phy_;}
	inline constexpr auto scale()const->Scale{return scl_;}
	inline constexpr auto set_scale(const Scale s)->void{scl_=s;}
	inline constexpr auto def()const->const ObjectDef&{return def_;}
	inline constexpr auto is_alive()const->bool{return!(flags_&1);}
	inline constexpr auto bounding_radius()const->Scale{return br_;}
	inline auto forward_vector()->Vector{
		refresh_Mmw_if_invalid();
		return Mmw_.axis_y().negate().normalize(); // not negated if positive y is up
	}
	
	// returns false if object died
	virtual auto update()->bool{return true;}
	
	virtual auto draw(Bitmap8b&dsp)->void{
		refresh_wld_points();

		if(enable::draw_dots){
			const Point*pt=pts_wld_;
			for(PointIx i=0;i<def_.npts;i++){
				dsp.draw_dot(*pt,0xe); // yellow dot
				pt++;
			}
		}

		if(enable::draw_polygons){
			dsp.draw_polygon(pts_wld_,def_.nbnd,def_.bnd,color_);
		}

		if(enable::draw_bounding_circle){
			dsp.draw_bounding_circle(phy_const().pos,bounding_radius());
		}

		if(enable::draw_normals){
			if(nmls_wld_){ // check if there are any normals defined
				const Point*nml=nmls_wld_;
				for(PointIx i=0;i<def_.nbnd;i++){
					Vector v=*nml;
					v.normalize().scale(3);
					Point p=pts_wld_[def_.bnd[i]];
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
	inline constexpr auto set_is_alive(const bool b)->void{
		if(b){ // alive bit is 0
			flags_&=Flags8b(~1);
		}else{ // not alive bit is 1
			flags_|=1;
		}
	}
	inline constexpr auto is_wld_pts_need_update()const->bool{
		return!(flags_&2);
	}
	inline constexpr auto set_wld_pts_need_update(const bool b)->void{
		if(b){
			flags_&=Flags8b(~2);
		}else{
			flags_|=2;
		}
	}
	constexpr auto refresh_wld_points()->void{
		refresh_Mmw_if_invalid();

		if(!is_wld_pts_need_update()){
			return;
		}

		if(metrics::enabled){
			metrics::matrix_set_transforms++;
		}

		// matrix has been updated, update cached points
		Mmw_.transform(def_.pts,pts_wld_,def_.npts);

		if(def_.nmls){ // check if there are any meaningful normals
			Mmw_.rotate(def_.nmls,nmls_wld_,def_.nbnd);
		}
	
		set_wld_pts_need_update(false);
	}
	constexpr auto refresh_Mmw_if_invalid()->void{
		if(phy().agl==Mmw_agl_ && phy().pos==Mmw_pos_ && scl_==Mmw_scl_){
			return;
		}

		set_wld_pts_need_update(true);

		Mmw_.set_transform(scl_,phy().agl,phy().pos);
		Mmw_scl_=scl_;
		Mmw_agl_=phy().agl;
		Mmw_pos_=phy().pos;
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

	inline static uint64 timer_tick{};
	inline static uint64 timer_tick_prv{};
	inline static uint64 fps_timer_tick_prv{};
	inline static Count fps_frame_counter{};

public:
	inline static TimeSec time{};
	inline static TimeSec dt{};
	inline static Count fps{};

	static auto init_statics()->void{
		ls_all=new Object*[nobjects_max];
		ls_all_pos=ls_all;
		ls_all_end=ls_all+nobjects_max;

		ls_deleted=new Object*[nobjects_max];
		ls_deleted_pos=ls_deleted;
		ls_deleted_end=ls_deleted+nobjects_max;

		fps_timer_tick_prv=timer_tick=timer_tick_prv=osca_tick;
		time=TimeSec(timer_tick)*osca_timer_sec_per_tick;
	}
	static auto tick()->void{
		metrics::reset();

		// time
		timer_tick_prv=timer_tick;
		timer_tick=osca_tick;
		time=TimeSec(timer_tick)*osca_timer_sec_per_tick;
		dt=TimeSec(timer_tick-timer_tick_prv)*osca_timer_sec_per_tick;

		// fps calculations
		fps_frame_counter++;
		const TimeSec fps_dt=TimeSec(timer_tick-fps_timer_tick_prv)*osca_timer_sec_per_tick;
		if(fps_dt>1){ // recalculate fps every second
			fps=Count(TimeSec(fps_frame_counter)/fps_dt);
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
	static auto allocated_objects_count()->unsigned{
		return unsigned(ls_all_pos-ls_all);
	}
private:
	static auto add_deleted(Object*obj)->void{
		if(!obj->is_alive()){ // debugging (can be removed)
			err.p("Object::add_deleted:1");
			osca_hang();
		}

		obj->set_is_alive(false);
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
				const bool o1_check_col_with_o2=o1->tb_col_msk_&o2->tb_ && o1->is_alive();
				const bool o2_check_col_with_o1=o2->tb_col_msk_&o1->tb_ && o2->is_alive();
				
				if(!o1_check_col_with_o2 && !o2_check_col_with_o1){
					continue;
				}

				if(metrics::enabled){
					metrics::collisions_checks++;
				}

				if(!Object::is_bounding_circles_in_collision(*o1,*o2)){
					continue;
				}

				if(metrics::enabled){
					metrics::collisions_checks_bounding_shapes++;
				}

				// refresh world coordinates
				o1->refresh_wld_points();
				o2->refresh_wld_points();

				// check if o1 points in o2 bounding shape or o2 points in o1 bounding shape
				if(!Object::is_in_collision(*o1,*o2) && !Object::is_in_collision(*o2,*o1)){
					continue;
				}

				if(o1_check_col_with_o2){
					// o1 type wants to handle collisions with o2 type
					if(!o1->on_collision(*o2)){
						add_deleted(o1);
					}
				}
				if(o2_check_col_with_o1){
					// o2 type wants to handle collisions with o1 type
					if(!o2->on_collision(*o1)){
						add_deleted(o2);
					}
				}
			}
		}
	}
	static auto is_bounding_circles_in_collision(Object&o1,Object&o2)->bool{
		const Scalar r1=o1.bounding_radius();
		const Scalar r2=o2.bounding_radius();
		
		const Point p1=o1.phy_const().pos;
		const Point p2=o2.phy_const().pos;

		// check if: sqrt(dx*dx+dy*dy)<=r1+r2
		//                dx*dx+dy*dy <=(r1+r2)²
		const Real dist_check=r1+r2;
		const Real dist2_check=dist_check*dist_check; // (r1+r2)²
		const Vector v=p2-p1; // dx,dy
		const Real dist2=v.dot(v);
		return dist2<=dist2_check;
	}
	// checks if any o1 bounding points are in o2 bounding shape
	static auto is_in_collision(const Object&o1,const Object&o2)->bool{
		// for each point in 'o1' check if behind every normal of 'o2'
		// if behind every normal then within the convex bounding shape thus collision

		// if o2 has no bounding shape (at least 3 points) return false
		if(!o2.def_.nmls){
			return false;
		}

		// for each point in 'o1' bounding shape
		const PointIx*bndptr=o1.def_.bnd; // bounding points indexes of 'o1'
		PointIx nbnd=o1.def_.nbnd;
		while(nbnd--){
			const Point&p1=o1.pts_wld_[*bndptr];
			if(is_point_in_bounding_shape(p1,o2)){
				return true;
			}
			bndptr++;
		}
		return false;
	}
	inline static auto is_point_in_bounding_shape(const Point&p0,const Object&o)->bool{
		const PointIx*bnd_ix_ptr=o.def_.bnd; // bounding points indexes
		const Vector*nml_ptr=o.nmls_wld_; // normals
		PointIx nbnd=o.def_.nbnd;
		while(nbnd--){
			const Vector&p{o.pts_wld_[*bnd_ix_ptr]};
			bnd_ix_ptr++;

			if(enable::draw_collision_check){
				vga13h.bmp().draw_dot(p,5);
			}

			const Vector v=p0-p; // vector from point on line to point to check
			if(v.dot(*nml_ptr)>0){
				// p is 'in front' of line, cannot be collision
				return false;
			}

			nml_ptr++;

			if(enable::draw_collision_check){
				vga13h.bmp().draw_dot(p,0xe);
			}
		}
		return true;
	}
};

} // end namespace osca
