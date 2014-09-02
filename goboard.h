#ifndef _GO_BOARD_H_
#define _GO_BOARD_H_
#include <math.h>
#include "basic_go_types.h"
#include "basic_board.h"
#include "zobrist.h"
#include "nbrcounter.h"

enum play_ret_t { play_ok, play_suicide, play_ss_suicide, play_ko };
template<uint T> 
class GoBoard:
public BasicBoard<T, GoBoard<T> >,
public NbrCounterBoard<T, GoBoard<T> >,
public ZobristBoard<T, GoBoard<T> >
{
public:
	static const bool use_mercy_rule = false;
	typedef NbrCounterBoard<T, GoBoard> NCB;
	using BasicBoard<T, GoBoard>::empty_v_cnt;
	using BasicBoard<T, GoBoard>::color_at;
	using BasicBoard<T, GoBoard>::empty_v;
	using BasicBoard<T, GoBoard>::player_v_cnt;
	using BasicBoard<T, GoBoard>::move_history;
	using BasicBoard<T, GoBoard>::move_no;
	using BasicBoard<T, GoBoard>::max_game_length;
	using NbrCounterBoard<T, GoBoard>::nbr_cnt;
	//using NbrCounterBoard<T, GoBoard>::diag_nbr_cnt;
	using ZobristBoard<T, GoBoard>::hash;
public:
	FastMap<Vertex<T>, Vertex<T> >  chain_next_v;
	uint                         	chain_lib_cnt[Vertex<T>::cnt]; // indexed by chain_id 棋串的气数
	FastMap<Vertex<T>, uint>        chain_id; // 每个点所属的棋串id
  
	int                          komi;
	Vertex<T>                    ko_v;             // vertex forbidden by ko(劫)
	uint                         last_empty_v_cnt;
	play_ret_t                   last_move_status;
public:                         // board interface
	GoBoard() {
		last_move_status = play_ok;
		ko_v         = Vertex<T>::any();
		set_komi(default_komi);            // standard for chinese rules
		vertex_for_each_all(v) {
			chain_next_v[v] = v;
			chain_id[v] = v;    // TODO is it needed, is it usedt?
			//chain_lib_cnt[v] = NbrCounter::max; // TODO is it logical? (off_boards)
			chain_lib_cnt[v] = nbr_cnt[v].empty_cnt();
		}
	}
	void set_komi(float fkomi) { 
		komi = int(ceil(-fkomi));
	}
	float get_komi() const { 
		return float(-komi) + 0.5;
	}
	// deprecated
	// checks for move legality
	// it has to point to empty vertexand empty
	// can't recognize play_suicide
	all_inline 
	bool is_legal(Player player, Vertex<T> v) { // 不严格的禁入点探测
		check();
		//v.check_is_on_board(); // TODO check v = pass || onboard

		// 1 pass
		// 2 不在对方包围中(含真眼和假眼)
		// 3 不是劫并且不是填对方的单眼
		return 
			v == Vertex<T>::pass() || 
			!nbr_cnt[v].player_cnt_is_max(player::other(player)) || 
			(eye_is_not_ko(v) && //(v != ko_v && 
			 !eye_is_suicide(v));
	}
public: // legality functions
	// MC模拟时仅考虑不填自己的眼,对于填自己空或者缩小眼位之类的则不管
	bool is_eyelike(Player player, Vertex<T> v) { // 是否像一个眼
		// 1 邻点是否全是己方棋子
		// 2 肩上的对方棋子数少于2(边角上少于1)
		// 缺点：无法判断环形棋块,
		// TODO:是否会错过故意填眼的妙招??
		assertc(board_ac, color_at[v] == color::empty);
		if(! nbr_cnt[v].player_cnt_is_max(player)) return false;
#if 1
		int diag_color_cnt[4]; // TODO
		color_for_each (col) 
			diag_color_cnt [col] = 0; // memset is slower

		vertex_for_each_diag_nbr (v, diag_v, i, {
				diag_color_cnt [color_at [diag_v]]++;
				});

		return diag_color_cnt [Color (player::other(player))] + (diag_color_cnt [color::off_board] > 0) < 2;
#else
		return diag_nbr_cnt[v].is_nbr_lt2(player);
#endif
	}
	void check_no_more_legal(Player player) { // at the end of the playout
		unused(player);
		if(!board_ac) return;
		vertex_for_each_all(v) // 确认对于该player来说，剩余的空点要么是眼，要么禁入
			if(color_at[v] == color::empty)
				assert(is_eyelike(player, v) || is_legal(player, v) == false);
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

		if(nbr_cnt[v].player_cnt_is_max(player::other(player))) {
			return play_eye(player, v);
#if 1 
		} else if(nbr_cnt[v].empty_cnt() == 0) { // 处理可能的自杀
			// complete suicide testing
			vertex_for_each_nbr(v, nbr_v,i, chain_lib_cnt[chain_id[nbr_v]]--); 
			uint valid = false;
			vertex_for_each_nbr(v, nbr_v,i, {
				valid |= !((color_at[nbr_v] != color::from_player(player)) ^ (chain_lib_cnt[chain_id[nbr_v]] == 0));
			});
			if(!valid) {
				vertex_for_each_nbr(v, nbr_v,i, chain_lib_cnt[chain_id[nbr_v]]++); 
				return false;
			}
		} else {
			vertex_for_each_nbr(v, nbr_v,i, chain_lib_cnt[chain_id[nbr_v]]--); 
		}
		basic_play(player, v);
		place_stone(player, v);
		vertex_for_each_nbr(v, nbr_v,i, {
			//NCB::place_stone_side(player, nbr_v, i);
			if(color::is_player(color_at[nbr_v])) {
				if(color_at[nbr_v] != color::from_player(player)) { // same color of groups
					if(chain_lib_cnt[chain_id[nbr_v]] == 0) 
						remove_chain(nbr_v);
				} else {
					if(chain_id[nbr_v] != chain_id[v]) {
						if(chain_lib_cnt[chain_id[v]] > chain_lib_cnt[chain_id[nbr_v]]) {
							merge_chains(v, nbr_v);
						} else {
							merge_chains(nbr_v, v);
						}         
					}
				}
			}
		});
#else
		} else {
			play_not_eye(player, v); //TODO: 
			// TODO invent complete suicide testing
			assertc(board_ac, last_move_status == play_ok || last_move_status == play_suicide); 
		}
#endif
		return true;
	}
