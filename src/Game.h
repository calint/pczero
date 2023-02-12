#pragma once
#include"osca.h"
#include"lib.h"
#include"lib2d.h"
#include"libge.h"

namespace osca{

class Game{
	inline static CoordsPx play_area_top_left{0,50};
	inline static DimensionPx play_area_dim{320,100};

	static auto create_scene();
	static auto create_scene2();
	static auto create_scene3();
	static auto create_boss();
	static auto create_circle(const Count segments)->Point*{
		Point*pts=new Point[unsigned(segments)];
		AngleRad th=0;
		AngleRad dth=2*PI/AngleRad(segments);
		for(Count i=0;i<segments;i++){
			pts[i]={cos(th),-sin(th)}; // CCW
			th+=dth;
		}
		return pts;
	}
	static auto create_circle_ix(const Count segments)->PointIx*{
		PointIx*ix=new PointIx[unsigned(segments)];
		for(PointIx i=0;i<segments;i++){
			ix[i]=i;
		}
		return ix;
	}

	static auto draw_axis(Bitmap8b&dsp){
		static AngleDeg deg=0;
		static Matrix2D R;
		if(deg>360)
			deg-=360;
		deg+=5;
		const AngleRad rotation=deg_to_rad(deg);
	//		shp3->set_angle(rotation);
		R.set_transform(5,rotation,{160,100});
		// dot axis
		world::draw_dot(dsp,160,100,0xf);
		const Vector xaxis=R.axis_x().normalize().scale(7);
		world::draw_dot(dsp,xaxis.x+160,xaxis.y+100,4);
		const Vector yaxis=R.axis_y().normalize().scale(7);
		world::draw_dot(dsp,yaxis.x+160,yaxis.y+100,2);
	}
public:
	inline static ObjectDef enemy_def;
	inline static ObjectDef ship_def;
	inline static ObjectDef bullet_def;
	inline static ObjectDef wall_def;
	inline static ObjectDef missile_def;
	inline static ObjectDef boss_def;

	inline static Object*player{nullptr};
	inline static Object*boss{nullptr};
	inline static Count enemies_alive{0};
	inline static Point boss_pos{20,60};
	inline static Vector boss_vel{10,0};

	static constexpr auto is_within_play_area(const Point&p)->bool{
		if(p.x>=Coord(play_area_top_left.x()+play_area_dim.width())){
			return false;
		}
		if(p.x<=Coord(play_area_top_left.x())){
			return false;
		}
		if(p.y>=Coord(play_area_top_left.y()+play_area_dim.height())){
			return false;
		}
		if(p.y<=Coord(play_area_top_left.y())){
			return false;
		}
		return true;
	}

	static constexpr void draw_dot(const Point&p,const Color8b color){
		if(!is_within_play_area(p))
			return;
		const CoordPx xi=CoordPx(p.x);
		const CoordPx yi=CoordPx(p.y);
		vga13h.bmp().pointer_offset({xi,yi}).write(color);
	}

	static auto draw_trajectory(const Point&p0,const Vector&vel,const Real t_s,const Real t_inc_s,const Color8b color)->void{
		Real t=0;
		while(true){
			t+=t_inc_s;
			Vector v=vel;
			v.scale(t);
			v.inc_by(p0);
			draw_dot(v,color);
			if(t>t_s)
				return;
		}
	}

