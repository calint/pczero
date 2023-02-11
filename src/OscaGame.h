#pragma once
#include"osca.h"
#include"lib.h"
#include"lib2d.h"
#include"libge.h"

namespace osca{

namespace game{
	static bool player_alive=true;
	static Count enemies_alive=0;
}

static ObjectDef enemy_def;
static ObjectDef ship_def;
static ObjectDef bullet_def;
static ObjectDef wall_def;
static ObjectDef missile_def;

static CoordsPx play_area_top_left{0,50};
static DimensionPx play_area_dim{320,100};

static constexpr auto object_within_play_area(Object&o)->bool{
	const Scale bounding_radius=o.bounding_radius();
	if(o.phy().pos.x>static_cast<Coord>(play_area_top_left.x()+play_area_dim.width())-bounding_radius){
		return false;
	}
	if(o.phy().pos.x<static_cast<Coord>(play_area_top_left.x())+bounding_radius){
		return false;
	}
	if(o.phy().pos.y>static_cast<Coord>(play_area_top_left.y()+play_area_dim.height())-bounding_radius){
		return false;
	}
	if(o.phy().pos.y<static_cast<Coord>(play_area_top_left.y())+bounding_radius){
		return false;
	}
	return true;
}

class Enemy final:public Object{
	static constexpr Scale scale=5;
	static constexpr Scale bounding_radius=scale*sqrt_of_2;
public:
	// type bits 0b100 check collision with
	// 'bullet'  0b0'0010
	Enemy(const Point2D&pos,const AngleRad agl):
		Object{0b100,0b10,enemy_def,scale,bounding_radius,pos,agl,3}
	{
		game::enemies_alive++;
	}
	~Enemy()override{
		game::enemies_alive--;
	}

