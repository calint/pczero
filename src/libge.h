#pragma once

//
// osca game engine library
//

namespace osca{

class ObjectDef final{
public:
	PointIx npts{}; // number of points in pts // ? implement span
	PointIx nbnd{}; // number of indexes in bnd // ? implement span
	Point*pts{}; // array of points used for rendering and bounding shape
	PointIx*bnd{}; // array of indexes in pts that defines the bounding shape as a convex polygon CCW
	Vector*nmls{}; // array of normals to the lines defined by bnd (calculated by init_normals())
	
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

using Time=Real;
using TimeSec=Time;

// update_all() and check_collisions() generate lists of objects to be deleted.
// the delete happens when deleted_commit() is called

class World{
public:
	constexpr static Size nobjects_max{256}; // maximum number of objects
private:
	inline static Object**ls_deleted{}; // list of pointers to deleted objects
	inline static Object**ls_deleted_pos{}; // next free slot
	inline static Object**ls_deleted_end{}; // end of list
public:
	constexpr static Real sec_per_tick{Real(1)/Real(1024)}; // the 1024 Hz clock
	inline static TimeSec time{};
	inline static TimeSec time_dt{};
	inline static TimeSec time_prv{};
	inline static Count fps_frame_counter{};
	inline static TimeSec fps_time_prv{};
	inline static Count fps{};

	static auto init_statics()->void{
		fps_time_prv=time_prv=time=TimeSec(osca_tmr_lo)*sec_per_tick; // ? not using the high bits can be problem
		// initiate the deleted list members
		ls_deleted=new Object*[nobjects_max];
		ls_deleted_pos=ls_deleted;
		ls_deleted_end=ls_deleted+nobjects_max;
	}
	static auto tick()->void;
	static auto deleted_add(Object*o)->void;
	static auto commit_deleted()->void;
};

// physics states are kept in their own buffer for better CPU cache utilization at update
using Velocity=Vector;
using Acceleration=Vector;
using AngularVelocity=AngleRad;

class PhysicsState final{
public:
	Point pos{}; // position
	Velocity vel{}; // velocity per second
	Acceleration acc{}; // acceleration per second
	AngleRad agl{}; // angle
	AngularVelocity dagl{}; // angular velocity per second
	Object*obj{}; // pointer to the object that owns this physics state
	              // note. circular reference
	inline auto update()->void{
		vel.inc_by(acc,World::time_dt);
		pos.inc_by(vel,World::time_dt);
		agl+=dagl*World::time_dt;
	}

	//-----------------------------------------------------------
	//-----------------------------------------------------------
	//-----------------------------------------------------------