public: // slow functions
	// very slow function
	bool undo() {
		Move<T> replay[max_game_length];

		uint   game_length  = move_no;
		float  old_komi     = get_komi();

		if(game_length == 0) 
			return false;

		rep(mn, game_length-1)
			replay[mn] = move_history[mn];

		//init();
		new(this) GoBoard();
		set_komi(old_komi); // TODO maybe last_player should be preserverd as well

		rep(mn, game_length-1)
			play_legal(replay[mn].get_player(), replay[mn].get_vertex());

		return true;
	}
	// very slow function 
	void sure_undo() {
		if(undo() == false) {
			cerr << "sure_undo failed\n";
			exit(1);
		}
	}
	// a very slow function
	bool is_strict_legal(Player pl, Vertex<T> v) {            // slow function
		if(try_play(pl, v) == false) return false;
		sure_undo();
		return true;
	}
	// very slow function
	bool try_play(Player player, Vertex<T> v) {
		if(v == Vertex<T>::pass()) {
			play_legal(player, v);      
			return true;
		}

		v.check_is_on_board();

		if(color_at[v] != color::empty) 
			return false; 

		if(is_legal(player,v) == false)
			return false;

		play_legal(player, v);

		if(last_move_status != play_ok) {
			sure_undo();
			return false;
		}

		if(is_hash_repeated()) {
			sure_undo();
			return false;
		}

		return true;
	}
	// this is very very slow function
	bool is_hash_repeated() { // 根据历史棋步重下一遍来判断全局同形,果然很慢。为什么不预存hash值呢
		GoBoard tmp_board;
		rep(mn, move_no-1) {
			tmp_board.play_legal(move_history[mn].get_player(), move_history[mn].get_vertex());
			if(hash == tmp_board.hash) 
				return true;
		}
		return false;
	}
