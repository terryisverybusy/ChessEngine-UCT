#ifndef _BASIC_BOARD_H_
#define _BASIC_BOARD_H_

// class Board
template<uint T, typename Derive> 
class BasicBoard {

public:
	static const uint board_size = T;
	static const uint board_area = T * T;
	static const uint max_empty_v_cnt = board_area;
	static const uint max_game_length = board_area * 4;

	FastMap<Vertex<T>, Color>       color_at; // 每个点的颜色，这是棋盘的基本结构
	FastMap<Vertex<T>, uint>        empty_pos; // 如果一个点是empty，记录其在empty_v中的索引值
	Vertex<T>                       empty_v [board_area]; // 棋盘上的空点
	uint                         	empty_v_cnt;
	Player                       a_player;      // player who made the last play(other::player is forbidden to retake)
	// 记录Player占据的数量
	uint			     player_v_cnt[player::cnt]; // map Player to uint
	Vertex<T>		     player_last_v[player::cnt];// map Player to Vertex
	// 记录行棋历史
	uint                         move_no;
	Move<T>                      move_history [max_game_length];
public: // macro
// 对所有空点
  #define empty_v_for_each(board, vv, i) {                             \
      Vertex<T> vv = Vertex<T>::any();                                 \
      rep(ev_i, (board)->empty_v_cnt) {                                \
        vv = (board)->empty_v [ev_i];                                  \
        i;                                                             \
      }                                                                \
    }

// 对所有空点以及pass
  #define empty_v_for_each_and_pass(board, vv, i) {                    \
      Vertex<T> vv = Vertex<T>::pass();                                \
      i;                                                               \
      rep(ev_i, (board)->empty_v_cnt) {                                \
        vv = (board)->empty_v [ev_i];                                  \
        i;                                                             \
      }                                                                \
    }
public:                         // board interface

	BasicBoard() { // TODO: 可以考虑预先建立全局空棋盘来优化性能
		init(); 
		cout << ""; // TODO remove this stupid statement
	}
	void load(const Derive* save_board) { // 快速拷贝棋盘，要求棋盘实现为浅拷贝
		Derive* p = static_cast<Derive*>(this);
		memcpy(p, save_board, sizeof(Derive)); 
		check();
	}
	void init() {
		empty_v_cnt  = 0;
		move_no = 0;
		player_for_each(pl) {
			player_v_cnt[pl] = 0;
			player_last_v[pl] = Vertex<T>::any();
		}
		a_player = player::black;
		vertex_for_each_all(v) {
			color_at[v] = color::off_board;
			if(v.is_on_board()) {
				// 登记空点
				color_at[v] = color::empty;
				empty_pos[v] = empty_v_cnt;
				empty_v[empty_v_cnt++] = v;
			}
		}
		check();
	}

protected: // play move functions
	void play(Player player, Vertex<T> v) { // Warning: has to be called before place_stone, because of hash storing
		assertc(board_ac, move_no <= max_game_length);
		a_player = player::other(player);
		player_last_v[player]  = v;
		move_history[move_no]  = Move<T>(player, v);
		move_no += 1;
	}
protected: // auxiliary functions
	void place_stone(Player pl, Vertex<T> v) {
		player_v_cnt[pl]++;
		color_at[v] = color::from_player(pl);

		empty_v_cnt--;
		empty_pos[empty_v[empty_v_cnt]] = empty_pos[v];
		empty_v[empty_pos[v]] = empty_v[empty_v_cnt];
	}
	void remove_stone(Player pl, Vertex<T> v) {
		player_v_cnt[pl]--;
		color_at[v] = color::empty;

		empty_pos[v] = empty_v_cnt;
		empty_v[empty_v_cnt++] = v;

		assertc(board_ac, empty_v_cnt < Vertex<T>::cnt);
	}
public: // utils

	// TODO/FIXME last_player should be preserverd in undo function
	//Player act_player() const { return player::other(last_player); } 
	Player act_player() const { return a_player; } 

	bool both_player_pass() {
		return 
			(player_last_v [player::black] == Vertex<T>::pass()) & 
			(player_last_v [player::white] == Vertex<T>::pass());
	}

	void print_cerr(Vertex<T> v = Vertex<T>::pass()) const {
		cerr << to_string(v);
	}

public:                         // consistency checks

	void check() const { // 检查整个棋盘一致性
		if(!board_ac) return;
		check_empty_v();
		check_color_at();
	}
private:
	void check_empty_v() const { //检查color_at,empty_pos,empty_v的一致性
		if(!board_empty_v_ac) return;

		FastMap<Vertex<T>, bool> noticed;
		uint exp_player_v_cnt[player::cnt];

		vertex_for_each_all(v) noticed[v] = false;
		assert(empty_v_cnt <= board_area);

		empty_v_for_each(this, v, {
				assert(noticed [v] == false);
				noticed [v] = true;
				});

		player_for_each(pl) exp_player_v_cnt [pl] = 0;
		vertex_for_each_all(v) {
			assert((color_at[v] == color::empty) == noticed[v]);
			if(color_at[v] == color::empty) {
				assert(empty_pos[v] < empty_v_cnt);
				assert(empty_v [empty_pos[v]] == v);
			}
			if(color::is_player(color_at [v])) exp_player_v_cnt [color::to_player(color_at[v])]++;
		}

		player_for_each(pl) 
			assert(exp_player_v_cnt [pl] == player_v_cnt [pl]);
	}

	void check_color_at() const { // 棋盘上的点不能是off_board
		if(!board_color_at_ac) return;

		vertex_for_each_all(v) {
			color::check(color_at [v]);
			assert((color_at[v] != color::off_board) == (v.is_on_board()));
		}
	}

};
#endif //_BASIC_BOARD_H_
