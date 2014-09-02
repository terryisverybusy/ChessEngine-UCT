#ifndef _BASIC_GO_TYPES_H_
#define _BASIC_GO_TYPES_H_

/*
 * 包含了基本的Player,Color,coord,Vertex,Move类
 */

//------------------------------------------------------------------------------
/*
 * Player,使用位运算加速计算另一方:other
 */
//Player
namespace player {

	//typedef uint t;
	enum t{ black = 1, white = 2, wrong = 3};
	const uint cnt = 3;

	inline t other(t pl) {
		//return t(pl ^ 3);//TODO: 究竟是位运算快还是if else快？
		if(pl == black) return white;
		else return black;
	}
	inline bool in_range(t pl) {
		return pl < wrong;
	}
	inline t operator++(t& pl) { return (pl = t(pl+1)); }
	inline void check(t pl) {
		assertc (player_ac, (pl & (~1)) == 0);
	}
};
typedef player::t Player;
// faster than non-loop
#define player_for_each(pl) \
	for (Player pl = player::black; player::in_range(pl); ++pl)

//------------------------------------------------------------------------------
/*
 * Color与Player对应，但是多了几种状态
 *
 */
// class color
namespace color {
	typedef uint t;
	enum {
		empty = 0, 
		black = 1, 
		white = 2, 
		off_board = 3, 
		wrong = 4
	};
	const int is_player_map[] = {
		0,1,1,0,0
	};
	const int not_player_map[] = {
		1,0,0,1,1
	};
	const int cnt = 4;
	inline void check(t cl) {
		assertc (color_ac, (cl & (~3)) == 0); 
	}
	inline bool is_player(t cl) {
		//return cl <= white;
		//return cl > 0;
		return is_player_map[cl]; 
	}
	inline bool is_not_player(t cl) {
		//return cl > white;
		//return cl == 0;
		return not_player_map[cl]; 
	}
	inline t from_player(Player pl) {
		return (t)(pl);
	}
	inline Player to_player(t cl) {
		return (Player)(cl);
	}
	inline bool in_range(t cl) {
		return cl < cnt;
	}
	//inline t operator++(t& pl) { return (pl = t(pl+1));}
}
typedef color::t Color;
// TODO test it for performance
#define color_for_each(col) \
	for (Color col = 0; color::in_range(col); ++col)

/*
 * 坐标类，坐标由两个int值表示，-1表示棋盘外的坐标
 */
namespace coord { // TODO class

	typedef int t;

	template<uint T> bool is_ok (t coord) { return (coord < int (T)) & (coord >= -1); }
	template<uint T> bool is_on_board (t coord) { return uint (coord) < T; }

	template<uint T> void check (t coo) { 
		unused (coo);
		assertc (coord_ac, is_ok<T> (coo)); 
	}

	template<uint T> void check2 (t row, t col) { 
		if (!coord_ac) return;
		if (row == -1 && col == -1) return;
		assertc (coord_ac, is_on_board<T> (row)); 
		assertc (coord_ac, is_on_board<T> (col)); 
	}

}
#define coord_for_each(rc) \
	for (coord::t rc = 0; rc < int(T); rc++)
//for (coord::t rc = 0; rc < int(T); rc = coord::t (rc+1))

//--------------------------------------------------------------------------------

/*
 * 棋盘上的交叉点，参数T是棋盘大小，用一个uint存储
 */
template<uint T> 
class Vertex {

	//static_assert (cnt <= (1 << bits_used));
	//static_assert (cnt > (1 << (bits_used-1)));

	uint idx;

public:

	const static uint dNS = (T + 2);
	const static uint dWE = 1;

	const static uint bits_used = 9;     // on 19x19 cnt == 441 < 512 == 1 << 9;
	const static uint pass_idx = 0;
	const static uint any_idx  = 1; // TODO any
	const static uint resign_idx = 2;

	const static uint cnt = (T + 2) * (T + 2);

	explicit Vertex () { } // TODO is it needed
	explicit Vertex (uint _idx) { idx = _idx; }

	operator uint() const { return idx; }

	bool operator== (Vertex other) const { return idx == other.idx; }
	bool operator!= (Vertex other) const { return idx != other.idx; }