	virtual auto update()->bool override{
		if(!object_within_play_area(*this)){
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
		// type bits 0b10 check collision with
		// 'ships'   0b0'0001
		// 'enemies' 0b0'0100
		// 'walls'   0b0'1000
		// 'missiles'0b1'0000
		Object{0b10,0b1'1101,bullet_def,scale,bounding_radius,{0,0},0,4}
	{}
	virtual auto update()->bool override{
		Object::update();
		return object_within_play_area(*this);
	}
	// returns false if object is to be deleted
	virtual auto on_collision(Object&other)->bool override{
		return false;
	}
};

class Ship final:public Object{
	static constexpr Scale scale=4;
	static constexpr Scale bounding_radius=scale*sqrt_of_2;
	Real fire_t_s=0;
public:
	Ship():
		// type bits 0b1 check collision with:
		// 'ships'   0b0'0001
		// 'bullets' 0b0'0010
		// 'enemies' 0b0'0100
		// 'walls'   0b0'1000
		// 'missiles'0b1'0000
		Object{0b1,0b1'1111,ship_def,scale,bounding_radius,{0,0},0,2}
	{}
	~Ship()override{
		game::player_alive=false;
	}

	virtual auto update()->bool override{
		Object::update();
		if(!object_within_play_area(*this)){
			phy().vel={0,0};
		}
		return true;
	}

	// returns false if object is to be deleted
	virtual auto on_collision(Object&other)->bool override{
		// collision with 'wall'
		return false;
	}

	auto fire(){
		const Real dt=world::time_s-fire_t_s;
		if(dt<.2f)
			return;
		fire_t_s=world::time_s;
		Bullet*b=new Bullet;
		Vector2D v=forward_vector().scale(1.1f);
		v.scale(scl_); // place bullet in front of ship
		b->phy().pos=phy().pos+v;
		b->phy().vel=v.normalize().scale(40);
		b->phy().agl=phy().agl;
	}
};


class Wall final:public Object{
public:
	// type bits 0b100 check collision with nothing
	Wall(const Scale scl,const Point2D&pos,const AngleRad agl):
		Object{0b0'1000,0,wall_def,scl,scl*sqrt_of_2,pos,agl,3}
	{}
};


class Missile final:public Object{
	static constexpr Scale scale=2;
	static constexpr Scale bounding_radius=scale*sqrt_of_2;
public:
	// type bits 0b1 check collision with:
	// 'ships'   0b0'0001
	// 'bullets' 0b0'0010
	// 'enemies' 0b0'0100
	// 'walls'   0b0'1000
	// 'missiles'0b1'0000
	Missile():
		Object{0b1'0000,0b1'1111,missile_def,scale,bounding_radius,{0,0},0,4}
	{}
	virtual auto update()->bool override{
		Object::update();
		return object_within_play_area(*this);
	}
	// returns false if object is to be deleted
	virtual auto on_collision(Object&other)->bool override{
		return false;
	}
};

class OscaGame{
	auto create_scene(){
		for(Real i=30;i<300;i+=20){
			Enemy*e=new Enemy({i,60},deg_to_rad(i));
			e->phy().dagl=deg_to_rad(10);
			e->phy().vel={0,2};
		}
	}
	auto create_scene2(){
		Object*o=new Wall(20,{160,100},0);
		o->phy().dagl=deg_to_rad(1);
	}
	auto create_scene3(){
		new Enemy({160,100},0);
	}
	auto create_circle(const Count segments)->Point2D*{
		Point2D*pts=new Point2D[static_cast<unsigned>(segments)];
		AngleRad th=0;
		AngleRad dth=2*PI/static_cast<AngleRad>(segments);
		for(Count i=0;i<segments;i++){
			pts[i]={cos(th),-sin(th)}; // CCW
			th+=dth;
		}
		return pts;
	}
	auto create_circle_ix(const Count segments)->PointIx*{
		PointIx*ix=new PointIx[static_cast<unsigned>(segments)];
		for(PointIx i=0;i<segments;i++){
			ix[i]=i;
		}
		return ix;
	}
public:
	OscaGame(){
		//----------------------------------------------------------
		// init statics
		//----------------------------------------------------------
		enemy_def={5,4,
			new Point2D[]{ // points in model coordinates, negative Y is "forward"
				{ 0,0},
				{-1,-.5f},
				{-1, .5f},
				{ 1, .5f},
				{ 1,-.5f},
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
			new Point2D[]{
				{ 0, 0},
				{ 0,-1},
				{-1,.5},
				{ 1,.5},
			},
			new PointIx[]{1,2,3} // bounding convex polygon CCW
		};
		ship_def.init_normals();

		bullet_def={1,1, // ? why not 0 nbnd and nullptr bnd
			new Point2D[]{
				{0,0},
			},
			new PointIx[]{0} // bounding points and convex polygon CCW
		};
		bullet_def.init_normals();

		wall_def={4,4,
			new Point2D[]{ // points in model coordinates, negative Y is "forward"
				{-1,-1},
				{-1, 1},
				{ 1, 1},
				{ 1,-1},
			},
			new PointIx[]{0,1,2,3} // bounding convex polygon CCW
		};
		wall_def.init_normals();

		missile_def={4,3,
			new Point2D[]{
				{ 0,-1},
				{-1,.5},
				{ 1,.5},
			},
			new PointIx[]{0,1,2} // bounding convex polygon CCW
		};
		missile_def.init_normals();

//		out.p_hex_32b(static_cast<unsigned>(play_area_top_left.y()));
//		out.p_hex_32b(static_cast<unsigned>(play_area_dim.width()));
//		osca_halt();
	}

	[[noreturn]] auto start(){
		// init stack
		const Address heap_address=Heap::data().address();
		const Address heap_disp_at_addr=vga13h.bmp().data().pointer().offset(50*320).address();
		const SizeBytes heap_disp_size=320*100;


		constexpr AngleRad ship_dagl=90;
		constexpr Scalar ship_speed=20;
		Ship*shp=new Ship;
		shp->phy().pos={160,130};
//		shp->phy().pos={160,100};
		create_scene();
//		create_scene2();

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
			metrics::reset();
			world::tick();

			// copy heap to screen
			pz_memcpy(heap_disp_at_addr,heap_address,heap_disp_size);
//			err.pos({0,2});
			out.pos({0,1});//.p("                                                            ").pos({0,1});

			PhysicsState::update_physics_states();
			Object::render_all(vga13h.bmp());
			Object::update_all();
			Object::check_collisions();
			world::deleted_commit();

			//		out.pos({0,2}).p("                                              ").pos({0,2});
			out.pos({12,2}).fg(2);
			out.p("k=").p_hex_8b(static_cast<unsigned char>(osca_key)).spc();
			out.p("e=").p_hex_8b(static_cast<unsigned char>(game::enemies_alive)).spc();
			out.p("m=").p_hex_8b(static_cast<unsigned char>(metrics::matrix_set_transforms)).spc();
			out.p("c=").p_hex_8b(static_cast<unsigned char>(metrics::collisions_checks)).spc();
			out.p("b=").p_hex_8b(static_cast<unsigned char>(metrics::collisions_checks_bounding_shapes)).spc();
			out.p("f=").p_hex_8b(static_cast<unsigned char>(Object::free_ixes_i)).spc();
			out.p("u=").p_hex_8b(static_cast<unsigned char>(Object::used_ixes_i)).spc();
			out.p("t=").p_hex_16b(static_cast<unsigned short>(osca_t)).spc();
			out.p("s=").p_hex_8b(static_cast<unsigned char>(world::time_s)).spc();
			out.p("d=").p_hex_8b(static_cast<unsigned char>(world::time_dt_s*1000)).spc();

			if(!game::player_alive)
				shp=nullptr;

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
					shp->phy().vel=shp->forward_vector().scale(ship_speed);

				if(keyboard[key_a])
					shp->phy().dagl=-deg_to_rad(ship_dagl);

				if(keyboard[key_s])
					shp->phy().vel=shp->forward_vector().negate().scale(ship_speed);

				if(keyboard[key_d])
					shp->phy().dagl=deg_to_rad(ship_dagl);

				if(!keyboard[key_a]&&!keyboard[key_d])
					shp->phy().dagl=0;

				if(!keyboard[key_w]&&!keyboard[key_s])
					shp->phy().vel={0,0};

				if(keyboard[key_spc])
					shp->fire();
			}
			switch(table_scancode_to_ascii[osca_key]){
			case'x':
				if(game::enemies_alive==0)
					create_scene();
				break;
			case'c':
				if(shp)
					break;
				shp=new Ship;
				shp->phy().pos={160,130};
//					shp->phy().agl=deg_to_rad(180);
				game::player_alive=true;
				break;
			default:
				break;
			}

//			draw_axis(vga13h.bmp());

			osca_yield();
		}
	}

	static auto draw_axis(Bitmap&dsp){
		static AngleDeg deg=0;
		static Matrix2D R;
		if(deg>360)
			deg-=360;
		deg+=5;
		const AngleRad rotation=deg_to_rad(deg);
	//		shp3->set_angle(rotation);
		R.set_transform(5,rotation,{160,100});
		// dot axis
		dot(dsp,160,100,0xf);
		const Vector2D xaxis=R.axis_x().normalize().scale(7);
		dot(dsp,xaxis.x+160,xaxis.y+100,4);
		const Vector2D yaxis=R.axis_y().normalize().scale(7);
		dot(dsp,yaxis.x+160,yaxis.y+100,2);
	}
};

} // end namespace