private:
	bool eye_is_not_ko(Vertex<T> v) {
		//return(v == ko_v) & (player == player::other(last_player));
		return v != ko_v;
	}
	bool eye_is_ko(Vertex<T> v) {
		//return(v == ko_v) & (player == player::other(last_player));
		return v == ko_v;
	}
	// deprecated
	bool eye_is_suicide(Vertex<T> v) {
		uint all_nbr_live = true;
		vertex_for_each_nbr(v, nbr_v,i, all_nbr_live &= (--chain_lib_cnt[chain_id[nbr_v]] != 0));
		vertex_for_each_nbr(v, nbr_v,i, chain_lib_cnt[chain_id[nbr_v]]++); //TODO: 能否更快
		return all_nbr_live;
	}
	no_inline
	bool play_eye(Player player, Vertex<T> v) {
		if(eye_is_ko(v)) return false;
		uint all_nbr_live = true;
		vertex_for_each_nbr(v, nbr_v,i, all_nbr_live &= (--chain_lib_cnt[chain_id[nbr_v]] != 0));
		if(all_nbr_live) {
			vertex_for_each_nbr(v, nbr_v,i, chain_lib_cnt[chain_id[nbr_v]]++); 
			return false;
		}
		basic_play(player, v);
		last_empty_v_cnt = empty_v_cnt;
		place_stone(player, v);
		vertex_for_each_nbr(v, nbr_v,i, {
			//NCB::place_stone_side(player, nbr_v, i);
			if((chain_lib_cnt[chain_id[nbr_v]] == 0)) 
				remove_chain(nbr_v);
		});
		assertc(board_ac, chain_lib_cnt[chain_id[v]] != 0);

		if(last_empty_v_cnt == empty_v_cnt) { // if captured exactly one stone, end this was eye
			ko_v = empty_v[empty_v_cnt - 1]; // then ko formed
		} else {
			ko_v = Vertex<T>::any();
		}
		return true;
	}

	// accept pass
	// will ignore simple-ko ban
	// will play single stone suicide
	all_inline 
	void play_legal(Player player, Vertex<T> v) {
		check();

		if(v == Vertex<T>::pass()) {
			basic_play(player, Vertex<T>::pass());
			return;
		}

		v.check_is_on_board();
		assertc(board_ac, color_at[v] == color::empty);

		if(nbr_cnt[v].player_cnt_is_max(player::other(player))) {
			play_eye_legal(player, v);
		} else {
			play_not_eye(player, v);
			// TODO invent complete suicide testing
			assertc(board_ac, last_move_status == play_ok || last_move_status == play_suicide); 
		}

	}
	// deprecated
	all_inline
	void play_not_eye(Player player, Vertex<T> v) {
		check();
		v.check_is_on_board();
		assertc(board_ac, color_at[v] == color::empty);

		basic_play(player, v);
		last_empty_v_cnt = empty_v_cnt; // TODO: remove this
		place_stone(player, v);

		vertex_for_each_nbr(v, nbr_v,i, {
			// This should be before 'if' to have good lib_cnt for empty vertices
			chain_lib_cnt[chain_id[nbr_v]]--; 

			if(color::is_player(color_at[nbr_v])) {
				if(color_at[nbr_v] != color::from_player(player)) { // same color of groups
					if(chain_lib_cnt[chain_id[nbr_v]] == 0) 
						remove_chain(nbr_v);
				} else {
					if(chain_id[nbr_v] != chain_id[v]) {
						if(chain_lib_cnt[chain_id[v]] > chain_lib_cnt[chain_id[nbr_v]]) {
							merge_chains(v, nbr_v);
						} else {
							merge_chains(nbr_v, v);
						}         
					}
				}
			}
		});
    
		if(chain_lib_cnt[chain_id[v]] == 0) {
			assertc(board_ac, last_empty_v_cnt - empty_v_cnt == 1);
			remove_chain(v);
			assertc(board_ac, last_empty_v_cnt - empty_v_cnt > 0);
			last_move_status = play_suicide;//TODO: 为何要先下这一手再undo呢?
		} else {
			last_move_status = play_ok;
		}
	}


	// deprecated
	// 在对方眼的位置下子（忽略因为劫而导致的禁入情况，因为调用前判断过了）
	no_inline
	void play_eye_legal(Player player, Vertex<T> v) {
		vertex_for_each_nbr(v, nbr_v,i, chain_lib_cnt[chain_id[nbr_v]]--);

		basic_play(player, v);
		last_empty_v_cnt = empty_v_cnt;
		place_stone(player, v);
    
		vertex_for_each_nbr(v, nbr_v,i, {
			if((chain_lib_cnt[chain_id[nbr_v]] == 0)) 
				remove_chain(nbr_v);
		});

		assertc(board_ac, chain_lib_cnt[chain_id[v]] != 0);

		if(last_empty_v_cnt == empty_v_cnt) { // if captured exactly one stone, end this was eye
			ko_v = empty_v[empty_v_cnt - 1]; // then ko formed
		} else {
			ko_v = Vertex<T>::any();
		}
	}

	void merge_chains(Vertex<T> v_base, Vertex<T> v_new) {
		Vertex<T> act_v;

		chain_lib_cnt[chain_id[v_base]] += chain_lib_cnt[chain_id[v_new]];

		act_v = v_new;
		do {
			chain_id[act_v] = chain_id[v_base];
			act_v = chain_next_v[act_v];
		} while(act_v != v_new);

		swap(chain_next_v[v_base], chain_next_v[v_new]);// 两个环断开，分别接到对方环上，扭成一个大环
	}


	no_inline 
	void remove_chain(Vertex<T> v){
		Vertex<T> act_v;
		Vertex<T> tmp_v;
		Color old_color;

		old_color = color_at[v];
		act_v = v;

		assertc(board_ac, color::is_player(old_color));

		do {
			remove_stone(color::to_player(old_color), act_v);
			chain_id[act_v] = act_v;
			chain_lib_cnt[act_v] = 0;// TODO: is it fixed?
			act_v = chain_next_v[act_v];
		} while(act_v != v);

		assertc(board_ac, act_v == v);

		do {
			vertex_for_each_nbr(act_v, nbr_v,i, {
				chain_lib_cnt[chain_id[nbr_v]]++;
				//NCB::remove_stone_side(color::to_player(old_color), nbr_v, i);
			});

			tmp_v = act_v;
			act_v = chain_next_v[act_v];
			chain_next_v[tmp_v] = tmp_v;
		} while(act_v != v);
	}
	void basic_play(Player player, Vertex<T> v) { // Warning: has to be called before place_stone, because of hash storing
		BasicBoard<T, GoBoard>::play(player, v);
		ko_v                    = Vertex<T>::any();
	}
	void place_stone(Player player, Vertex<T> v) {
		BasicBoard<T, GoBoard>::place_stone(player, v);
		NbrCounterBoard<T, GoBoard>::place_stone(player, v);
		ZobristBoard<T, GoBoard>::place_stone(player, v);

		assertc(chain_next_v_ac, chain_next_v[v] == v);
		assertc(chain_next_v_ac, chain_id[v] == v);
		assertc(chain_next_v_ac, chain_lib_cnt[v] == nbr_cnt[v].empty_cnt());
		//chain_id[v] = v;
		//chain_lib_cnt[v] = nbr_cnt[v].empty_cnt();
	}
	void remove_stone(Player pl, Vertex<T> v) {
		// 按照place_stone相反的顺序
		ZobristBoard<T, GoBoard>::remove_stone(pl, v);
		NbrCounterBoard<T, GoBoard>::remove_stone(pl, v);
		BasicBoard<T, GoBoard>::remove_stone(pl, v);
		
		//chain_id[v] = v;
	}