	bool in_range ()          const { return idx < cnt; }
	Vertex& operator++() { ++idx; return *this;}

	void check ()             const { assertc (vertex_ac, in_range ()); }

	coord::t row () const { return idx / dNS - 1; }

	coord::t col () const { return idx % dNS - 1; }

	//TODO:this usualy can be achieved quicker by color_at lookup
	bool is_on_board () const {
		return coord::is_on_board<T> (row ()) & coord::is_on_board<T> (col ());
	}

	void check_is_on_board () const { 
		assertc (vertex_ac, is_on_board ()); 
	}

	Vertex N () const { return Vertex (idx - dNS); }
	Vertex W () const { return Vertex (idx - dWE); }
	Vertex E () const { return Vertex (idx + dWE); }
	Vertex S () const { return Vertex (idx + dNS); }

	/*
	   Vertex NW () const { return N ().W (); } // TODO can it be faster?
	   Vertex NE () const { return N ().E (); } // only Go
	   Vertex SW () const { return S ().W (); } // only Go
	   Vertex SE () const { return S ().E (); }
	   */
	Vertex NW () const { return Vertex (idx - dNS - dWE); }
	Vertex NE () const { return Vertex (idx - dNS + dWE); }
	Vertex SW () const { return Vertex (idx - dWE + dNS); }
	Vertex SE () const { return Vertex (idx + dWE + dNS); }


	static Vertex pass   () { return Vertex (Vertex::pass_idx); }
	static Vertex any    () { return Vertex (Vertex::any_idx); }
	static Vertex resign () { return Vertex (Vertex::resign_idx); }
	// TODO make this constructor a static function
	/*
	   Vertex (coord::t r, coord::t c) {
	   coord::check2<T> (r, c);
	   idx = (r+1) * dNS + (c+1) * dWE;
	   }
	   */
	static Vertex of_coords(coord::t r, coord::t c) {
		coord::check2<T> (r, c);
		return Vertex((r+1) * dNS + (c+1) * dWE);
	}

};



#define vertex_for_each_all(vv) \
	for (Vertex<T> vv = Vertex<T>(0); vv.in_range (); ++vv) // TODO 0 works??? // TODO player the same way!

// misses some offboard vertices (for speed) 
#define vertex_for_each_faster(vv)                                  \
	for (Vertex<T> vv = Vertex<T>(Vertex<T>::dNS+Vertex<T>::dWE);                 \
		vv <= T * (Vertex<T>::dNS + Vertex<T>::dWE);   \
	++vv)

#define vertex_for_each_far_nbr(center_v, d, nbr_v, block) { \
	Vertex<T> nbr_v; \
	Vertex<T> vv1,vv2;\
	coord::t row = center_v.row();\
	coord::t col = center_v.col();\
	coord::t left = max(col - d, 0);\
	coord::t right = min(col + d, int(T) - 1);\
	coord::t top = max(row - d, 0);\
	coord::t bottom = min(row + d, int(T) - 1);\
	Vertex<T> tl = Vertex<T>::of_coords(top, left);\
	Vertex<T> tr = Vertex<T>::of_coords(top, right);\
	Vertex<T> bl = Vertex<T>::of_coords(bottom, left);\
	Vertex<T> br = Vertex<T>::of_coords(bottom, right);\
	for (vv1 = tl, vv2 = tr; vv1 <= bl; vv1 = vv1.S(), vv2 = vv2.S()) {\
		for (nbr_v = vv1; nbr_v <= vv2; ++nbr_v) {\
			block;\
		}\
	}\
}

/*
 * 邻点
 */
#define vertex_for_each_nbr(center_v, nbr_v,i, block) {       \
	center_v.check_is_on_board ();                      \
	Vertex<T> nbr_v;                                    \
	uint i;						\
	nbr_v = center_v.N (); i=4;block;                       \
	nbr_v = center_v.W (); i=6;block;                       \
	nbr_v = center_v.S (); i=0;block;                       \
	nbr_v = center_v.E (); i=2;block;                       \
}
/*
 * 邻点(但是i为正序)
 */
