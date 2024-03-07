#pragma once

namespace osca{

class ObjectDef final{
public:
	PointIx npts{0}; // number of points in pts // ? implement span
	PointIx nbnd{0}; // number of indexes in bnd // ? implement span
	Point*pts{nullptr}; // array of points used for rendering and bounding shape
	PointIx*bnd{nullptr}; // array of indexes in pts that defines the bounding shape as a convex polygon CCW
	Vector*nmls{nullptr}; // array of normals to the lines defined by bnd (calculated by init_normals())
//	~ObjectDef(){
//		if(pts)
//			delete[]pts;
//		if(bnd)
//			delete[]bnd;
//		if(nmls)
//			delete[]nmls;
//	}
	// destructor should not happen and generates calls to __cxa_atexit __dso_handle
	void operator delete(void*)=delete;

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
	static Count matrix_set_transforms{0};
	static Count collisions_checks{0};
	static Count collisions_checks_bounding_shapes{0};
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
	inline static Object*deleted[nobjects_max]; // ? todo improve with lesser memory footprint
	inline static int deleted_ix{0};
public:
//	constexpr static Real sec_per_tick{1/Real(18.2)}; // the default 18.2 Hz clock
	constexpr static Real sec_per_tick{Real(1)/Real(1024)}; // the 1024 Hz clock
	inline static TimeSec time{0};
	inline static TimeSec time_dt{0};
	inline static TimeSec time_prv{0};
	inline static Count fps_frame_counter{0};
	inline static TimeSec fps_time_prv{0};
	inline static Count fps{0};

	static auto init_statics()->void{
		fps_time_prv=time_prv=time=TimeSec(osca_tmr_lo)*sec_per_tick; // ? not using the high bits can be problem
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
	Point pos{0,0};
	Velocity vel{0,0}; // velocity per sec
	Acceleration acc{0,0}; // acceleration per sec
	AngleRad agl{0};
	AngularVelocity dagl{0}; // angular velocity per sec
	Object*obj{nullptr}; // pointer to the object to which this physics state belongs to
	                     // note. circular reference
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
	inline auto update()->void{
		vel.inc_by(acc,World::time_dt);
		pos.inc_by(vel,World::time_dt);
		agl+=dagl*World::time_dt;
	}

	//-----------------------------------------------------------
	//-----------------------------------------------------------
	//-----------------------------------------------------------

	inline static PhysicsState*mem_start{nullptr};
	inline static PhysicsState*next_free{nullptr};
	inline static PhysicsState*mem_end{nullptr};
	static auto init_statics()->void{
		mem_start=new PhysicsState[World::nobjects_max];
		next_free=mem_start;
		mem_end=reinterpret_cast<PhysicsState*>(mem_start+World::nobjects_max);
	}
	static auto alloc()->PhysicsState*{
		// check buffer overrun
		if(next_free==mem_end){
			err.p("PhysicsState:e1");
			osca_hang();
		}
		PhysicsState*p=next_free;
		next_free++;
		return p;
	}
	// returns reference to object that has a new address for physics state
	// this function works with ~Object() to relocate physics state
	static auto free(PhysicsState*phy)->Object&{
		// decrement next_free to point to last state in heap
		// copy last state to the freed area
		// return pointer to object that has had it's phy_ moved

		// check buffer underflow
//		if(next_free==mem_start){
//			out.printer().p("PhysicsState:e2");
//		}
		next_free--;
		Object&o=*next_free->obj;
		*phy=*next_free;
		pz_memset(next_free,3,sizeof(PhysicsState)); // ? debugging
		return o;
	}
	static auto update_all()->void{
		PhysicsState*ptr=mem_start;
		while(ptr<next_free){
			ptr->update();
			ptr++;
		}
	}
	static auto clear_buffer(char b=0)->void{
		const Address from=Address(mem_start);
		const SizeBytes n=SizeBytes(mem_end)-SizeBytes(mem_start); // ? may break if pointer is bigger than 2G
		pz_memset(from,b,n);
	}
};

namespace enable{
	constexpr static bool draw_dots{true};
	constexpr static bool draw_polygons{true};
	constexpr static bool draw_normals{true};
	constexpr static bool draw_collision_check{false};
	constexpr static bool draw_bounding_circle{true};
}

using SlotIx=short; // index in Object::freeSlots[]
// info that together with ~Object maintains usedSlots[]
struct SlotInfo{
	Object**oix{nullptr}; // pointer to element in Object::all[]
	Object*obj{nullptr}; // object owning this slot
};

using TypeBits=unsigned; // used by Object to declare 'type' as a bit and interests in collision with other types.
using Bits8=unsigned char;
using Scalar=Real;
constexpr Scale sqrt_of_2=Real(1.414213562);
class Object{
	friend World;
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
	SlotIx used_ix_{}; // index in used_ixes array. used at new and delete
	Color8b color_{};
	Bits8 bits_{}; // bit 1: is not alive
	               // bit 2: pts_wls_ don't need update
public:
	constexpr Object()=delete;
	constexpr Object(const Object&)=delete; // copy constructor
	constexpr Object&operator=(const Object&)=delete; // copy assignment
	constexpr Object(Object&&)=delete; // move constructor
	constexpr Object&operator=(Object&&)=delete; // move assignment
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
		if(free_ixes_i==0){
			err.p("Object:e1");
			osca_hang();
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
	// called only by 'World' at 'commit_deleted()'
	virtual~Object(){
		// free returns a pointer to the object that has had it's
		// physics state moved to the newly freed physics location.
		// set the pointer of that object's phy to the freed one
		PhysicsState::free(this->phy_).phy_=phy_;

		// get slot info for this object
		SlotInfo this_slot=used_ixes[used_ix_]; // ? this lookup can be optimized with a **Object in a field. speed vs space
		*this_slot.oix=nullptr; // ? this is unnecessary
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
		if(nmls_wld_)
			delete[]nmls_wld_;
	}
	inline constexpr auto type_bits()const->TypeBits{return tb_;}
	inline constexpr auto type_bits_collision_mask()const->TypeBits{return tb_col_msk_;}
	inline constexpr auto phy()->PhysicsState&{return*phy_;}
	// returns physics state as const (read only)
	inline constexpr auto phy_ro()const->const PhysicsState&{return*phy_;}
	inline constexpr auto scale()const->Scale{return scl_;}
	inline constexpr auto set_scale(const Scale s)->void{scl_=s;}
	inline constexpr auto def()const->const ObjectDef&{return def_;}
	auto forward_vector()->Vector{
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
			dsp.draw_bounding_circle(phy_ro().pos,bounding_radius());
		}
	}

