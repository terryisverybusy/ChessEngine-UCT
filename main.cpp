#include <ctime>//#include <time.h>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
//#include "unistd.h"
using namespace std;
#include "go_conf.h"
#include "utils.h"
#include "goboard.h"
#include "renjuboard.h"
#include "playout.h"
#include "go_io.h"
#include "uct.h"



#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <ctype.h>
#include <stdarg.h>





#include "pisqpipe.h"
//新增的
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "pisqpipe.h"
const char *infotext="author=\"Petr Lastovicka\", version=\"7.5\", country=\"Czech Republic\",  www=\"http://petr.lastovicka.sweb.cz\"";





int width,height; /* the board size */
int info_timeout_turn=30000; /* time for one turn in milliseconds */
int info_timeout_match=1000000000; /* total time for a game */
int info_time_left=1000000000; /* left time for a game */
int info_max_memory=0; /* maximum memory in bytes, zero if unlimited */
int info_game_type=1; /* 0:human opponent, 1:AI opponent, 2:tournament, 3:network tournament */
int info_exact5=0; /* 0:five or more stones win, 1:exactly five stones win */
int info_continuous=0; /* 0:single game, 1:continuous */
int terminateNew; /* return from brain_turn when terminate>0 */
unsigned start_time; /* tick count at the beginning of turn */
char dataFolder[256]; /* folder for persistent files */

static char cmd[256];
static HANDLE event1,event2;


void brain_init() 
{
  if(width<5 || width>127 || height<5 || height>127){
    pipeOut("ERROR size of the board");
    return;
  }
    //init();
  pipeOut("OK");
}

void brain_restart()
{
  brain_init();
}



//choose your move and call do_mymove(x,y);
void brain_turn() 
{
  //computer();
  //do_mymove(resultMove->x, resultMove->y);
}


/** write move to the pipe and update internal data structures */
void do_mymove(int x,int y) 
{
 // brain_my(x,y);
  pipeOut("%d,%d",x,y);
}










/** write a line to STDOUT */
int pipeOut(char *fmt,...)
{
  int i;
  va_list va;
  va_start(va,fmt);
  i=vprintf(fmt,va);
  putchar('\n');
  fflush(stdout);
  va_end(va);
  return i;
}



/** read a line from STDIN */
static void get_line() 
{
  int c,bytes;
  
  bytes=0;
  do{
    c=getchar();
    if(c==EOF) exit(0);
    if(bytes<sizeof(cmd)) cmd[bytes++]=(char)c;
  }while(c!='\n');
  cmd[bytes-1]=0;
  if(cmd[bytes-2]=='\r') cmd[bytes-2]=0;
}

/** parse coordinates x,y */
static int parse_coord(const char *param,int *x,int *y) 
{
  if(sscanf_s(param,"%d,%d",x,y)!=2 ||
    *x<0 || *y<0 || *x>=width || *y>=height){
    return 0;
  }
  return 1;
}










/** parse coordinates x,y and player number z */
static void parse_3int_chk(const char *param,int *x,int *y,int *z) 
{
  if(sscanf_s(param,"%d,%d,%d",x,y,z)!=3 || *x<0 || *y<0 || 
    *x>=width || *y>=height) *z=0;
}








/** return pointer to word after cmd if input starts with cmd, otherwise return NULL */
static const char *get_cmd_param(const char *cmd,const char *input) 
{
  int n1,n2;
  n1=strlen(cmd);
  n2=strlen(input);
  if(n1>n2 || _strnicmp(cmd,input,n1)) return NULL; /* it is not cmd */
  input+=strlen(cmd);
  while(isspace(input[0])) input++;
  return input;
}








/** send suggest */
void suggest(int x,int y) 
{
  pipeOut("SUGGEST %d,%d",x,y);
}








/** main function for the working thread */
static DWORD WINAPI threadLoop(LPVOID)
{
  for(;;){
    WaitForSingleObject(event1,INFINITE);
    //brain_turn();
    SetEvent(event2);
  }
}

/** start thinking */
static void turn()
{
  terminateNew=0;
  ResetEvent(event2);
  SetEvent(event1);
}






/** stop thinking */
static void stop()
{
  terminateNew=1;
  WaitForSingleObject(event2,INFINITE);
}

static void start()
{
  start_time=GetTickCount();
  stop();
  if(!width){
    width=height=20;
    brain_init();
  }
}