public:
	int approx_score() const {
		return komi + player_v_cnt[player::black] -  player_v_cnt[player::white];
	}

	Player approx_winner() { return Player((approx_score() <= 0)+1); }

	Player winner() const { 
		return Player((score() <= 0) + 1); 
	}

	int score() const {
		int eye_score = 0;

		empty_v_for_each(this, v, {
			eye_score += nbr_cnt[v].player_cnt_is_max(player::black);
			eye_score -= nbr_cnt[v].player_cnt_is_max(player::white);
		});

		return approx_score() + eye_score;
	}

	int vertex_score(Vertex<T> v) {
		switch(color_at[v]) {
			case color::black: return 1;
			case color::white: return -1;
			case color::empty: 
				return 
					(nbr_cnt[v].player_cnt_is_max(player::black)) -
					(nbr_cnt[v].player_cnt_is_max(player::white));
			case color::off_board: return 0;
			default: assert(false);
		}
	}
public:                         // consistency checks
	void check() const { // 检查整个棋盘一致性
		if(!board_ac) return;
		BasicBoard<T, GoBoard>::check();
		NbrCounterBoard<T, GoBoard>::check();
		ZobristBoard<T, GoBoard>::check();
		check_chain_at();
		check_chain_next_v();

	}
private:
	void check_chain_at() const {
		if(!chain_at_ac) return;

		vertex_for_each_all(v) { // whether same color neighbours have same root and liberties
			// TODO what about off_board and empty? 可以不用管
			if(color::is_player(color_at[v])) {

				assert(chain_lib_cnt[chain_id[v]] != 0); // player所属的串气不为0
				vertex_for_each_nbr(v, nbr_v,i, {
					if(color_at[v] == color_at[nbr_v]) // 同色邻点属于同一串
					assert(chain_id[v] == chain_id[nbr_v]);
				});
			}
		}
	}
	void check_chain_next_v() const {
		if(!chain_next_v_ac) return;
		vertex_for_each_all(v) {
			chain_next_v[v].check();
			if(!color::is_player(color_at[v])) {
				assert(chain_next_v[v] == v);
			}
			if(color_at[v] == color::empty) {
				assert(chain_lib_cnt[v] == nbr_cnt[v].empty_cnt());
			}
		}
	}
};
#endif //_GO_BOARD_H_
