typedef void*Addr;
typedef int Size;

class Ref{
	Addr addr;
public:
	inline Ref(Addr a):addr{a}{}
	inline auto get_addr()const->Addr{return addr;}
	inline auto write_byte(char b)->void{*reinterpret_cast<char*>(addr)=b;}
	inline auto write_int(int i)->void{*reinterpret_cast<int*>(addr)=i;}
	inline auto get_offset_ref(Size offset_B)->Ref{return Ref{reinterpret_cast<char*>(addr)+offset_B};}
};

class File:public Ref{
	Size size_B;
public:
	inline File(Addr a,Size nbytes):
		Ref{a},size_B{nbytes}
	{}
	auto to(File f)->void;
	auto to(File f,Size nbytes)->void;
	inline auto get_size_B()const->Size{return size_B;}
};

typedef int Coord;

class Coords{
	Coord x;
	Coord y;
public:
	inline Coords(Coord x_,Coord y_):x{x_},y{y_}{}
	inline auto get_x()const->Coord{return x;}
	inline auto get_y()const->Coord{return y;}
	inline auto set_x(Coord x_){x=x_;}
	inline auto set_y(Coord y_){y=y_;}
	inline auto set(Coord x_,Coord y_){set_x(x_);set_y(y_);}
};

typedef int Width;
typedef int Height;

class Bitmap:public File{
	Width width_px;
	Height height_px;
public:
	inline Bitmap(Addr a,Width w_px,Height h_px):
		File{a,w_px*h_px},
		width_px{w_px},height_px{h_px}
	{}
	inline auto get_width_px()const->Width{return width_px;}
	inline auto get_height_px()const->Height{return height_px;}
	auto to(Bitmap&b,const Coords&c)->void;
};