/** do command cmd */
 void do_command() 
{
  const char *param;
  const char *info;
  char *t;
  int x,y,who,e;
  
  if((param=get_cmd_param("info",cmd))!=0) {
    if((info=get_cmd_param("max_memory",param))!=0) info_max_memory=atoi(info);
    if((info=get_cmd_param("timeout_match",param))!=0) info_timeout_match=atoi(info);
    if((info=get_cmd_param("timeout_turn",param))!=0) info_timeout_turn=atoi(info);
    if((info=get_cmd_param("time_left",param))!=0) info_time_left=atoi(info);
    if((info=get_cmd_param("game_type",param))!=0) info_game_type=atoi(info);
    if((info=get_cmd_param("rule",param))!=0){ e=atoi(info); info_exact5=e&1; info_continuous=(e>>1)&1; }
    if((info=get_cmd_param("folder",param))!=0) strncpy_s(dataFolder,info,sizeof(dataFolder)-1);
#ifdef DEBUG_EVAL
   // if((info=get_cmd_param("evaluate",param))!=0){ if(parse_coord(info,&x,&y)) brain_eval(x,y); }
#endif
    /* unknown info is ignored */
  } 
  else if((param=get_cmd_param("start",cmd))!=0) {
    if(sscanf_s(param,"%d %d",&width,&height)==1) height=width;
    start();
    brain_init();
  } 
  else if((param=get_cmd_param("rectstart",cmd))!=0) {
    if(sscanf_s(param,"%d ,%d",&width,&height)!=2){
      pipeOut("ERROR bad RECTSTART parameters");
    }else{
      start();
      brain_init();
    }
  } 
  else if((param=get_cmd_param("restart",cmd))!=0) {
    start();
    brain_restart();
  } 
  else if((param=get_cmd_param("turn",cmd))!=0) {
    start();
    if(!parse_coord(param,&x,&y)){
      pipeOut("ERROR bad coordinates");
    }else{
     // brain_opponents(x,y);
      turn();
    }
  } 
  else if((param=get_cmd_param("play",cmd))!=0) {
    start();
    if(!parse_coord(param,&x,&y)){
      pipeOut("ERROR bad coordinates");
    }else{
   //   do_mymove(x,y);
    }
  } 
  else if((param=get_cmd_param("begin",cmd))!=0) {
    start();
    turn();
  } 
  else if((param=get_cmd_param("about",cmd))!=0) {
#ifdef ABOUT_FUNC
    //brain_about();
#else
    pipeOut("%s",infotext);
#endif
  } 
  else if((param=get_cmd_param("end",cmd))!=0) {
    stop();
  //  brain_end();
    exit(0);
  } 
  else if((param=get_cmd_param("board",cmd))!=0) {
    start();
    for(;;){ /* fill the whole board */
      get_line();
      parse_3int_chk(cmd,&x,&y,&who);
        if(who==1)       int guan = 1;// brain_my(x,y);
        else if(who==2) int guan=1;//brain_opponents(x,y) ;
        else if(who==3) int guan=2;//brain_block(x,y);
        else{
        if(_stricmp(cmd,"done")) pipeOut("ERROR x,y,who or DONE expected after BOARD");
        break;
      }
    }
    turn();
  } 
  else if((param=get_cmd_param("takeback",cmd))!=0) {
    start();
    t="ERROR bad coordinates";
    if(parse_coord(param,&x,&y)){
   //   e= brain_takeback(x,y);
      if(e==0) t="OK";
      else if(e==1) t="UNKNOWN";
    }
    pipeOut(t);
  }
  else{ 
    pipeOut("UNKNOWN command");
  }
}

























namespace simple_playout_benchmark {


	uint win_cnt[player::cnt];
	uint                playout_ok_cnt;
	int                 playout_ok_score;
	//PerformanceTimer    perf_timer;

	template <bool score_per_vertex,uint T, template<uint T> class Policy, template<uint T> class Board > 
	void run(Board<T> const * start_board, 
		Policy<T> * policy,
			uint playout_cnt, 
			ostream& out) 
	{
		Board<T>  mc_board [1];
		FastMap<Vertex<T>, int>   vertex_score;
		playout_status_t status;
		Player winner;
		int score;

		playout_ok_cnt   = 0;
		playout_ok_score = 0;

		player_for_each(pl) 
			win_cnt [pl] = 0;

		vertex_for_each_all(v) 
			vertex_score [v] = 0;

		//perf_timer.reset();

		Playout<T, Policy, Board> playout(policy, mc_board);

		double starttimepoint,stoptimepoint;
		starttimepoint = clock();//perf_timer.start();
		float seconds_begin = get_seconds();
		uint move_no = 0;
		uint pl_cnt = 0;

		rep(ii, playout_cnt) {
			mc_board->load(start_board);
			status = playout.run();
			//cerr<<to_string(*mc_board)<<endl;
			switch(status) {
				case pass_pass:
					playout_ok_cnt += 1;

					score = mc_board -> score();
					playout_ok_score += score;

					winner = Player((score <= 0) + 1);  // mc_board->winner()
					win_cnt [winner] ++;
					move_no += mc_board->move_no;
					pl_cnt += 1;

					if(score_per_vertex) {
						vertex_for_each_faster(v)
							vertex_score [v] += mc_board->vertex_score(v);
					}
					break;
				case mercy:
					//out << "Mercy rule should be off for benchmarking" << endl;
					//return;
					win_cnt [mc_board->approx_winner()] ++;
					move_no += mc_board->move_no;
					pl_cnt += 1;
					break;
				case too_long:
					break;
			}
		}

		float seconds_end = get_seconds();
		stoptimepoint = clock(); //perf_timer.stop();

		out << "Initial board:" << endl;
		out << "komi " << start_board->get_komi() << " for white" << endl;
		out << "av step " << (move_no/pl_cnt) << endl;

		//out << start_board->to_string();
		//out << to_string(*start_board);
		out << endl;

		if(score_per_vertex) {
			FastMap<Vertex<T>, float> black_own;
			vertex_for_each_all(v) 
				black_own [v] = float(vertex_score [v]) / float(playout_ok_cnt);

			out << "P(black owns vertex) rescaled to [-1, 1] (p*2 - 1): " << endl 
			<< to_string_2d(black_own) << endl;
		}

		out << "Black wins    = " << win_cnt [player::black] << endl
			<< "White wins    = " << win_cnt [player::white] << endl
			<< "P(black win)  = " 
			<< float(win_cnt [player::black]) / 
			float(win_cnt [player::black] + win_cnt [player::white]) 
			<< endl;

		float avg_score = float(playout_ok_score) / float(playout_ok_cnt);

		out << "E(score)      = " 
			<< avg_score - 0.5 
			<< " (without komi = " << avg_score - mc_board->komi << ")" << endl << endl;

		float seconds_total = seconds_end - seconds_begin;
		float cc_per_playout = (stoptimepoint-starttimepoint) / double(playout_cnt); // float cc_per_playout = perf_timer.ticks() / double(playout_cnt);

		out << "Performance: " << endl
			<< "  " << playout_cnt << " playouts" << endl
			<< "  " << seconds_total << " seconds" << endl
			<< "  " << float(playout_cnt) / seconds_total / 1000.0 << " kpps" << endl
			<< "  " << cc_per_playout << " ccpp(clock cycles per playout)" << endl
			<< "  " << 1000000.0 / cc_per_playout  << " kpps/GHz(clock independent)" << endl;
	}
}






