
enum playout_status_t { pass_pass, mercy, too_long };


// ----------------------------------------------------------------------
template <uint T, template<uint T> class Policy, template<uint T> class Board> 
class Playout {
public:
	static const uint max_playout_length = T * T * 2;
	Policy<T>*  policy;
	Board<T>*   board;

	Playout (Policy<T>* policy_, Board<T>*  board_) : policy (policy_), board (board_) {}

	all_inline
	void play_move () {
		policy->prepare_vertex ();

		Player   act_player = board->act_player ();
		while (true) {
			//Player   act_player = board->act_player ();
			Vertex<T>   v          = policy->next_vertex ();
#if 0
			// 简单的判断是否合法，但是有误判
			if (board->is_legal (act_player, v) == false) {
				policy->bad_vertex (v);
				continue;
			} else {
				board->play_legal (act_player, v);
				policy->played_vertex (v);
				act_player = board->act_player ();
				break;
			}
#endif
			if (board->play(act_player, v) == false) {
				policy->bad_vertex(v);
				continue;
			} else {
				policy->played_vertex(v);
				act_player = board->act_player();
				break;
			}
		}
	}

	all_inline
	playout_status_t run () {

		policy->begin_playout (board);
		while (true) {
			play_move ();

			if (board->both_player_pass ()) {
				policy->end_playout (pass_pass);
				return pass_pass;
			}

			if (board->move_no >= max_playout_length) {
				policy->end_playout (too_long);
				return too_long;
			}

			if (Board<T>::use_mercy_rule && uint (abs (board->approx_score ())) > mercy_threshold) {
				policy->end_playout (mercy);
				return mercy;
			}
		}
	}

};

// ----------------------------------------------------------------------
template<uint T> class GoPolicy {
protected:

	PmRandom pm; 

	uint to_check_cnt;
	uint act_evi;

	GoBoard<T>* board;
	Player act_player;

public:

	GoPolicy () : board (NULL),pm(time(NULL)) { }

	void begin_playout (GoBoard<T>* board_) { 
		board = board_;
	}

	void prepare_vertex () {
		act_player     = board->act_player ();
		to_check_cnt   = board->empty_v_cnt;
		act_evi        = pm.rand_int (board->empty_v_cnt); 
	}

	/*
	 * TODO:
	 * 这里随机下棋有个缺陷，就是坏点的下一个点有更高的机会被选择
	 */
	Vertex<T> next_vertex () {
		Vertex<T> v;
		while (true) {
			if (to_check_cnt == 0) return Vertex<T>::pass ();
			to_check_cnt--;
			v = board->empty_v [act_evi];
			act_evi++;
			if (act_evi == board->empty_v_cnt) act_evi = 0;
			if (!board->is_eyelike (act_player, v)) return v;
		}
	}

	void bad_vertex (Vertex<T>) {
	}

	void played_vertex (Vertex<T>) { 
	}

	void end_playout (playout_status_t) { 
	}

};

template<uint T> class RenjuPolicy {
protected:

	PmRandom pm; 

	uint to_check_cnt;
	uint act_evi;

	RenjuBoard<T>* board;
	Player act_player;

public:

	RenjuPolicy () : board (NULL),pm(clock()/*time(NULL)*/) { }//wy2013 need revision

	void begin_playout (RenjuBoard<T>* board_) { 
		board = board_;
	}

	void prepare_vertex () {
		act_player     = board->act_player ();
		to_check_cnt   = board->good_v_cnt;
		act_evi        = pm.rand_int (to_check_cnt); 
	}

	Vertex<T> next_vertex () {
		Vertex<T> v;
		while (true) {
			if (to_check_cnt == 0) return Vertex<T>::pass ();
			to_check_cnt--;
			v = board->good_v [act_evi];
			act_evi++;
			if (act_evi == board->good_v_cnt) act_evi = 0;
			return v;
		}
	}

	void bad_vertex (Vertex<T>) {
	}

	void played_vertex (Vertex<T>) { 
	}

	void end_playout (playout_status_t) { 
	}

};
//template<uint T> PmRandom SimplePolicy<T>::pm(123);
