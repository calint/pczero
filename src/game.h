#pragma once

//
// osca game
//

#include"osca.h"
#include"lib.h"
#include"libge.h"

namespace osca::game{

class Ship;

class Game{
	inline static const PointPx play_area_top_left{0,50};
	inline static const DimensionPx play_area_dim{320,100};

	static auto create_player()->void;
	static auto create_scene()->void;
	static auto create_boss()->void;

	static auto create_circle(const Count segments)->Point*{
		Point*pts=new Point[unsigned(segments)];
		AngleRad th=0;
		AngleRad dth=2*PI/AngleRad(segments);
		for(Count i=0;i<segments;i++){
			Real fsin=0;
			Real fcos=0;
			sin_and_cos(th,fsin,fcos);
			pts[i]={fcos,-fsin};
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

	static auto draw_axis(Bitmap8b&dsp)->void{
		static AngleDeg deg=0;
		static Matrix R;
		if(deg>360){ // ? what if deg=730
			deg-=360;
		}
		deg+=5;
		const AngleRad rotation=deg_to_rad(deg);
		R.set_transform(5,rotation,{160,100});
		// dot axis
		dsp.draw_dot({160,100},0xf);
		const Vector xaxis=R.axis_x().normalize().scale(7);
		dsp.draw_dot({xaxis.x+160,xaxis.y+100},4);
		const Vector yaxis=R.axis_y().normalize().scale(7);
		dsp.draw_dot({yaxis.x+160,yaxis.y+100},2);
	}
public:
	inline static ObjectDef enemy_def{};
	inline static ObjectDef ship_def{};
	inline static ObjectDef bullet_def{};
	inline static ObjectDef wall_def{};
	inline static ObjectDef missile_def{};
	inline static ObjectDef boss_def{};

	inline static Ship*player{};
	inline static Object*boss{};
	inline static Count enemies_alive{};
	inline static Point boss_pos{20,60};
	inline static Vector boss_vel{10,0};

	static auto is_in_play_area(const Point&p)->bool{
		if(p.x>=Coord(play_area_top_left.x+play_area_dim.width())){
			return false;
		}
		if(p.x<Coord(play_area_top_left.x)){
			return false;
		}
		if(p.y>=Coord(play_area_top_left.y+play_area_dim.height())){
			return false;
		}
		if(p.y<Coord(play_area_top_left.y)){
			return false;
		}
		return true;
	}

	static auto draw_dot(const Point&p,const Color8b color)->void{
		if(!is_in_play_area(p)){
			return;
		}
		const CoordPx xi=CoordPx(p.x);
		const CoordPx yi=CoordPx(p.y);
		*static_cast<Color8b*>(vga13h.bmp().address_offset({xi,yi}))=color;
	}

	static auto draw_trajectory(const Point&p0,const Vector&vel,const TimeSec t_s,const TimeSec t_inc_s,const Color8b color)->void{
		TimeSec t=0;
		Point p{p0};
		while(t<t_s){
			t+=t_inc_s;
			p.inc_by(vel,t_inc_s);
			draw_dot(p,color);
		}
	}

	static auto is_in_play_area(const Object&o)->bool{
		const Real bounding_radius=o.bounding_radius();
		const PhysicsState&phy=o.phy_const();
		const Real xmax=Real(Game::play_area_top_left.x+Game::play_area_dim.width());
		const Real xmin=Real(Game::play_area_top_left.x);
		const Real ymax=Real(Game::play_area_top_left.y+Game::play_area_dim.height());
		const Real ymin=Real(Game::play_area_top_left.y);
		return !(phy.pos.x>=xmax-bounding_radius || phy.pos.x<=xmin+bounding_radius ||
	             phy.pos.y>=ymax-bounding_radius || phy.pos.y<=ymin+bounding_radius);
	}

	[[noreturn]] static auto run()->void;
};

//
// game objects
//

constexpr TypeBits tb_none = 0;
constexpr TypeBits tb_ships = 1;
constexpr TypeBits tb_bullets = 2;
constexpr TypeBits tb_enemies = 4;
constexpr TypeBits tb_walls = 8;
constexpr TypeBits tb_missiles = 16;
constexpr TypeBits tb_bosses = 32;

class Enemy final:public Object{
	static constexpr Scale scl=5;
	static constexpr Scale bounding_radius=scl*sqrt_of_2;
public:
	Enemy():
		Object{
			tb_enemies,
			tb_bullets|tb_missiles,
			Game::enemy_def,scl,bounding_radius,{0,0},0,3}
	{
		Game::enemies_alive++;
	}

	~Enemy()override{
		Game::enemies_alive--;
	}

	auto update()->bool override{
		if(!Game::is_in_play_area(*this)){
			phy().vel.y=-phy().vel.y;
		}
		return true;
	}

	// returns false if object is to be deleted
	auto on_collision(Object&other)->bool override{
		return false; // collision with type 'bullet'
	}
};

class Bullet final:public Object{
	static constexpr Scale scl=0.5;
	static constexpr Scale bounding_radius=scl*sqrt_of_2;
	TimeSec created_time;
public:
	static constexpr Scalar speed=40;
	static constexpr TimeSec lifetime=10;

	Bullet():
		Object{
			tb_bullets,
			tb_bosses|tb_enemies|tb_ships|tb_walls,
			Game::bullet_def,scl,bounding_radius,{0,0},0,4},
		created_time{time}
	{}

	auto update()->bool override{
		Object::update();
		if(lifetime<time-created_time)
			return false;
		return Game::is_in_play_area(*this);
	}

	// returns false if object is to be deleted
	auto on_collision(Object&other)->bool override{
		return false;
	}
};

class Ship final:public Object{
	static constexpr Scale scl=4;
	static constexpr Scale bounding_radius=scl*sqrt_of_2;
	static constexpr AngleDeg dagl=90;
	static constexpr Scalar speed=20;
	static constexpr TimeSec fire_rate=TimeSec(.2);
	TimeSec fire_t{0};
public:
	bool auto_aim_at_boss{false};
	Byte padding[3]{};

	Ship():
		Object{
			tb_ships,
			tb_bosses|tb_bullets|tb_enemies|tb_missiles|tb_ships|tb_walls,
			Game::ship_def,scl,bounding_radius,{0,0},0,2}
	{
		Game::player=this;
	}

	~Ship()override{
		Game::player=nullptr;
	}

	auto turn_left()->void{phy().dagl=-deg_to_rad(dagl);}
	auto turn_right()->void{phy().dagl=deg_to_rad(dagl);}
	auto turn_still()->void{phy().dagl=0;}
	auto thrust_fwd()->void{phy().vel=forward_vector().scale(speed);}
	auto thrust_rev()->void{phy().vel=forward_vector().negate().scale(speed);}

	auto update()->bool override{
		Object::update();
		if(!Game::is_in_play_area(*this)){
			phy().vel={0,0};
		}
		if(!auto_aim_at_boss){
			return true;
		}
		if(!Game::boss){
			turn_still();
			return true;
		}
		attack_target_expected_location(*Game::boss,true);
		return true;
	}

	// returns false if object is to be deleted
	auto on_collision(Object&other)->bool override{
		// collision with 'wall'
		return false;
	}

	auto fire()->void{
		const TimeSec dt=time-fire_t;
		if(dt<fire_rate){
			return;
		}
		fire_t=time;
		Bullet*b=new Bullet;
		Vector v=forward_vector().scale(Real(1.1));
		v.scale(scale()); // place bullet in front of ship
		b->phy().pos=phy().pos+v;
		b->phy().vel=v.normalize().scale(Bullet::speed);
		b->phy().agl=phy().agl;
	}
private:
	auto attack_target_current_location(const Object&target,const bool draw_trajectory=false)->void{
		// aim and shoot at targets' current location
		constexpr Real margin_of_error=Real(0.01);
		Vector v_tgt=target.phy_const().pos-phy_const().pos;
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
		Vector v_aim=find_aim_vector_for_moving_target(target,Bullet::lifetime,Real(.2),margin_of_error_t,draw_trajectory);
		if(v_aim.x==0 && v_aim.y==0){
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

	// ? move to TargetingSystem class
	auto find_aim_vector_for_moving_target(const Object&tgt,const Real eval_t,const Real eval_dt,const Real error_margin_t,const bool draw_trajectory=false)->Vector{
		Real t=0;
		Point p_tgt=tgt.phy_const().pos;
		Vector v_tgt=tgt.phy_const().vel;
		const Vector a_tgt=tgt.phy_const().acc;
		while(t<eval_t){
			t+=eval_dt;
			// get expected position of target
			v_tgt.inc_by(a_tgt,eval_dt);
			p_tgt.inc_by(v_tgt,eval_dt);
			if(draw_trajectory){
				// draw target position at 't'
				Game::draw_dot(p_tgt,2);
			}

			// aim vector to the expected location
			const Vector v_aim=p_tgt-phy_const().pos;
			// get t for bullet to reach expected location
			const Real t_bullet=v_aim.magnitude()/Bullet::speed;
			// note: optimizing away sqrt() in magnitude() reduces precision when
			//       aiming at far targets since t_aim grows in a "non-linear" way

			// difference between target and bullet intersection t
			const Real t_aim=abs(t_bullet-t);

			// draw evaluated aim vector
//			Vector v2=v_aim;
//			v2.normalize().scale(Bullet::speed);
//			Game::draw_trajectory(phy_ro().pos,v2,t_bullet,Real(.1),0xe);

			// if t within error margin return aim vector
			if(t_aim<error_margin_t){
				if(draw_trajectory){
					// draw aim vector
					Vector v3=v_aim;
					v3.normalize().scale(Bullet::speed);
					Game::draw_trajectory(phy_const().pos,v3,t_bullet,Real(.2),2);
//					err.pos({1,1}).p_hex_32b(unsigned(t_aim*100));
				}
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
		Object{
			tb_walls,
			tb_none,
			Game::wall_def,scl,scl*sqrt_of_2,pos,agl,3}
	{}
};


class Missile final:public Object{
	static constexpr Scale scl=2;
	static constexpr Scale bounding_radius=scl*sqrt_of_2;
public:
	Missile():
		Object{
			tb_missiles,
			tb_bosses|tb_bullets|tb_enemies|tb_missiles|tb_ships|tb_walls,
			Game::missile_def,scl,bounding_radius,{0,0},0,4}
	{}

	auto update()->bool override{
		Object::update();
		return Game::is_in_play_area(*this);
	}

	// returns false if object is to be deleted
	auto on_collision(Object&other)->bool override{
		return false;
	}
};

class Boss final:public Object{
	static constexpr Scale scl=3;
	static constexpr Scale bounding_radius=scl*sqrt_of_2;
	static constexpr TimeSec boss_live_t{10};

	Count health{5};
	TimeSec time_started{};
public:
	Boss():
		Object{
			tb_bosses,
			tb_bullets|tb_missiles,
			Game::boss_def,scl,bounding_radius,{0,0},0,0x4}
	{
		time_started=time;
		Game::boss=this;
	}

	~Boss()override{
		Game::boss=nullptr;
	}

	auto update()->bool override{
		Object::update();
		if(!Game::is_in_play_area(*this)){
			return false;
		}
		if(time-time_started>boss_live_t){
			return false;
		}
		return true;
	}

	// returns false if object is to be deleted
	auto on_collision(Object&other)->bool override{
		health--;
		return health>0;
	}
};

auto Game::create_scene()->void{
	for(Real i=30;i<300;i+=20){
		Enemy*o=new Enemy;
		o->phy().pos={i,60};
		o->phy().agl=deg_to_rad(i);
		o->phy().dagl=deg_to_rad(10);
		o->phy().vel={0,2};
	}
}

auto Game::create_player()->void{
	Ship*o=new Ship;
	o->phy().pos={160,130};
	player=o;
}

auto Game::create_boss()->void{
	Object*o=new Boss;
	if(boss_vel.y>20){
		if(boss_vel.x>0){
			boss_pos={300,60};
			boss_vel={-10,0};
		}else{
			boss_pos={20,60};
			boss_vel={10,0};
		}
	}
	if(boss_vel.x>0){
		boss_vel.inc_by({5,2});
	}else{
		boss_vel.inc_by({-5,2});
	}
	o->phy().pos=boss_pos;
	o->phy().vel=boss_vel;
	o->phy().dagl=deg_to_rad(25);
}

[[noreturn]] auto Game::run()->void{
	const TaskId task_id=osca_active_task->get_id();
	
	//----------------------------------------------------------
	// init statics
	//----------------------------------------------------------
	
	// framework
	Object::init_statics();
	PhysicsState::init_statics();
	PhysicsState::clear(11);

	//
	// object definitions
	//	
	enemy_def={5,4,
		new Point[]{ // points in model coordinates, negative Y is "forward"/"up"
			{ 0,  0},
			{-1,-.5},
			{-1, .5},
			{ 1, .5},
			{ 1,-.5},
		},
		new PointIx[]{1,2,3,4} // bounding convex polygon CCW
	};
	enemy_def.init_normals();

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

	bullet_def={1,1, // at least one bounding point for collision detection
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

	constexpr Count segments=6;
	boss_def={segments,segments,
		create_circle(segments),
		create_circle_ix(segments),
	};
	boss_def.init_normals();

	// clear screen by copying ram to vga
	const Address clear_start_at_address=vga13h.bmp().address_offset({0,50});
	const Address clear_copy_from_address=Heap::data().address();
	const SizeBytes clear_copy_num_bytes=vga13h.bmp().dim().width()*100;

	create_player();
	create_scene();
	create_boss();

	constexpr Byte key_w=0;
	constexpr Byte key_a=1;
	constexpr Byte key_s=2;
	constexpr Byte key_d=3;
	constexpr Byte key_spc=4;
	constexpr Byte key_j=5;
	bool keyb[6]{}; // wasd j and space pressed status

	out.pos({12,1}).fg(6).p("keys: w a s d j  f g x c [ctrl+tab] [ctrl+Fx]");

	// game loop
	while(true){
		// clear game area
		pz_memcpy(clear_start_at_address,clear_copy_from_address,clear_copy_num_bytes);

		Object::tick();

		if(!Game::boss){
			create_boss();
		}

		// print stats
		out.pos({10,2}).fg(2);
		out.p("i=").p_hex_8b(static_cast<Byte>(task_focused_id)).spc();
		out.p("t=").p_hex_16b(static_cast<unsigned short>(osca_tmr_lo)).spc();
		out.p("k=").p_hex_8b(static_cast<Byte>(osca_key)).spc();
		out.p("m=").p_hex_8b(static_cast<Byte>(metrics::matrix_set_transforms)).spc();
		out.p("c=").p_hex_8b(static_cast<Byte>(metrics::collisions_checks)).spc();
		out.p("b=").p_hex_8b(static_cast<Byte>(metrics::collisions_checks_bounding_shapes)).spc();
		out.p("a=").p_hex_8b(static_cast<Byte>(Object::allocated_objects_count())).spc();
		out.p("s=").p_hex_8b(static_cast<Byte>(Object::time)).spc();
		out.p("f=").p_hex_16b(static_cast<unsigned short>(Object::fps)).spc();

		Ship*shp=Game::player;

		if(task_focused_id==task_id){
			// this task has keyboard focus, handle keyboard
			while(const Byte sc=keyboard.get_next_scan_code()){
				switch(sc){
				case 0x11: // w pressed
					keyb[key_w]=true;
					break;
				case 0x91: // w released
					keyb[key_w]=false;
					break;
				case 0x1e: // a pressed
					keyb[key_a]=true;
					break;
				case 0x9e: // a released
					keyb[key_a]=false;
					break;
				case 0x1f: // s pressed
					keyb[key_s]=true;
					break;
				case 0x9f: // s released
					keyb[key_s]=false;
					break;
				case 0x20: // d pressed
					keyb[key_d]=true;
					break;
				case 0xa0: // d released
					keyb[key_d]=false;
					break;
				case 0x39: // space pressed
					keyb[key_spc]=true;
					break;
				case 0xb9: // space released
					keyb[key_spc]=false;
					break;
				case 0x24: // j pressed
					keyb[key_j]=true;
					break;
				case 0xa4: // j released
					keyb[key_j]=false;
					break;
				default:
					break;
				}
				switch(table_scancode_to_ascii[sc]){
				case'x':
					if(Game::enemies_alive==0)create_scene();
					break;
				case'c':
					if(!shp)create_player();
					break;
				case'f':
					if(shp)shp->auto_aim_at_boss=true;
					break;
				case'g':
					if(shp)shp->auto_aim_at_boss=false;
					break;
				default:
					break;
				}
			}
		}
		if(shp){
			if(keyb[key_w])shp->thrust_fwd();
			if(keyb[key_s])shp->thrust_rev();
			if(!shp->auto_aim_at_boss){
				if(keyb[key_a])shp->turn_left();
				if(keyb[key_d])shp->turn_right();
				if(!keyb[key_a] && !keyb[key_d])shp->turn_still();
			}
			if(!keyb[key_w] && !keyb[key_s])shp->phy().vel={0,0};
			if(keyb[key_j])shp->fire();
		}
	}
}

} // end namespace osca::game