template<uint T, class Board>
void pre(Board & board, int a[][2], int size) {
	for(int i = 0; i < size; i++)
	board.play(board.act_player(), Vertex<T>::of_coords(a[i][0], a[i][1]));
}




template <uint T, template<uint T> class Policy, template<uint T> class Board> 
void match_human(Board<T>* board, Policy<T>* policy, bool aifirst = true) {

	UCT<T, Policy, Board>* engine = new UCT<T, Policy, Board>(board, policy);

	

	if(aifirst) {
		Player pl = board->act_player();
		Vertex<T> v = engine->gen_move(pl);
		
		board->play(pl, v);
		//新增
		 pipeOut("%d,%d",v.row(),v.col());

		cerr<<to_string(*board);
		cout<<"="<<v<<endl;
	}
	while(true) {

		Player pl = board->act_player();
		Vertex<T> v = engine->gen_move(pl);
		
		board->play(pl, v);
		 pipeOut("%d,%d",v.row(),v.col());
		cerr<<to_string(*board);
		cout<<"="<<v<<endl;
		//string line;
		//if (!getline(cin, line)) break;
	//	Vertex<T> v = of_gtp_string<T>(line);
		//Player pl = board->act_player();
	//	if(v == Vertex<T>::any() || board->color_at[v] != color::empty || !board->play(pl,v)) {
	//		cout<<"?"<<endl;
	//		continue;
	//	}
//		engine->play(pl,v);
	//	pl = board->act_player();
		//v = engine->gen_move(pl);

		
//		if(v != Vertex<T>::resign()) {
//			board->play(pl, v);
			//新增
			// pipeOut("%d,%d",v.row(),v.col());



	//	}
		cerr<<to_string(*board);
	//	cout<<"="<<v<<endl;
	}
}


bool parse_arg(int argc, char** argv) {
	bool ret = true;
	while(--argc > 0) {
		switch(argv[argc][0]) {
			case '1': ret = false;
				  break;
			case '-': argv[argc]+=2;
				  {
					  int time = atoi(argv[argc]);
					  if(time != 0) {
						  time_per_move = time;
					  }
				  }
				  break;
			default:
				  break;
		}
	}
	return ret;
}




float time_per_move		  = 9.0;
// main
int main(int argc, char** argv) { 
	
	/*
	
	DWORD tid;
  event1=CreateEvent(0,FALSE,FALSE,0);
  CreateThread(0,0,threadLoop,0,0,&tid);
  event2=CreateEvent(0,TRUE,TRUE,0);
	//新增
	const char *param;
		get_line();
		if((param=get_cmd_param("start",cmd))!=0) {
       if(sscanf_s(param,"%d %d",&width,&height)==1) height=width;
         start();
          brain_init();
         } 


		int x,y ;
		get_line();
       if((param=get_cmd_param("turn",cmd))!=0) {
          start();
    if(!parse_coord(param,&x,&y)){
      pipeOut("ERROR bad coordinates");
    }else{
     // brain_opponents(x,y);
      turn();
    }
  } 


  */







	setvbuf(stdout, (char *)NULL, _IONBF, 0);
	setvbuf(stderr, (char *)NULL, _IONBF, 0);

	RenjuBoard<15> board;
	RenjuPolicy<15> policy;

	 ostringstream response;
     uint playout_cnt = 100000;
	
	
	ofstream fout("log.txt"); 
	cerr.rdbuf(fout.rdbuf());
	match_human(&board, &policy, parse_arg(argc, argv));

	
	




	return 0;
}