	// returns false if object is to be deleted
	virtual auto on_collision(Object&other)->bool{return true;}

	inline constexpr auto is_alive()const->bool{return!(bits_&1);}

	inline constexpr auto bounding_radius()const->Scalar{return br_;}

private:
	// set to false by 'World' add 'deleted_add' and used to avoid deleting
	// same object more than once
	inline constexpr auto set_is_alive(const bool v)->void{
		if(v){ // alive bit is 0
			bits_&=~1;
		}else{ // not alive bit is 1
			bits_|=1;
		}
	}

	inline constexpr auto is_wld_pts_need_update()const->bool{return!(bits_&2);}
	inline constexpr auto set_wld_pts_need_update(const bool v)->void{
		if(v){ // refresh_wld_pts bit is 0
			bits_&=~2;
		}else{ // refresh_wld_pts bit is 1
			bits_|=2;
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
	inline static Object*all[World::nobjects_max]; // array of pointers to allocated objects
	inline static Object**free_ixes[World::nobjects_max]; // free indexes in all[]
	inline static SlotIx free_ixes_i{0}; // index in freeSlots[] of next free slot
	inline static SlotInfo used_ixes[World::nobjects_max]; // used indexes in all[]
	inline static SlotIx used_ixes_i{0}; // index in freeSlots[] of next free slot
public:
	static auto free_slots_count()->SlotIx{return free_ixes_i;}
	static auto used_slots_count()->SlotIx{return used_ixes_i;}
//	static inline auto hasFreeSlot()->bool{return free_ixes_i!=0;}
	static auto init_statics()->void{
		const SlotIx n=sizeof(free_ixes)/sizeof(Object**);
		for(SlotIx i=0;i<n;i++){
			free_ixes[i]=&all[i];
		}
		free_ixes_i=World::nobjects_max-1;
	}
	static auto update_all()->void{
		for(SlotIx i=0;i<used_ixes_i;i++){
			Object*o=object_for_used_slot(i);
			if(!o->update()){
				World::deleted_add(o);
			}
		}
	}
	static auto draw_all(Bitmap8b&dsp)->void{
		for(SlotIx i=0;i<used_ixes_i;i++){
			Object*o=object_for_used_slot(i);
			o->draw(dsp);
		}
	}
	static auto check_collisions()->void{
		for(SlotIx i=0;i<used_ixes_i-1;i++){
			for(SlotIx j=i+1;j<used_ixes_i;j++){
				Object*o1=used_ixes[i].obj;
				Object*o2=used_ixes[j].obj;
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
	static inline auto object_for_used_slot(const SlotIx i)->Object*{
		Object*o=used_ixes[i].obj;
//		if(!o){
//			err.p("null-pointer-exception [e1]");
//			osca_hang();
//		}
		return o;
	}

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

auto World::deleted_add(Object*o)->void{
	if(!o->is_alive()) // check if object already deleted
		return;
	o->set_is_alive(false);
	deleted[deleted_ix]=o;
	deleted_ix++;
}
auto World::commit_deleted()->void{
	for(int i=0;i<deleted_ix;i++){
		delete deleted[i];
	}
	deleted_ix=0;
}

auto World::tick()->void{
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

}// end namespace