#define vertex_for_each_nbr2(center_v, nbr_v,i, block) {       \
	center_v.check_is_on_board ();                      \
	Vertex<T> nbr_v;                                    \
	uint i;						\
	nbr_v = center_v.N (); i=0;block;                       \
	nbr_v = center_v.W (); i=2;block;                       \
	nbr_v = center_v.S (); i=4;block;                       \
	nbr_v = center_v.E (); i=6;block;                       \
}

/*
 * 对角邻点
 */
#define vertex_for_each_diag_nbr(center_v, nbr_v, i, block) {      \
	center_v.check_is_on_board ();                          \
	Vertex<T> nbr_v;                                        \
	uint i;						\
	nbr_v = center_v.NW (); i=4;block;                          \
	nbr_v = center_v.NE (); i=6;block;                          \
	nbr_v = center_v.SE (); i=0;block;                          \
	nbr_v = center_v.SW (); i=2;block;                          \
}
#define vertex_for_each_diag_nbr2(center_v, nbr_v, i, block) {      \
	center_v.check_is_on_board ();                          \
	Vertex<T> nbr_v;                                        \
	uint i;						\
	nbr_v = center_v.NW (); i=0;block;                          \
	nbr_v = center_v.NE (); i=2;block;                          \
	nbr_v = center_v.SE (); i=4;block;                          \
	nbr_v = center_v.SW (); i=6;block;                          \
}

#define player_vertex_for_each_9_nbr(center_v, pl, nbr_v, i) {      \
	v::check_is_on_board (center_v);                            \
	Move    nbr_v;                                              \
	player_for_each (pl) {                                      \
		nbr_v = center_v;                                   \
		i;                                                  \
		vertex_for_each_nbr      (center_v, nbr_v, i);      \
		vertex_for_each_diag_nbr (center_v, nbr_v, i);      \
	}                                                           \
}
#define vertex_for_each_8_nbr(center_v, nbr_v, i, block) {      	    \
	center_v.check_is_on_board ();                            \
	Vertex<T> nbr_v;                                    \
	uint i;						\
	nbr_v = center_v.N (); i=0;block;                       \
	nbr_v = center_v.W (); i=1;block;                       \
	nbr_v = center_v.S (); i=0;block;                       \
	nbr_v = center_v.E (); i=1;block;                       \
	nbr_v = center_v.NW (); i=2;block;                          \
	nbr_v = center_v.NE (); i=3;block;                          \
	nbr_v = center_v.SE (); i=2;block;                          \
	nbr_v = center_v.SW (); i=3;block;                          \
}
//--------------------------------------------------------------------------------

/*
 * 用一个uint表示一次Move,即在Vertex前面再加上一个表示Player的bit
 */
template<uint T> class Move {
public:

	//const static uint cnt = player::white << Vertex<T>::bits_used | Vertex<T>::cnt;
	const static uint cnt = player::black << Vertex<T>::bits_used | Vertex<T>::cnt;

	const static uint no_move_idx = 1;

	uint idx;

	void check () {
		Player ((idx >> Vertex<T>::bits_used) + 1);
		Vertex<T> (idx & ((1 << Vertex<T>::bits_used) - 1)).check ();
	}

	explicit Move (Player player, Vertex<T> v) { 
		//idx = (player << Vertex<T>::bits_used) | v;
		idx = ((player>>1) << Vertex<T>::bits_used) | v;
	}

	explicit Move () {
		Move (player::black, Vertex<T>::any ());
	}

	explicit Move (int idx_) {
		idx = idx_;
	}

	Player get_player () { 
		return Player ((idx >> Vertex<T>::bits_used) + 1);
	}

	Vertex<T> get_vertex () { 
		return Vertex<T> (idx & ((1 << ::Vertex<T>::bits_used) - 1)) ; 
	}


	operator uint() const { return idx; }

	bool operator== (Move other) const { return idx == other.idx; }
	bool operator!= (Move other) const { return idx != other.idx; }

	bool in_range ()          const { return idx < cnt; }
	Move& operator++() { ++idx; return *this;}
};

    

#define move_for_each_all(m) for (Move m = Move (0); m.in_range (); ++m)

/********************************************************************************/
#endif // _BASIC_GO_TYPES_H_
