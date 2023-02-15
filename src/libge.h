#pragma once
#include"lib2d.h"

namespace osca{

using Point=Vector; // a point in 2D
using PointIx=short; // index into a list of points

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
	inline static TimeSec fps_last_time{0};
	inline static Count fps{0};

	static auto init_statics()->void{
		time=TimeSec(osca_tmr_lo)*sec_per_tick;
		// set previous time to a reasonable value so that dt does not
		// become huge at first frame
		time_prv=time-sec_per_tick;
		fps_last_time=time;
	}
	static auto tick()->void;
	static auto deleted_add(Object*o)->void;
	static auto commit_deleted()->void;

	static constexpr auto draw_dot(const Point&p,const Color8b color)->void{
		const CoordPx xi=CoordPx(p.x);
		const CoordPx yi=CoordPx(p.y);
		const Bitmap8b bmp=vga13h.bmp();
		if(xi>bmp.dim().width())
			return;
		if(yi>bmp.dim().width())
			return;
		bmp.pointer_offset({xi,yi}).write(color);
	}
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
	inline static PhysicsState*mem_limit{nullptr};
	static auto init_statics()->void{
		mem_start=new PhysicsState[World::nobjects_max];
		next_free=mem_start;
		mem_limit=reinterpret_cast<PhysicsState*>(mem_start+World::nobjects_max);
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
	// this function works with ~Object() to relocate physics state
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
		const SizeBytes n=SizeBytes(mem_limit)-SizeBytes(mem_start); // ? may break if pointer is bigger than 2G
		pz_memset(from,b,n);
	}
};

namespace enable{
	constexpr static bool draw_dots{true};
	constexpr static bool draw_polygons{true};
	constexpr static bool draw_polygons_fill{false};
	constexpr static bool draw_polygons_edges{true};
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
constexpr Scale sqrt_of_2=Real(1.414213562);
class Object{
	friend World;
	TypeBits tb_; // object type that is usually a bit (32 object types supported)
	TypeBits colchk_tb_; // bits used to logical and with other object's type_bits and if true then collision detection is done
	PhysicsState*phy_; // kept in own buffer of states for better CPU cache utilization at update
	                   // may change between frames (when objects are deleted)
	Scale scl_; // scale that is used in transform from model to world coordinates
	const ObjectDef&def_; // contains the model definition
	Point*pts_wld_; // transformed model to world points cache
	Vector*nmls_wld_; // normals of boundingunsigned shape rotated to the world coordinates (not normalized if scale!=1)
	Matrix2D Mmw_; // model to world transform
	Point Mmw_pos_; // current position used in transform matrix
	AngleRad Mmw_agl_; // current angle used in transform matrix
	Scale Mmw_scl_;  // current scale used in transform matrix
	Scalar br_; // bounding radius scl_*sqrt(2)
	SlotIx used_ix_; // index in used_ixes array. used at new and delete
	Color8b color_;
	unsigned char bits_; // bit 1: is not alive
	                     // bit 2: pts_wls_ don't need update
public:
	constexpr Object()=delete;
	constexpr Object(const Object&)=delete; // copy constructor
	constexpr Object&operator=(const Object&)=delete; // copy assignment
	constexpr Object(Object&&)=delete; // move constructor
	constexpr Object&operator=(Object&&)=delete; // move assignment
	Object(const TypeBits tb,const TypeBits collision_check_tb,const ObjectDef&def,const Scale scl,const Scalar bounding_radius,const Point&pos,const AngleRad rad,const Color8b color):
		tb_{tb},
		colchk_tb_{collision_check_tb},
		phy_{PhysicsState::alloc()},
		scl_{scl},
		def_{def},
		pts_wld_{new Point[unsigned(def.npts)]},
		 // def might be a dot or a line and not have normals -> no bounding shape
		nmls_wld_{def.nmls?new Vector[unsigned(def.nbnd)]:nullptr},
		Mmw_{},
		Mmw_pos_{0,0},
		Mmw_agl_{0},
		Mmw_scl_{0},
		br_{bounding_radius},
		used_ix_{0},
		color_{color},
		bits_{0}
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
	inline constexpr auto type_bits()const->TypeBits{return tb_;}	// returns false if object is to be deleted
	inline constexpr auto type_bits_collision_mask()const->TypeBits{return colchk_tb_;}	// returns false if object is to be deleted
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
				World::draw_dot(*pt,0xe); // yellow dot
				pt++;
			}
		}
		if(enable::draw_polygons){
			draw_polygon(dsp, pts_wld_,def_.nbnd,def_.bnd,color_);
		}
		if(enable::draw_normals){
			if(nmls_wld_){ // check if there are any normals defined
				const Point*nml=nmls_wld_;
				for(PointIx i=0;i<def_.nbnd;i++){
					Vector v=*nml;
					v.normalize().scale(3);
					Point p=pts_wld_[def_.bnd[i]];
					Vector v1{p.x+nml->x,p.y+nml->y};
					World::draw_dot(v1,0xf);
					nml++;
				}
			}
		}
		if(enable::draw_bounding_circle){
			draw_bounding_circle(dsp,phy_ro().pos,bounding_radius());
		}
	}

	// returns false if object is to be deleted
	virtual auto on_collision(Object&other)->bool{return true;}

	inline constexpr auto is_alive()const->bool{return!(bits_&1);}

	inline constexpr auto bounding_radius()const->Scalar{return br_;}

