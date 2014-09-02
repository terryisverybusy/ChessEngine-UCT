#ifndef _ZOBRIST_H_
#define _ZOBRIST_H_

// class Hash
class Hash {
public:
	uint64 hash;
	Hash() { }
	uint index() const { return hash; }
	uint lock() const { return hash >> 32; }
	void randomize(PmRandom& pm) { 
		hash =
			(uint64(pm.rand_int()) << (0*16)) ^
			(uint64(pm.rand_int()) << (1*16)) ^ 
			(uint64(pm.rand_int()) << (2*16)) ^ 
			(uint64(pm.rand_int()) << (3*16));
	}
	void set_zero() { hash = 0; }
	bool operator== (const Hash& other) const { return hash == other.hash; }
	void operator^= (const Hash& other) { hash ^= other.hash; }
};
// class Zobrist
template<uint T> 
class Zobrist {
public:

	FastMap<Move<T>, Hash> hashes;
	Zobrist(PmRandom& pm) {
		player_for_each(pl) vertex_for_each_all(v) {
			Move<T> m = Move<T> (pl, v);
			hashes [m].randomize(pm);
		}
	}
	Hash of_move(Move<T> m) const { return hashes [m]; }
	Hash of_pl_v(Player pl,  Vertex<T> v) const { return hashes [Move<T> (pl, v)];}
};

template<uint T, typename Derive>
class ZobristBoard {
	static const Zobrist<T> zobrist[1];
public:
	Hash                         hash;
	ZobristBoard() {
		hash = recalc_hash();
	}
	Hash recalc_hash() const { // 将所有的移动(Move)异或起来(^=)
		const Derive* const p = static_cast<const Derive*>(this);
		Hash new_hash;
		new_hash.set_zero();
		vertex_for_each_all(v) {
			if(color::is_player(p->color_at [v])) {
				new_hash ^= zobrist->of_pl_v(color::to_player(p->color_at[v]), v);
			}
		}
		return new_hash;
	}
	void check() const {
		assertc(board_hash_ac, hash == recalc_hash());
	}
	void place_stone(Player pl, Vertex<T> v) {
		hash ^= zobrist->of_pl_v(pl, v);
	}
	void remove_stone(Player pl, Vertex<T> v) {
		Derive* p = static_cast<Derive*>(this);
		hash ^= zobrist->of_pl_v(pl, v);
	}
};
extern PmRandom zobrist_pm;
template<uint T, typename Derive> 
const Zobrist<T> ZobristBoard<T, Derive>::zobrist[1] = { Zobrist<T> (zobrist_pm) }; // TODO move it to board
#endif //_ZOBRIST_H_
