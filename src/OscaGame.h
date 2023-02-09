#pragma once
#include"osca.h"
#include"lib.h"
#include"lib2d.h"
#include"libge.h"

namespace osca{

namespace game{
	static bool player_alive=true;
}

static ObjectDef rectangle_def;
static ObjectDef ship_def;
static ObjectDef bullet_def;

class Wall:public Object{
public:
	// type bits 0b100 check collision with type 'bullet' 0b10
	Wall(const Scale scl,const Point2D&pos,const Angle agl):
		Object{0b100,0b10,rectangle_def,scl,pos,agl,3}
	{}

	constexpr virtual auto update()->bool override{
		if(phy().pos.y>130||phy().pos.y<70){
			phy().dpos.y=-phy().dpos.y;
		}
		return true;
	}

	// returns false if object is to be deleted
	constexpr virtual auto on_collision(Object&other)->bool override{
		return false; // collision with type 'bullet'
	}
};

class Bullet:public Object{
public:
	Bullet():
		// type bits 0b10 check collision with type 'wall' 0b100
		Object{0b10,0b100,bullet_def,.5f,{0,0},0,4}
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
		// type bits 0b1 check collision with nothing 'wall' 0b100
		Object{0b1,0b110,ship_def,4,{0,0},0,2}
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
		if(dt<5)
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

class OscaGame{
	auto create_scene(){
		for(float i=30;i<300;i+=20){
			Wall*w=new Wall(5,{i,90},deg_to_rad(i));
			w->phy().dagl=deg_to_rad(1);
			w->phy().dpos={0,.2f};
		}
	}
public:
	OscaGame(){
		//----------------------------------------------------------
		// init statics
		//----------------------------------------------------------
		// ? read from file
		rectangle_def={5,4,
			new Point2D[]{ // points in model coordinates, negative Y is "forward"
				{ 0,0},
				{-1,-.5f},
				{-1, .5f},
				{ 1, .5f},
				{ 1,-.5f},
			},
			new PointIx[]{1,2,3,4} // bounding convex polygon CCW
		};
		rectangle_def.init_normals();

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

		bullet_def={4,3,
			new Point2D[]{
				{ 0,-1},
				{-1,.5},
				{ 1,.5},
			},
			new PointIx[]{0,1,2} // bounding convex polygon CCW
		};
		bullet_def.init_normals();
	}

	[[noreturn]] auto start(){
		// init stack
		const Address heap_address=Heap::data().address();
		const Address heap_disp_at_addr=vga13h.bmp().data().pointer().offset(50*320).address();
		const SizeBytes heap_disp_size=320*100;

		Ship*shp=new Ship;
		shp->phy().pos={160,130};

//		Wall*w=new Wall(5,{160,90},0);
//		w->phy().dagl=deg_to_rad(1);

		create_scene();

	//	out.p_hex_16b(static_cast<unsigned short>(sizeof(Object))).pos({1,2});

		// start task
		while(true){
			metrics::reset();

			// copy heap to screen
			pz_memcpy(heap_disp_at_addr,heap_address,heap_disp_size);

			out.pos({0,1});//.p("                                                            ").pos({0,1});

			PhysicsState::update_physics_states();
			Object::render_all(vga13h.bmp());
			Object::update_all();
			Object::check_collisions();
			if(!game::player_alive)
				shp=nullptr;

			//		out.pos({0,2}).p("                                              ").pos({0,2});
			out.pos({19,2}).fg(2).p("m=").p_hex_16b(metrics::matrix_set_transforms);
			out.pos({26,2}).fg(2).p("c=").p_hex_16b(metrics::collisions_checks);
			out.pos({33,2}).fg(2).p("b=").p_hex_16b(metrics::collisions_checks_bounding_shapes);
			out.pos({40,2}).fg(2).p("f=").p_hex_8b(static_cast<unsigned char>(Object::free_ixes_i));
			out.pos({45,2}).fg(2).p("u=").p_hex_8b(static_cast<unsigned char>(Object::used_ixes_i));
			out.pos({50,2}).fg(2).p("t=").p_hex_32b(osca_t);

			if(shp)
				shp->phy().dpos={0,0};

			const char ch=table_scancode_to_ascii[osca_key];
			if(shp){
				switch(ch){
				case'w':
					if(game::player_alive)
						shp->phy().dpos=shp->forward_vector().scale(.3f);
					break;
				case'a':
					if(game::player_alive)
						shp->phy().agl-=deg_to_rad(2);
					break;
				case's':
					if(game::player_alive)
						shp->phy().dpos=shp->forward_vector().negate().scale(.3f);
					break;
				case'd':
					if(game::player_alive)
						shp->phy().agl+=deg_to_rad(2);
					break;
				case' ':
					if(game::player_alive)
						shp->fire();
					break;
				default:
					break;
				}
			}
			switch(ch){
			case'x':
				create_scene();
				break;
			case'c':
				if(Object::hasFreeSlot()){
					if(shp)
						break;
					shp=new Ship;
					shp->phy().pos={160,130};
//					shp->phy().agl=deg_to_rad(180);
					game::player_alive=true;
				}else{
					err.p("out of free slots");
					osca_halt();
				}
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