private:
	// used by 'world' to avoid deleting same object more than once
	inline constexpr auto set_is_alive(const bool v)->void{
		if(v){ // alive bit is 0
			bits_&=0xff-1;
		}else{ // not alive bit is 1
			bits_|=1;
		}
	}

	inline constexpr auto is_wld_pts_need_update()const->bool{return!(bits_&2);}
	inline constexpr auto set_wld_pts_need_update(const bool v)->void{
		if(v){ // refresh_wld_pts bit is 0
			bits_&=0xff-2;
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
	// ? move rendering code to other class
	constexpr static auto draw_bounding_circle(Bitmap8b&dsp,const Point&p,const Scale r)->void{
		const Count segments=Count(5*r);
		AngleRad th=0;
		AngleRad dth=2*PI/AngleRad(segments);
		for(Count i=0;i<segments;i++){
			const Coord x=p.x+r*cos(th);
			const Coord y=p.y+r*sin(th);
			World::draw_dot({x,y},1);
			th+=dth;
		}
	}
	// ? move rendering code to other class
	constexpr static auto draw_polygon(Bitmap8b&dsp,const Point pts[],const PointIx npoly_ixs,const PointIx ix[],const Color8b color)->void{
//		const PointIx*pi=ix;
//		for(PointIx i=0;i<npoly_ixs;i++){
//			const Point2D&p=pts[*pi++];
//			dot(dsp,p.x,p.y,4);
//		}
		if(npoly_ixs<2){ // ? what if 0, 2 is a line
			World::draw_dot(pts[0],color);
			return;
		}
		PointIx topy_ix=0;
		const Point&first_point=pts[ix[0]];
		Coord topx=first_point.x;
		Coord topy=first_point.y;
		PointIx i=1;
		while(i<npoly_ixs){
			const Point&p=pts[ix[i]]; // ? use pointer
			const Coord y=p.y;
			if(y<topy){
				topy=y;
				topx=p.x;
				topy_ix=i;
			}
			i++;
		}
		PointIx ix_lft,ix_rht;
		ix_lft=ix_rht=topy_ix;
		Coord x_lft,x_rht;
		x_lft=x_rht=topx;
		bool adv_lft=true,adv_rht=true;
		Coord dxdy_lft,dxdy_rht;
		dxdy_lft=dxdy_rht=0;
		Coord x_nxt_lft=0;
		Coord y_nxt_lft=topy;
		Coord x_nxt_rht=0;
		Coord y_nxt_rht=topy;
		Coord dy_rht=0;
		Coord dy_lft=0;
		Coord y=topy;
//		CoordPx wi=CoordPx(dsp.dim().width());
		CoordPx wi=CoordPx(dsp.dim().width());
		CoordPx y_scr=CoordPx(y);
		Color8b*pline=static_cast<Color8b*>(dsp.data().address())+y_scr*wi;
		PointIx last_elem_ix=npoly_ixs-1;
		while(true){
			if(adv_lft){
				if(ix_lft==last_elem_ix){
					ix_lft=0;
				}else{
					ix_lft++;
				}
				x_nxt_lft=pts[ix[ix_lft]].x;
				y_nxt_lft=pts[ix[ix_lft]].y; // ? whatif prevy==nxty
				dy_lft=y_nxt_lft-y;
				if(dy_lft!=0){
					dxdy_lft=(x_nxt_lft-x_lft)/dy_lft;
				}else{
					dxdy_lft=x_nxt_lft-x_lft;
				}
			}
			if(adv_rht){
				if(ix_rht==0){
					ix_rht=last_elem_ix;
				}else{
					ix_rht--;
				}
				x_nxt_rht=pts[ix[ix_rht]].x;
				y_nxt_rht=pts[ix[ix_rht]].y;
				dy_rht=y_nxt_rht-y;
				if(dy_rht!=0){
					dxdy_rht=(x_nxt_rht-x_rht)/dy_rht;
				}else{
					dxdy_rht=x_nxt_rht-x_rht;
				}
			}
			CoordPx scan_lines_until_next_turn=0;
			const CoordPx yscr=CoordPx(y);
			if(y_nxt_lft>y_nxt_rht){
//				scan_lines_until_next_turn=static_cast<CoordPx>(y_nxt_rht-y);
				scan_lines_until_next_turn=CoordPx(y_nxt_rht)-yscr;
				adv_lft=false;
				adv_rht=true;
			}else{
//				scan_lines_until_next_turn=static_cast<CoordPx>(y_nxt_lft-y); // this generates more artifacts
				scan_lines_until_next_turn=CoordPx(y_nxt_lft)-yscr;
				adv_lft=true;
				adv_rht=false;
			}
			while(true){
				if(scan_lines_until_next_turn<=0)
					break;
//				Color8b*p_lft=pline+static_cast<CoordPx>(x_lft+.5555f);
//				Color8b*p_rht=pline+static_cast<CoordPx>(x_rht+.5555f);
				Color8b*p_lft=pline+CoordPx(x_lft);
				Color8b*p_rht=pline+CoordPx(x_rht);
				if(p_lft>p_rht) // ? can happen?
					break;
				scan_lines_until_next_turn--;
				CoordPx npx=CoordPx(p_rht-p_lft);
				if(enable::draw_polygons_fill){
					pz_memset(p_lft,char(color),npx); // ? npx+1?
				}
				if(enable::draw_polygons_edges){
					*p_lft=color;
					*(p_lft+npx)=color;
				}
//				y+=1;
				pline+=wi;
				x_lft+=dxdy_lft;
				x_rht+=dxdy_rht;
			}
			if(ix_lft==ix_rht) // ? render dot or line?
				break;
			if(adv_lft){
				x_lft=x_nxt_lft;
				y=y_nxt_lft;
//				pline=static_cast<Color8b*>(dsp.data().address())+static_cast<CoordPx>(y)*wi;
			}
			if(adv_rht){
				x_rht=x_nxt_rht;
				y=y_nxt_rht;
//				pline=static_cast<Color8b*>(dsp.data().address())+static_cast<CoordPx>(y)*wi;
			}
		}
//		const PointIx*pi=ix;
//		for(PointIx j=0;j<npoly_ixs;j++){
//			const Point2D&p=pts[*pi++];
//			dot(dsp,p.x,p.y,static_cast<unsigned char>(4));
//		}
	}

	//----------------------------------------------------------------
	// statics
	//----------------------------------------------------------------
	inline static Object*all[World::nobjects_max]; // array of pointers to allocated objects
	inline static Object**free_ixes[World::nobjects_max]; // free indexes in all[]
	inline static SlotIx free_ixes_i{0}; // index in freeSlots[] of next free slot
	inline static SlotInfo used_ixes[World::nobjects_max]; // free indexes in all[]
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
				const bool o1_check_col_with_o2=o1->colchk_tb_&o2->tb_;
				const bool o2_check_col_with_o1=o2->colchk_tb_&o1->tb_;
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

				// check if o1 points in o2 bounding shape
				if(Object::is_in_collision(*o1,*o2)){
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
					continue;
				}
				// check if o2 points in o1 bounding shape
				if(Object::is_in_collision(*o2,*o1)){
					if(o1_check_col_with_o2){
						if(!o1->on_collision(*o2)){
							World::deleted_add(o1);
						}
					}
					if(o2_check_col_with_o1){
						if(!o2->on_collision(*o1)){
							World::deleted_add(o2);
						}
					}
				}
			}
		}
	}

