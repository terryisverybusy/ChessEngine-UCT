#ifndef _RENJU_BOARD_H_
#define _RENJU_BOARD_H_
#include <math.h>
#include "basic_go_types.h"
#include "basic_board.h"
#include "zobrist.h"
#include "nbrcounter.h"

template<uint T> 
class RenjuBoard:
public BasicBoard<T, RenjuBoard<T> >,
public NbrCounterBoard<T, RenjuBoard<T> >
{
public:
	static const bool use_mercy_rule = true;
	using BasicBoard<T, RenjuBoard>::empty_v_cnt;
	using BasicBoard<T, RenjuBoard>::color_at;
	using BasicBoard<T, RenjuBoard>::board_area;
	using BasicBoard<T, RenjuBoard>::empty_v;
	using BasicBoard<T, RenjuBoard>::player_v_cnt;
	using BasicBoard<T, RenjuBoard>::move_history;
	using BasicBoard<T, RenjuBoard>::move_no;
	using BasicBoard<T, RenjuBoard>::max_game_length;
	using NbrCounterBoard<T, RenjuBoard>::nbr_cnt;
	using NbrCounterBoard<T, RenjuBoard>::diag_nbr_cnt;

	int                          komi;
	//FastMap<Vertex<T>, Vertex<T> >  chain_next_v;
	Vertex<T> chain_next_v[4][Vertex<T>::cnt];
	uint chain_length[4][Vertex<T>::cnt]; // indexed by chain_id 棋串的长度
	uint chain_id[4][Vertex<T>::cnt]; // 每个点所属的4个方向上的棋串id
	FastMap<Vertex<T>, bool>        good_at; // 是否good 
	FastMap<Vertex<T>, uint>        good_pos; // good vetex在good_v中的索引
	Vertex<T>                       good_v[board_area]; // 棋盘上的空点
	uint                         	good_v_cnt;
#undef empty_v_for_each_and_pass
  #define empty_v_for_each_and_pass(board, vv, i) {                    \
      Vertex<T> vv = Vertex<T>::pass();                                \
      i;                                                               \
      rep(ev_i, (board)->good_v_cnt) {                                \
        vv = (board)->good_v[ev_i];                                  \
        i;                                                             \
      }                                                                \
    }
public:                         // board interface
	RenjuBoard() {
		vertex_for_each_all(v) {
			rep(i, 4) {
				chain_next_v[i][v] = v;
				chain_id[i][v] = v;
				chain_length[i][v] = 0;
			}
			good_at[v] = false;
		}
		good_v_cnt  = 0;
		Vertex<T> v = Vertex<T>::of_coords(T/2,T/2);
		good_at[v] = true;
		good_pos[v] = good_v_cnt;
		good_v[good_v_cnt++] = v;
		komi = 0;
	}
	all_inline 
	bool play(Player player, Vertex<T> v) {
		check();
		if(v == Vertex<T>::pass()) {
			basic_play(player, Vertex<T>::pass());
			return true;
		}
		v.check_is_on_board();
		assertc(board_ac, color_at[v] == color::empty);
		basic_play(player, v);
		place_stone(player, v);
		rep(i, 4) {
			//chain_length[i][chain_id[i][v]] = 1;
			chain_length[i][v] = 1;
		}
		vertex_for_each_8_nbr(v, nbr_v, i, {
			if(color_at[nbr_v] == color::from_player(player)) { // same color of groups
				assertc(board_ac, chain_id[i][nbr_v] != chain_id[i][v]);
				if(chain_id[i][nbr_v] != chain_id[i][v]) {
					if(chain_length[i][chain_id[i][v]] > chain_length[i][chain_id[i][nbr_v]]) {
						merge_chains(v, nbr_v, i);
					} else {
						merge_chains(nbr_v, v, i);
					}         
				}
			}
		});
		if(komi == 0) {
			rep(i, 4) {
				if(chain_length[i][chain_id[i][v]] >= 5) {
					komi = (player == player::black) ? 500 : -500;
					break;
				}
			}
		}
		return true;
	}
	int score() const {
		return komi;
	}
	Player winner() const { 
		return Player((score() <= 0) + 1); 
	}
	int approx_score() const {
		return komi;
	}
	Player approx_winner() { return Player((approx_score() <= 0)+1); }
	int vertex_score(Vertex<T> v) {
		return 0;
	}
	void set_komi(float fkomi) { 
	}
	float get_komi() const { 
		return komi;
	}
public: // legality functions
private:
	void basic_play(Player player, Vertex<T> v) { // Warning: has to be called before place_stone, because of hash storing
		BasicBoard<T, RenjuBoard>::play(player, v);
	}

	void place_stone(Player player, Vertex<T> v) {
		BasicBoard<T, RenjuBoard>::place_stone(player, v);
		NbrCounterBoard<T, RenjuBoard>::place_stone(player, v);
		if(good_at[v]) {
			good_v_cnt--;
			good_pos[good_v[good_v_cnt]] = good_pos[v];
			good_v[good_pos[v]] = good_v[good_v_cnt];
		}
		vertex_for_each_far_nbr(v, 1, nbr_v, {
			if(!good_at[nbr_v] && color_at[nbr_v] == color::empty) {
				good_at[nbr_v] = true;
				good_pos[nbr_v] = good_v_cnt;
				good_v[good_v_cnt++] = nbr_v;
			}
		});
	}
	void remove_stone(Player pl, Vertex<T> v) {
		// 按照place_stone相反的顺序
		NbrCounterBoard<T, RenjuBoard>::remove_stone(pl, v);
		BasicBoard<T, RenjuBoard>::remove_stone(pl, v);
	}
	void merge_chains(Vertex<T> v_base, Vertex<T> v_new, uint i) {
		Vertex<T> act_v;

		chain_length[i][chain_id[i][v_base]] += chain_length[i][chain_id[i][v_new]];

		act_v = v_new;
		do {
			chain_id[i][act_v] = chain_id[i][v_base];
			act_v = chain_next_v[i][act_v];
		} while(act_v != v_new);

		swap(chain_next_v[i][v_base], chain_next_v[i][v_new]);// 两个环断开，分别接到对方环上，扭成一个大环
	}

public:                         // consistency checks
	void check() const { // 检查整个棋盘一致性
		if(!board_ac) return;
		BasicBoard<T, RenjuBoard>::check();
		NbrCounterBoard<T, RenjuBoard>::check();
	}
};
#endif //_RENJU_BOARD_H_
