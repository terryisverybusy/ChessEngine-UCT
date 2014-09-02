#ifndef _NBRCOUNTER_H_
#define _NBRCOUNTER_H_
// class NbrCounter
/*
 * 用12个bit来表示其邻点中black,white,empty数量
 * 每4个bit表示一种情况的数量
 * off_board的情况则看做既是black也是white
 */
class NbrCounter {
public:
	uint8 bitfield;
	static const uint max = 4;                 // maximal number of neighbours
	static const uint cnt = 256;
	NbrCounter():bitfield(0x00) { }

	void player_inc(Player player, uint i) { 
		bitfield |= (player << i); 
	}
	void player_dec(Player player, uint i) { 
		bitfield &= ~(color::off_board << i); 
	}
	void off_board_inc(uint i) { 
		bitfield |= (color::off_board << i); 
	}
	static const uint cnt_map[player::cnt][cnt];
	static const uint eye_map[player::cnt][cnt];

	uint empty_cnt() const { return cnt_map[0][bitfield]; }
	uint player_cnt(Player pl) const { return cnt_map[pl][bitfield]; }
	uint player_cnt_is_max(Player pl) const { 
		return cnt_map[pl][bitfield] == max;
	}
	// diag_nbr less than 2 or 1
	uint is_nbr_lt2(Player pl) const { return eye_map[pl][bitfield]; }

	void check() {
		if(!nbr_cnt_ac) return;
		assert(empty_cnt() <= max);
		assert(player_cnt(player::black) <= max);
		assert(player_cnt(player::white) <= max);
	}
	static void output_cnt_map() {
		for(int i = 0; i < 3; i++) {
			printf("{");
			for(int j = 0; j < 256; j++) {
				if((j % 16) == 0) {
					printf("\n");
				}
				int count = 0;
				for(int n = 0; n < 8; n+= 2) {
					int b = ((j >> n) & 3);
					if(i == 0) {
						if(b == 0) count++;
					} else {
						if((b & i) == i) count++;
					}
				}
				printf("%d,",count);
			}
			printf("\n},\n");
		}
	}
	static void output_eye_map() {
		player_for_each(i) {
			printf("{");
			for(int j = 0; j < 256; j++) {
				if((j % 16) == 0) {
					printf("\n");
				}
				int count = 0;
				int off = 0;
				for(int n = 0; n < 8; n+= 2) {
					int b = ((j >> n) & 3);
					if(b == 0) {
					} else if(b == 3) {
						off = 1;
					} else if(b != i) {
						count++;
					}
				}
				printf("%d,",(count+off<2));
			}
			printf("\n},\n");
		}
	}
};
template<uint T, typename Derive>
class NbrCounterBoard {
public:
	FastMap<Vertex<T>, NbrCounter>  nbr_cnt; // incremental, for fast eye checking 每个点的邻点数
	FastMap<Vertex<T>, NbrCounter>  diag_nbr_cnt; // 每个点的肩
	NbrCounterBoard() {
		vertex_for_each_all(v) {
			// 先初始化棋盘外的点
			nbr_cnt[v] = NbrCounter();

			if(v.is_on_board()) {
				// 登记空点
				vertex_for_each_nbr2(v, nbr_v, i, {
					if(!nbr_v.is_on_board()) 
					nbr_cnt[v].off_board_inc(i);
				});
				// 更新肩
				vertex_for_each_diag_nbr2(v, nbr_v, i, {
					if(!nbr_v.is_on_board()) 
					diag_nbr_cnt[v].off_board_inc(i);
				});
			}
		}
	}
	void place_stone(Player pl, Vertex<T> v) {
		vertex_for_each_nbr(v, nbr_v,i, nbr_cnt[nbr_v].player_inc(pl,i));
		// 更新肩的信息
		//vertex_for_each_diag_nbr(v, nbr_v,i,diag_nbr_cnt[nbr_v].player_inc(pl,i));
	}
	void remove_stone(Player pl, Vertex<T> v) {
		//Derive* p = static_cast<Derive*>(this);
		vertex_for_each_nbr(v, nbr_v,i,nbr_cnt[nbr_v].player_dec(pl,i));
		// 更新肩
		//vertex_for_each_diag_nbr(v, nbr_v,i,diag_nbr_cnt[nbr_v].player_dec(pl,i));
	}
	void place_stone_side(Player pl, Vertex<T> v, uint i) {
		nbr_cnt[v].player_inc(pl,i);
	}
	void remove_stone_side(Player pl, Vertex<T> v, uint i) {
		nbr_cnt[v].player_dec(pl,i);
	}
	void check() const {
		check_nbr_cnt();
	}
private:
	void check_nbr_cnt() const { // 按照邻点的定义检查一遍nbr_cnt是否正确
		if(!board_nbr_cnt_ac) return;
		const Derive* p = static_cast<const Derive*>(this);

		vertex_for_each_all(v) {
			coord::t r;
			coord::t c;
			uint nbr_color_cnt[color::cnt];
			//uint expected_nbr_cnt;

			if(p->color_at[v] == color::off_board) continue; // TODO is that right?

			r = v.row();
			c = v.col();

			assert(coord::is_on_board<T> (r)); // checking the macro
			assert(coord::is_on_board<T> (c));
			color_for_each(col) nbr_color_cnt[col] = 0;

			vertex_for_each_nbr(v, nbr_v, i, nbr_color_cnt[p->color_at[nbr_v]]++);

			assert(nbr_cnt[v].player_cnt(player::black) == (nbr_color_cnt[color::black] + nbr_color_cnt[color::off_board]));
			assert(nbr_cnt[v].player_cnt(player::white) == (nbr_color_cnt[color::white] + nbr_color_cnt[color::off_board]));
			assert(nbr_cnt[v].empty_cnt() == (nbr_color_cnt[color::empty]));
		}
	}
};
#endif //_NBRCOUNTER_H_