private:
	static inline auto object_for_used_slot(const SlotIx i)->Object*{
		Object*o=used_ixes[i].obj;
		if(!o){
			err.p("null-pointer-exception [e1]");
			osca_halt();
		}
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
	static auto is_in_collision(Object&o1,Object&o2)->bool{
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
				World::draw_dot(p,5);
			}
			const Vector v{p0-p}; // vector from point on line to point to check
			if(v.dot(*nml_ptr)>0){ // use abs(v)<0.0001f (example)?
				// p "in front" of line, cannot be collision
				return false;
			}
			nml_ptr++;
			if(enable::draw_collision_check){
				World::draw_dot(p,0xe);
			}
		}
		return true;
	}
};

auto World::deleted_add(Object*o)->void{ // ! this might be called several times for the same object
//		if(!o->is_alive()){
//			err.p("world::deleted_add:1");
//			osca_halt();
//		}
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
	time=TimeSec(osca_tmr_lo)*sec_per_tick;
	time_dt=time-time_prv;
	// fps calculations
	fps_frame_counter++;
	const TimeSec dt=time-fps_last_time;
	if(dt>1){
		fps=Count(TimeSec(fps_frame_counter)/dt);
		fps_frame_counter=0;
		fps_last_time=time;
	}

	Object::draw_all(vga13h.bmp());
	PhysicsState::update_all();
	Object::update_all();
	Object::check_collisions();
	World::commit_deleted();
}

}// end namespace