	inline static PhysicsState*ls_all{};
	inline static PhysicsState*ls_all_pos{};
	inline static PhysicsState*ls_all_end{};
	static auto init_statics()->void{
		ls_all=new PhysicsState[World::nobjects_max];
		ls_all_pos=ls_all;
		ls_all_end=ls_all+World::nobjects_max;
	}
	static auto alloc()->PhysicsState*{
		// check buffer overrun
		if(ls_all_pos==ls_all_end){
			err.p("PhysicsState:e1");
			osca_hang();
		}
		PhysicsState*p=ls_all_pos;
		ls_all_pos++;
		return p;
	}
	// returns reference to object that has a new address for physics state
	// this function works with ~Object() to relocate physics state
	static auto free(PhysicsState*phy)->Object&{
		ls_all_pos--;
		Object&o=*(ls_all_pos->obj);
		*phy=*ls_all_pos;
		pz_memset(ls_all_pos,3,sizeof(PhysicsState)); // debugging (can be removed)
		return o;
	}
	static auto update_all()->void{
		for(PhysicsState*it=ls_all;it<ls_all_pos;++it){
			it->update();
		}
	}
	static auto clear_buffer(char b=0)->void{
		const Address from=Address(ls_all);
		const SizeBytes n=SizeBytes(ls_all_end)-SizeBytes(ls_all); // ? may break if pointer is bigger than 2G
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

using TypeBits=unsigned; // used by Object to declare 'type' as a bit and interests in collision with other types.
using Bits8=unsigned char;
using Scalar=Real;

constexpr Scale sqrt_of_2=Real(1.414213562);

class Object{
	friend World;
	Object**ls_all_pos_{}; // pointer to element in 'all' list containing pointer to this object
	TypeBits tb_{}; // object type that is usually a bit (32 object types supported)
	TypeBits tb_col_msk_{}; // bits used to bitwise 'and' with other object's type_bits and if true then collision detection is done
	PhysicsState*phy_{}; // kept in own buffer of states for better CPU cache utilization at update
	                   // may change between frames (when objects are deleted)
	Scale scl_{}; // scale that is used in transform from model to world coordinates
	const ObjectDef&def_; // contains the model definition
	Point*pts_wld_{}; // transformed model to world points cache
	Vector*nmls_wld_{}; // normals of boundingunsigned shape rotated to the world coordinates (not normalized if scale!=1)
	Matrix Mmw_{}; // model to world transform
	Point Mmw_pos_{}; // current position used in transform matrix
	AngleRad Mmw_agl_{}; // current angle used in transform matrix
	Scale Mmw_scl_{};  // current scale used in transform matrix
	Scalar br_{}; // bounding radius scl_*sqrt(2)
	Color8b color_{}; // shape color
	Bits8 flags_{}; // bit 1: is not alive
	               // bit 2: pts_wls_ don't need update
	char padding[2]{};
public:
	Object(const TypeBits tb,const TypeBits tb_col_msk,const ObjectDef&def,const Scale scl,const Scalar bounding_radius,const Point&pos,const AngleRad rad,const Color8b color):
		tb_{tb},
		tb_col_msk_{tb_col_msk},
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
		phy_->agl=rad;
		phy_->obj=this;

		// allocate index in all[] from free slots
		if(ls_all_pos==ls_all_end){
			err.p("Object:e1");
			osca_hang();
		}
		
		*ls_all_pos=this;
		ls_all_pos_=ls_all_pos;
		ls_all_pos++;
	}
	// called only by 'World' at 'commit_deleted()'
	virtual~Object(){
		// free returns a pointer to the object that has had it's
		// physics state moved to the newly freed physics location.
		// set the pointer of that object's phy to the freed one
		PhysicsState::free(this->phy_).phy_=phy_;

		ls_all_pos--;
		// move the last object pointer in list to the slot this object had
		*ls_all_pos_=*ls_all_pos;
		*ls_all_pos=nullptr; // debugging (can be removed)
		// adjust pointer to the slot in 'all' list
		(*ls_all_pos_)->ls_all_pos_=ls_all_pos_;
		// delete cached points
		delete[]pts_wld_;
		if(nmls_wld_)
			delete[]nmls_wld_;
	}

	constexpr Object()=delete;
	constexpr Object(const Object&)=delete; // copy constructor
	constexpr Object&operator=(const Object&)=delete; // copy assignment
	constexpr Object(Object&&)=delete; // move constructor
	constexpr Object&operator=(Object&&)=delete; // move assignment

	inline constexpr auto type_bits()const->TypeBits{return tb_;}
	inline constexpr auto type_bits_collision_mask()const->TypeBits{return tb_col_msk_;}
	inline constexpr auto phy()->PhysicsState&{return*phy_;}
	inline constexpr auto phy_const()const->const PhysicsState&{return*phy_;}
	inline constexpr auto scale()const->Scale{return scl_;}
	inline constexpr auto set_scale(const Scale s)->void{scl_=s;}
	inline constexpr auto def()const->const ObjectDef&{return def_;}
	inline constexpr auto is_alive()const->bool{return!(flags_&1);}
	inline constexpr auto bounding_radius()const->Scalar{return br_;}
	inline auto forward_vector()->Vector{
		refresh_Mmw_if_invalid();
		return Mmw_.axis_y().negate().normalize(); // ? not negated (if positive y is up)
	}
	
	// returns false if object is to be deleted
	virtual auto update()->bool{return true;}
	
	virtual auto draw(Bitmap8b&dsp)->void{
		refresh_wld_points();
		if(enable::draw_dots){
			const Point*pt=pts_wld_;
			for(PointIx i=0;i<def_.npts;i++){
//				dot(dsp,pt->x,pt->y,color_);
				dsp.draw_dot(*pt,0xe); // yellow dot
				pt++;
			}
		}
		if(enable::draw_polygons){
			dsp.draw_polygon(pts_wld_,def_.nbnd,def_.bnd,color_);
		}
		if(enable::draw_normals){
			if(nmls_wld_){ // check if there are any normals defined
				const Point*nml=nmls_wld_;
				for(PointIx i=0;i<def_.nbnd;i++){
					Vector v=*nml;
					v.normalize().scale(3);
					Point p=pts_wld_[def_.bnd[i]];
					Vector v1{p.x+nml->x,p.y+nml->y};
					dsp.draw_dot(v1,0xf);
					nml++;
				}
			}
		}
		if(enable::draw_bounding_circle){
			dsp.draw_bounding_circle(phy_const().pos,bounding_radius());
		}
	}

	// returns false if object is to be deleted
	virtual auto on_collision(Object&other)->bool{return true;}

private:
	// set to false by 'World' add 'deleted_add' and used to avoid deleting
	// same object more than once
	inline constexpr auto set_is_alive(const bool b)->void{
		if(b){ // alive bit is 0
			flags_&=Bits8(~1);
		}else{ // not alive bit is 1
			flags_|=1;
		}
	}

	inline constexpr auto is_wld_pts_need_update()const->bool{return!(flags_&2);}
	inline constexpr auto set_wld_pts_need_update(const bool b)->void{
		if(b){ // refresh_wld_pts bit is 0
			flags_&=Bits8(~2);
		}else{ // refresh_wld_pts bit is 1
			flags_|=2;
		}
	}

	constexpr auto refresh_wld_points()->void{
		refresh_Mmw_if_invalid();
		if(!is_wld_pts_need_update())
			return;
		if(metrics::enabled)
			metrics::matrix_set_transforms++;
		// matrix has been updated, update cached points
		Mmw_.transform(def_.pts,pts_wld_,def_.npts);
		if(def_.nmls) // check if there are any meaningful normals
			Mmw_.rotate(def_.nmls,nmls_wld_,def_.nbnd);
		set_wld_pts_need_update(false);
	}
	constexpr auto refresh_Mmw_if_invalid()->void{
		if(phy().agl==Mmw_agl_&&phy().pos==Mmw_pos_&&scl_==Mmw_scl_)
			return;
		set_wld_pts_need_update(true);
		Mmw_.set_transform(scl_,phy().agl,phy().pos);
		Mmw_scl_=scl_;
		Mmw_agl_=phy().agl;
		Mmw_pos_=phy().pos;
	}

	//----------------------------------------------------------------
	// statics
	//----------------------------------------------------------------
	inline static Object**ls_all{}; // list of pointers to allocated objects
	inline static Object**ls_all_pos{}; // next free object slot
	inline static Object**ls_all_end{}; // end of list of pointers to allocated objects
public:
//	static inline auto hasFreeSlot()->bool{return free_ixes_i!=0;}
	static auto init_statics()->void{
		ls_all=new Object*[World::nobjects_max];
		ls_all_pos=ls_all;
		ls_all_end=ls_all+World::nobjects_max;
	}
	static auto allocated_objects_count()->unsigned{
		return unsigned(ls_all_pos-ls_all);
	}
	static auto update_all()->void{
		for(Object**it=ls_all;it<ls_all_pos;++it){
			Object*o=*it;
			if(!o->update()){
				World::deleted_add(o);
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
				const bool o1_check_col_with_o2=o1->tb_col_msk_&o2->tb_;
				const bool o2_check_col_with_o1=o2->tb_col_msk_&o1->tb_;
				if(!o1_check_col_with_o2&&!o2_check_col_with_o1)
					continue;
//				out.p("chk ").p_hex_8b(static_cast<unsigned char>(tb1)).p(' ').p_hex_8b(static_cast<unsigned char>(tb2)).p(' ');
				if(metrics::enabled)
					metrics::collisions_checks++;
				if(!Object::is_bounding_circles_in_collision(*o1,*o2))
					continue;
				if(metrics::enabled)
					metrics::collisions_checks_bounding_shapes++;

				// refresh world coordinates
				o1->refresh_wld_points();
				o2->refresh_wld_points();

				// check if o1 points in o2 bounding shape or o2 points in o1 bounding shape
				if(!Object::is_in_collision(*o1,*o2)&&!Object::is_in_collision(*o2,*o1))
					continue;

				if(o1_check_col_with_o2){
					// o1 type wants to handle collisions with o2 type
					if(!o1->on_collision(*o2)){
						World::deleted_add(o1);
					}
				}
				if(o2_check_col_with_o1){
					// o2 type wants to handle collisions with o1 type
					if(!o2->on_collision(*o1)){
						World::deleted_add(o2);
					}
				}
			}
		}
	}

private:
	static auto is_bounding_circles_in_collision(Object&o1,Object&o2)->bool{
		const Scalar r1=o1.bounding_radius();
		const Scalar r2=o2.bounding_radius();
		const Point p1=o1.phy().pos;
		const Point p2=o2.phy().pos;

		// check if: sqrt(dx*dx+dy*dy)<=r1+r2
		//                dx*dx+dy*dy <=(r1+r2)²
		const Real dist_check=r1+r2;
		const Real dist2_check=dist_check*dist_check; // (r1+r2)²
		Vector v{p2-p1}; // dx,dy
		v.x*=v.x;
		v.y*=v.y;
		const Real dist2=v.x+v.y;
		if(dist2>dist2_check)
			return false;
		return true;
	}
	// checks if any o1 bounding points are in o2 bounding shape
	static auto is_in_collision(const Object&o1,const Object&o2)->bool{
		// for each point in o1 check if behind every normal of o2
		// if behind every normal then within the convex bounding shape thus collision

		// if o2 has no bounding shape (at least 3 points) return false
		if(!o2.def_.nmls) // ? check if points equal? with floats?
			return false;

		// for each point in o1 bounding shape
		const PointIx nbnd=o1.def_.nbnd;
		const PointIx*bndptr=o1.def_.bnd; // bounding point index of o1
		for(PointIx i=0;i<nbnd;i++){
			// reference pts_pts_wld_[bnd[i]]
			const Point&p1=o1.pts_wld_[*bndptr];
			bndptr++;
			if(is_point_in_bounding_shape(p1,o2))
				return true;
		}
		return false;
	}
	inline static auto is_point_in_bounding_shape(const Point&p0,const Object&o)->bool{
		const PointIx nbnd{o.def_.nbnd};
		const PointIx*bnd_ix_ptr{o.def_.bnd}; // bounding point index
		const Vector*nml_ptr{o.nmls_wld_}; // normals
		for(PointIx j=0;j<nbnd;j++){
			const Vector&p{o.pts_wld_[*bnd_ix_ptr]};
			bnd_ix_ptr++;
			if(enable::draw_collision_check){
				vga13h.bmp().draw_dot(p,5);
			}
			const Vector v{p0-p}; // vector from point on line to point to check
			if(v.dot(*nml_ptr)>0){ // use abs(v)<0.0001f (example)?
				// p "in front" of line, cannot be collision
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

inline auto World::deleted_add(Object*o)->void{
	if(!o->is_alive()) // check if object already deleted
		return;
	o->set_is_alive(false);
	if(ls_deleted_pos==ls_deleted_end){
		err.p("World::deleted_add:1");
		osca_hang();
	}
	*ls_deleted_pos=o;
	ls_deleted_pos++;
}

inline auto World::commit_deleted()->void{
	for(Object**it=ls_deleted;it<ls_deleted_pos;++it){
		delete *it;
		*it=nullptr; // debugging (can be removed)
	}
	ls_deleted_pos=ls_deleted;
}

inline auto World::tick()->void{
	metrics::reset();

	time_prv=time;
	time=TimeSec(osca_tmr_lo)*sec_per_tick; // ? not using the high bits is a problem. fix
	time_dt=time-time_prv;
	if(time_dt<0){ // ? is that once every 4 billionth tick?
		time_dt=0;
	}
	// fps calculations
	fps_frame_counter++;
	const TimeSec dt=time-fps_time_prv;
	if(dt>1){
		fps=Count(TimeSec(fps_frame_counter)/dt);
		fps_frame_counter=0;
		fps_time_prv=time;
	}

	Object::draw_all(vga13h.bmp());
	PhysicsState::update_all();
	Object::update_all();
	Object::check_collisions();
	World::commit_deleted();
}

} // end namespace osca
