#ifndef _UTILS_H_
#define _UTILS_H_
#ifdef unix
#include <sys/resource.h>
#endif
#include <iostream>
#include <ctime>
#include   <assert.h> 
//#include <string>
//#include <sstream>
//#include <iomanip>
using namespace std;


// standard macros

#define qq(...) fprintf(stdout, __VA_ARGS__); fflush (stdout);

//TODO rename to ignore
#define unused(p) (void)(p)
#define nop unused(0)

#define rep(i,n)     for (int i = 0;   i < (int)(n); i++)
#define seq(i,a,b)   for (let (i, a); i <= (b); i++)
#define dseq(i,b,a)  for (let (i, b); i >= (a); i--)

#define let(a,b) typeof(b) a=(b)
#define for_each(it, c) for(let(it,(c).begin()); it!=(c).end(); ++it)

#define assertc(aspect, expr) assert((aspect) ? (expr) : true)

const float large_float = 1000000000000.0;


//#define cc_measure(cc_clock, instr) { cc_clock.start (); instr; cc_clock.stop (); }


// class PmRandom


class PmRandom {             // Park - Miller "minimal standard" 

	static const int cnt = (uint(1)<<31) - 1;  

	uint seed;
	//tr1::minstd_rand0 mt; // this is eqivalent when #include <tr1/random>

public:

	PmRandom (uint seed_ = 12345) //: mt (seed_) 
	{ seed = seed_; }

	void set_seed (uint _seed) { seed = _seed; }
	uint get_seed () { return seed; }

	uint rand_int () {       // a number between  0 ... cnt - 1
		uint hi, lo;
		lo = 16807 * (seed & 0xffff);
		hi = 16807 * (seed >> 16);
		lo += (hi & 0x7fff) << 16;
		lo += hi >> 15;
		seed = (lo & 0x7FFFFFFF) + (lo >> 31);
		return seed;
		//return mt (); // equivalen
	}

	// n must be between 1 .. (1<<16) + 1
	inline uint rand_int (uint n) { // 0 .. n-1
		assertc (pm_ac, n > 0);
		assertc (pm_ac, n <= (1<<16)+1);
		return ((rand_int () & 0xffff) * n) >> 16;
	}

	/*void test () {
		uint start = rand_int ();

		uint n = 1;
		uint max = 0;
		uint sum = start;

		while (true) {
			uint r = rand_int ();
			if (r == start) break;
			n++;
			sum += r;
			if (max < r) max = r;
		}
		printf ("n = %d\n", n);
		printf ("max = %d\n", max);
		printf ("sum = %d\n", sum);
	}*/

	/*void test2 (uint k, uint n) {
		uint bucket [k];

		rep (ii, k)  bucket [ii] = 0;
		rep (ii, n) {
			uint r = rand_int (k);
			assert (r < k);
			bucket [r] ++;
		}
		rep (ii, k)  printf ("%d\n", bucket [ii]);
	}*/

};


// TODO use Stack in Board
template <typename elt_t, uint _max_size> 
class Stack {
public:
	elt_t tab [_max_size];
	uint size;

	Stack () {
		size = 0;
	}

	void check () const { 
		assertc (stack_ac, size <= _max_size);
	}

	bool is_empty () const { return size == 0; }

	elt_t& top () { assertc (stack_ac, size > 0); return tab [size-1]; }

	void   push_back (elt_t& elt) { tab [size++] = elt; check (); }


	elt_t pop_random (PmRandom& pm) { 
		assertc (stack_ac, size > 0); 
		uint idx = pm.rand_int (size);
		elt_t elt = tab [idx];
		size--;
		tab [idx] = tab [size];
		return elt; 
	}

	void   pop () { size--; check (); }

};


// very simple and useful FastMap

template <typename idx_t, typename elt_t> 
class FastMap {
	elt_t tab [idx_t::cnt];
public:
	elt_t& operator[] (idx_t pl)             { return tab [pl]; }
	const elt_t& operator[] (idx_t pl) const { return tab [pl]; }
};


// some useful functions


inline float get_seconds () {
#ifdef unix
	rusage usage [1];
	getrusage (RUSAGE_SELF, usage);
	return float(usage->ru_utime.tv_sec) + float(usage->ru_utime.tv_usec) / 1000000.0;
#else
	return float(clock()) / CLOCKS_PER_SEC;

#endif
}

inline void fatal_error (const char* s) {
	cerr << "Fatal error: " << s << endl;
	assert (false);
	exit (1);
}

// g++ extensions


#if 0

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#else

#define likely(x)   (x)
#define unlikely(x) (x)

#endif


/*
 * VC++2005中使用__forceinline来强迫内联，使用__declspec(noinline)来强迫不内联
 */

#if 0

#define no_inline   __attribute__((noinline))
#define flatten     __attribute__((flatten))
#define all_inline  __attribute__((always_inline))

#else

#define no_inline
#define flatten  
#define all_inline

#endif
#endif // _UTILS_H_