	static constexpr auto is_within_play_area(Object&o)->bool{
		const Scale bounding_radius=o.bounding_radius();
		if(o.phy().pos.x>Coord(Game::play_area_top_left.x()+Game::play_area_dim.width())-bounding_radius){
			return false;
		}
		if(o.phy().pos.x<Coord(Game::play_area_top_left.x())+bounding_radius){
			return false;
		}
		if(o.phy().pos.y>Coord(Game::play_area_top_left.y()+Game::play_area_dim.height())-bounding_radius){
			return false;
		}
		if(o.phy().pos.y<Coord(Game::play_area_top_left.y())+bounding_radius){
			return false;
		}
		return true;
	}
	[[noreturn]] static auto start()->void;
};

class Enemy final:public Object{
	static constexpr Scale scale=5;
	static constexpr Scale bounding_radius=scale*sqrt_of_2;
public:
	// 'ships'   0b00'0001
	// 'bullets' 0b00'0010
	// 'enemies' 0b00'0100
	// 'walls'   0b00'1000
	// 'missiles'0b01'0000
	// 'bosses'  0b10'0000
	Enemy(const Point&pos,const AngleRad agl):
		Object{0b100,0b01'0010,Game::enemy_def,scale,bounding_radius,pos,agl,3}
	{
		Game::enemies_alive++;
	}
	~Enemy()override{
		Game::enemies_alive--;
	}

	virtual auto update()->bool override{
		if(!Game::is_within_play_area(*this)){
			phy().vel.y=-phy().vel.y;
		}
		return true;
	}

	// returns false if object is to be deleted
	virtual auto on_collision(Object&other)->bool override{
		return false; // collision with type 'bullet'
	}
};

class Bullet final:public Object{
	static constexpr Scale scale=1;
	static constexpr Scale bounding_radius=scale*sqrt_of_2;
public:
	Bullet():
		// 'ships'   0b00'0001
		// 'bullets' 0b00'0010
		// 'enemies' 0b00'0100
		// 'walls'   0b00'1000
		// 'missiles'0b01'0000
		// 'bosses'  0b10'0000
		Object{0b10,0b11'1101,Game::bullet_def,scale,bounding_radius,{0,0},0,4}
	{}
	virtual auto update()->bool override{
		Object::update();
		return Game::is_within_play_area(*this);
	}
	// returns false if object is to be deleted
	virtual auto on_collision(Object&other)->bool override{
		return false;
	}

	inline static constexpr Real speed=40;
};

class Ship final:public Object{
	static constexpr Scale scale=4;
	static constexpr Scale bounding_radius=scale*sqrt_of_2;
	static constexpr AngleRad dagl=90;
	static constexpr Scalar speed=20;
	Real fire_t_s=0;
public:
	Ship():
		// 'ships'   0b00'0001
		// 'bullets' 0b00'0010
		// 'enemies' 0b00'0100
		// 'walls'   0b00'1000
		// 'missiles'0b01'0000
		// 'bosses'  0b10'0000
		Object{0b1,0b11'1111,Game::ship_def,scale,bounding_radius,{0,0},0,2}
	{
		Game::player=this;
	}
	~Ship()override{
		Game::player=nullptr;
	}

	auto turn_left()->void{
		phy().dagl=-deg_to_rad(dagl);
	}
	auto turn_right()->void{
		phy().dagl=deg_to_rad(dagl);
	}
	auto turn_still()->void{
		phy().dagl=0;
	}
	auto thrust_fwd()->void{
		phy().vel=forward_vector().scale(speed);
	}
	auto thrust_rev()->void{
		phy().vel=forward_vector().negate().scale(speed);
	}

	virtual auto update()->bool override{
		Object::update();
		if(!Game::is_within_play_area(*this)){
			phy().vel={0,0};
		}
		if(!Game::boss){
			turn_still();
			return true;
		}
//		attack_target_current_location(*Game::boss,true);
		attack_target_expected_location(*Game::boss,true);
		return true;
	}

	// returns false if object is to be deleted
	virtual auto on_collision(Object&other)->bool override{
		// collision with 'wall'
		return false;
	}

	auto fire()->void{
		const Real dt=world::time_s-fire_t_s;
		if(dt<Real(.2))
			return;
		fire_t_s=world::time_s;
		Bullet*b=new Bullet;
		Vector v=forward_vector().scale(Real(1.1));
		v.scale(scl_); // place bullet in front of ship
		b->phy().pos=phy().pos+v;
		b->phy().vel=v.normalize().scale(Bullet::speed);
		b->phy().agl=phy().agl;
	}
private:
	auto attack_target_current_location(const Object&target,const bool draw_trajectory=false)->void{
		// aim and shoot at targets' current location
		constexpr Real margin_of_error=Real(0.01);
		Vector v_tgt=target.phy_ro().pos-phy_ro().pos;
		v_tgt.normalize();
		Vector v_fwd=forward_vector();
		// draw trajectory of bullet
		Vector v_bullet=v_fwd;
		v_bullet.scale(Bullet::speed);
		if(draw_trajectory){
			Game::draw_trajectory(phy().pos,v_bullet,5,.5,0xe);
		}
		Vector n_fwd=v_fwd.normal();
		Real dot=v_tgt.dot(n_fwd);
		if(abs(dot)<margin_of_error){
			turn_still();
			fire();
		}else if(dot<0){
			turn_left();
		}else if(dot>0){
			turn_right();
		}
	}

	// aim and shoot at targets' expected location
	auto attack_target_expected_location(const Object&target,const bool draw_trajectory=false)->void{
		constexpr Real margin_of_error_t=Real(0.25);
		Vector v_aim=find_aim_vector_for_moving_target(target,10,Real(.2),margin_of_error_t);
		if(v_aim.x==0&&v_aim.y==0){
			// did not find aim vector
			turn_still();
			return;
		}
		v_aim.normalize();
		const Vector v_fwd=forward_vector();
		const Vector n_fwd=v_fwd.normal();
		const Real dot=v_aim.dot(n_fwd);
		constexpr Real margin_of_error_aim=Real(0.01);
		if(abs(dot)<margin_of_error_aim){
			turn_still();
			fire();
		}else if(dot<0){
			turn_left();
		}else if(dot>0){
			turn_right();
		}
	}

	auto find_aim_vector_for_moving_target(const Object&tgt,const Real t_eval_span,const Real dt,const Real error_margin_t)->Vector{
		Real t=0;
		const Point p_tgt=tgt.phy_ro().pos;
		const Vector v_tgt=tgt.phy_ro().vel;
		while(true){
			t+=dt;
			if(t>t_eval_span)
				break;
			// get expected position of target at 't'
			Vector v=v_tgt;
			v.scale(t);
			Point p{p_tgt};
			p.inc_by(v);

			// draw target position at 't'
			Game::draw_dot(p,2);

			// aim vector to the expected location
			const Vector v_aim=p-phy_ro().pos;
			// get magnitude of aim vector
			const Real mgn=v_aim.magnitude();
			// get t for bullet to reach expected location
			const Real t_bullet=mgn/Bullet::speed;

			// draw evaluated aim vector
//			Vector v2=v_aim;
//			v2.normalize().scale(Bullet::speed);
//			Game::draw_trajectory(phy_ro().pos,v2,t_bullet,Real(.1),0xe);

			// if t within error margin return aim vector
			const Real t_aim=abs(t_bullet-t);
			if(t_aim<error_margin_t){
				// draw aim vector
				Vector v3=v_aim;
				v3.normalize().scale(Bullet::speed);
				Game::draw_trajectory(phy_ro().pos,v3,t_bullet,Real(.2),2);
//				err.pos({1,1}).p_hex_32b(unsigned(t_aim*100));
				return v_aim;
			}
		}
		// no aim vector found for evaluated time span
		return{0,0};
	}
};


class Wall final:public Object{
public:
	Wall(const Scale scl,const Point&pos,const AngleRad agl):
		Object{0b00'1000,0,Game::wall_def,scl,scl*sqrt_of_2,pos,agl,3}
	{}
};


class Missile final:public Object{
	static constexpr Scale scale=2;
	static constexpr Scale bounding_radius=scale*sqrt_of_2;
public:
	// 'ships'   0b00'0001
	// 'bullets' 0b00'0010
	// 'enemies' 0b00'0100
	// 'walls'   0b00'1000
	// 'missiles'0b01'0000
	// 'bosses'  0b10'0000
	Missile():
		Object{0b01'0000,0b11'1111,Game::missile_def,scale,bounding_radius,{0,0},0,4}
	{}
	virtual auto update()->bool override{
		Object::update();
		return Game::is_within_play_area(*this);
	}
	// returns false if object is to be deleted
	virtual auto on_collision(Object&other)->bool override{
		return false;
	}
};

class Boss final:public Object{
	static constexpr Scale scale=3;
	static constexpr Scale bounding_radius=scale*sqrt_of_2;
	Count health{5};
public:
	// 'ships'   0b00'0001
	// 'bullets' 0b00'0010
	// 'enemies' 0b00'0100
	// 'walls'   0b00'1000
	// 'missiles'0b01'0000
	// 'bosses'  0b10'0000
	Boss():
		Object{0b10'0000,0b01'0010,Game::boss_def,scale,bounding_radius,{0,0},0,0xe}
	{
		Game::boss=this;
	}
	~Boss()override{
		Game::boss=nullptr;
	}
	virtual auto update()->bool override{
		Object::update();
//		if(game::player){
//			Vector v=game::player->phy().pos-phy().pos;
//			v.normalize().scale(5);
//			phy().vel=v;
//		}else{
//			Vector v=Vector{160,60}-phy().pos;
//			v.normalize().scale(5);
//			phy().vel=v;
//		}
//		return object_within_play_area(*this);
		if(!Game::is_within_play_area(*this)){
			phy_->pos=Game::boss_pos;
		}
		return true;
	}
	// returns false if object is to be deleted
	virtual auto on_collision(Object&other)->bool override{
		health--;
		if(health<=0)
			return false;
		return true;
	}
};

auto Game::create_scene(){
	for(Real i=30;i<300;i+=20){
		Enemy*e=new Enemy({i,60},deg_to_rad(i));
		e->phy().dagl=deg_to_rad(10);
		e->phy().vel={0,2};
	}
	Object*o=new Boss;
	o->phy().pos={160,60};
}
auto Game::create_scene2(){
	Object*o=new Wall(20,{160,100},0);
	o->phy().dagl=deg_to_rad(1);
}
auto Game::create_scene3(){
	new Enemy({160,100},0);
}
auto Game::create_boss(){
	Object*o=new Boss;
	o->phy().pos=boss_pos;
	o->phy().vel=Game::boss_vel;
	boss_vel.inc_by({5,2});
	o->phy().dagl=deg_to_rad(25);
	Game::boss=o;
}

[[noreturn]] auto Game::start()->void{
	//----------------------------------------------------------
	// init statics
	//----------------------------------------------------------
	enemy_def={5,4,
		new Point[]{ // points in model coordinates, negative Y is "forward"
			{ 0,0},
			{-1,-.5},
			{-1, .5},
			{ 1, .5},
			{ 1,-.5},
		},
		new PointIx[]{1,2,3,4} // bounding convex polygon CCW
	};
	enemy_def.init_normals();

//		constexpr unsigned segments=8;
//		ship_def={segments,segments,
//			create_circle(segments),
//			create_circle_ix(segments),
//		};
//		ship_def={4,4,
//			new Point2D[]{ // points in model coordinates, negative Y is "forward"
//				{-1,-1},
//				{-1, 1},
//				{ 1, 1},
//				{ 1,-1},
//			},
//			new PointIx[]{0,1,2,3} // bounding convex polygon CCW
//		};
	ship_def={4,3,
		new Point[]{
			{ 0, 0},
			{ 0,-1},
			{-1,.5},
			{ 1,.5},
		},
		new PointIx[]{1,2,3} // bounding convex polygon CCW
	};
	ship_def.init_normals();

	bullet_def={1,1, // ? why not 0 nbnd and nullptr bnd
		new Point[]{
			{0,0},
		},
		new PointIx[]{0} // bounding points and convex polygon CCW
	};
	bullet_def.init_normals();

	wall_def={4,4,
		new Point[]{ // points in model coordinates, negative Y is "forward"
			{-1,-1},
			{-1, 1},
			{ 1, 1},
			{ 1,-1},
		},
		new PointIx[]{0,1,2,3} // bounding convex polygon CCW
	};
	wall_def.init_normals();

	missile_def={4,3,
		new Point[]{
			{ 0,-1},
			{-1,.5},
			{ 1,.5},
		},
		new PointIx[]{0,1,2} // bounding convex polygon CCW
	};
	missile_def.init_normals();

	constexpr unsigned segments=6;
	boss_def={segments,segments,
		create_circle(segments),
		create_circle_ix(segments),
	};
	boss_def.init_normals();

//		out.p_hex_32b(static_cast<unsigned>(play_area_top_left.y()));
//		out.p_hex_32b(static_cast<unsigned>(play_area_dim.width()));
//		osca_halt();

	// init stack
	const Address heap_address=Heap::data().address();
	const Address heap_disp_at_addr=vga13h.bmp().data().pointer().offset(50*320).address();
	const SizeBytes heap_disp_size=320*100;

	Ship*shp=new Ship;
	shp->phy().pos={160,130};
//		shp->phy().pos={160,100};
//		create_scene();
	create_boss();

//		Ship*shp=nullptr;
//	out.p_hex_16b(static_cast<unsigned short>(sizeof(Object))).pos({1,2});

	constexpr unsigned char key_w=0;
	constexpr unsigned char key_a=1;
	constexpr unsigned char key_s=2;
	constexpr unsigned char key_d=3;
	constexpr unsigned char key_spc=4;
	bool keyboard[]{false,false,false,false,false}; // wasd and space pressed status

	world::init();
	// start task
	while(true){
		pz_memcpy(heap_disp_at_addr,heap_address,heap_disp_size);

		world::tick();

		if(!Game::boss)
			create_boss();
//			Bitmap8b bmp{Address(0x10'0000),{100,100}};
//			bmp.to(vga13h.bmp(),{100,1});

		// copy heap to screen
//			err.pos({0,2});
		out.pos({0,1});//.p("                                                            ").pos({0,1});

//			Object::render_all(vga13h.bmp());
//			PhysicsState::update_physics_states();
//			Object::update_all();
//			Object::check_collisions();
//			world::deleted_commit();

		//		out.pos({0,2}).p("                                              ").pos({0,2});
		out.pos({12,2}).fg(2);
		out.p("k=").p_hex_8b(static_cast<unsigned char>(osca_key)).spc();
		out.p("e=").p_hex_8b(static_cast<unsigned char>(Game::enemies_alive)).spc();
		out.p("m=").p_hex_8b(static_cast<unsigned char>(metrics::matrix_set_transforms)).spc();
		out.p("c=").p_hex_8b(static_cast<unsigned char>(metrics::collisions_checks)).spc();
		out.p("b=").p_hex_8b(static_cast<unsigned char>(metrics::collisions_checks_bounding_shapes)).spc();
		out.p("f=").p_hex_8b(static_cast<unsigned char>(Object::free_ixes_i)).spc();
		out.p("u=").p_hex_8b(static_cast<unsigned char>(Object::used_ixes_i)).spc();
		out.p("t=").p_hex_16b(static_cast<unsigned short>(osca_t)).spc();
		out.p("s=").p_hex_8b(static_cast<unsigned char>(world::time_s)).spc();
		out.p("d=").p_hex_8b(static_cast<unsigned char>(world::time_dt_s*1000)).spc();

		if(!Game::player)
			shp=nullptr;

//		if(Game::player)
//			Game::draw_trajectory(vga13h.bmp(),Game::player->phy().pos,Game::player->phy().vel,10,.5,0xe);
//		if(Game::boss)
//			Game::draw_trajectory(vga13h.bmp(),Game::boss->phy().pos,Game::boss->phy().vel,10,.5,0xe);

		if(shp){
			while(const unsigned kc=osca_keyb.get_next_scan_code()){
				switch(kc){
				case 0x11: // w pressed
					keyboard[key_w]=true;
					break;
				case 0x91: // w released
					keyboard[key_w]=false;
					break;
				case 0x1e: // a pressed
					keyboard[key_a]=true;
					break;
				case 0x9e: // a released
					keyboard[key_a]=false;
					break;
				case 0x1f: // s pressed
					keyboard[key_s]=true;
					break;
				case 0x9f: // s released
					keyboard[key_s]=false;
					break;
				case 0x20: // d pressed
					keyboard[key_d]=true;
					break;
				case 0xa0: // d released
					keyboard[key_d]=false;
					break;
				case 0x39: // space pressed
					keyboard[key_spc]=true;
					break;
				case 0xb9: // space released
					keyboard[key_spc]=false;
					break;
				default:
					break;
				}
			}
			if(keyboard[key_w])
				shp->thrust_fwd();

//				if(keyboard[key_a])
//					shp->turn_left();

			if(keyboard[key_s])
				shp->thrust_rev();

//				if(keyboard[key_d])
//					shp->turn_right();

//				if(!keyboard[key_a]&&!keyboard[key_d])
//					shp->turn_still();

			if(!keyboard[key_w]&&!keyboard[key_s])
				shp->phy().vel={0,0};

			if(keyboard[key_spc])
				shp->fire();
		}
		switch(table_scancode_to_ascii[osca_key]){
		case'x':
			if(Game::enemies_alive==0)
				create_scene();
			break;
		case'c':
			if(shp)
				break;
			shp=new Ship;
			shp->phy().pos={160,130};
			break;
		default:
			break;
		}

//			draw_axis(vga13h.bmp());

		osca_yield();
	}
}

} // end namespace
