#pragma once
#include"osca.h"
#include"lib.h"
#include"lib2d.h"
#include"libge.h"

namespace osca{

namespace game{
	static bool player_alive=true;
	static Size enemies_alive=0;
}

static ObjectDef enemy_def;
static ObjectDef ship_def;
static ObjectDef bullet_def;
static ObjectDef wall_def;
static ObjectDef missile_def;

class Enemy:public Object{
public:
	// type bits 0b100 check collision with
	// 'bullet'  0b0'0010
	Enemy(const Point2D&pos,const Angle agl):
		Object{0b100,0b10,enemy_def,5,pos,agl,3}
	{
		game::enemies_alive++;
	}

	constexpr virtual auto update()->bool override{
		if(phy().pos.y>130||phy().pos.y<70){
			phy().dpos.y=-phy().dpos.y;
		}
		return true;
	}

	// returns false if object is to be deleted
	virtual auto on_collision(Object&other)->bool override{
		game::enemies_alive--;
		return false; // collision with type 'bullet'
	}
};

class Bullet:public Object{
public:
	Bullet():
		// type bits 0b10 check collision with
		// 'ship'    0b0'0001
		// 'enemies' 0b0'0100
		// 'walls'   0b0'1000
		// 'missiles'0b1'0000
		Object{0b10,0b1'1101,bullet_def,1,{0,0},0,4}
	{}
	constexpr virtual auto update()->bool override{
		Object::update();
		if(phy().pos.x>300){
			return false;
		}
		if(phy().pos.x<20){
			return false;
		}
		if(phy().pos.y>130){
			return false;
		}
		if(phy().pos.y<70){
			return false;
		}
		return true;
	}
	// returns false if object is to be deleted
	constexpr virtual auto on_collision(Object&other)->bool override{
		return false;
	}
};

class Ship:public Object{
	unsigned fire_t=0;
public:
	Ship():
		// type bits 0b1 check collision with:
		// 'bullets' 0b0'0010
		// 'enemies' 0b0'0100
		// 'walls'   0b0'1000
		// 'missiles'0b1'0000
		Object{0b1,0b1'1110,ship_def,4,{0,0},0,2}
	{}

	constexpr virtual auto update()->bool override{
		Object::update();
		if(phy().pos.x>300||phy().pos.x<20){
			phy().dpos.x=-phy().dpos.x;
		}
		if(phy().pos.y>130||phy().pos.y<70){
			phy().dpos.y=-phy().dpos.y;
		}
		return true;
	}

	// returns false if object is to be deleted
	virtual auto on_collision(Object&other)->bool override{
		// collision with 'wall'
		game::player_alive=false;
		return false;
	}

	auto fire(){
		const unsigned dt=osca_t-fire_t;
		if(dt<3)
			return;
		fire_t=osca_t;
		Bullet*b=new Bullet;
		Vector2D v=forward_vector();
		v.scale(scl_); // place bullet in front of ship
		b->phy().pos=phy().pos+v;
		b->phy().dpos=v.normalize().scale(3);
		b->phy().agl=phy().agl;
	}
};


class Wall:public Object{
public:
	// type bits 0b100 check collision with nothing
	Wall(const Scale scl,const Point2D&pos,const Angle agl):
		Object{0b0'1000,0,wall_def,scl,pos,agl,3}
	{}
};


class Missile:public Object{
public:
	Missile():
		Object{0b1'0000,0b1111,missile_def,2,{0,0},0,4}
	{}
	constexpr virtual auto update()->bool override{
		Object::update();
		if(phy().pos.x>300){
			return false;
		}
		if(phy().pos.x<20){
			return false;
		}
		if(phy().pos.y>130){
			return false;
		}
		if(phy().pos.y<70){
			return false;
		}
		return true;
	}
	// returns false if object is to be deleted
	constexpr virtual auto on_collision(Object&other)->bool override{
		return false;
	}
};

class OscaGame{
	auto create_scene(){
		for(float i=30;i<300;i+=20){
			Enemy*w=new Enemy({i,90},deg_to_rad(i));
			w->phy().dagl=deg_to_rad(1);
			w->phy().dpos={0,.2f};
		}
	}
	auto create_scene2(){
		new Wall(20,{160,100},0);
	}
	auto create_scene3(){
		new Enemy({160,100},0);
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

		bullet_def={1,1,
			new Point2D[]{
				{0,0},
			},
			new PointIx[]{} // bounding convex polygon CCW
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
	}

	[[noreturn]] auto start(){
		// init stack
		const Address heap_address=Heap::data().address();
		const Address heap_disp_at_addr=vga13h.bmp().data().pointer().offset(50*320).address();
		const SizeBytes heap_disp_size=320*100;


		Ship*shp=new Ship;
		shp->phy().pos={160,130};
//		create_scene();
		create_scene();

//		Ship*shp=nullptr;
	//	out.p_hex_16b(static_cast<unsigned short>(sizeof(Object))).pos({1,2});

		constexpr unsigned char key_w=0;
		constexpr unsigned char key_a=1;
		constexpr unsigned char key_s=2;
		constexpr unsigned char key_d=3;
		constexpr unsigned char key_spc=4;
		bool keyboard[]{false,false,false,false,false}; // wasd and space pressed status
		// start task
		while(true){
			metrics::reset();

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
			out.p("t=").p_hex_16b(static_cast<unsigned short>(osca_t));
//			shp->phy().agl+=deg_to_rad(1);
//			shp->fire();
//			osca_yield();
//			continue;

			if(!game::player_alive)
				shp=nullptr;

			const char ch=table_scancode_to_ascii[osca_key];
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
					shp->phy().dpos=shp->forward_vector().scale(.3f);

				if(keyboard[key_a])
					shp->phy().dagl=-deg_to_rad(2);

				if(keyboard[key_s])
					shp->phy().dpos=shp->forward_vector().negate().scale(.3f);

				if(keyboard[key_d])
					shp->phy().dagl=deg_to_rad(2);

				if(!keyboard[key_a]&&!keyboard[key_d])
					shp->phy().dagl=0;

				if(!keyboard[key_w]&&!keyboard[key_s])
					shp->phy().dpos={0,0};

				if(keyboard[key_spc])
					shp->fire();
			}
			switch(ch){
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

	static auto draw_axis(Bitmap&dsp)->void{
		static Degrees deg=0;
		static Matrix2D R;
		if(deg>360)
			deg-=360;
		deg+=5;
		const float rotation=deg_to_rad(deg);
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
