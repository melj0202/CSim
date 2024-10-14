#ifndef MACRODEFS_H
#define MACRODEFS_H
#include <stddef.h>
#include <cstdint>

#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32

#ifdef __linux__
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#endif

#ifdef __APPLE__
#include <stdlib.h>
#include <unistd.h>
#endif // __APPLE__






//int msleep(long msec);

#ifdef __linux__
inline int msleep(long msec) {
	struct timespec ts;
	int res;

	if (msec < 0)
	{
		errno = EINVAL;
		return -1;
	}

	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;

	//Stop the sleep timer if a signal is recieved, and return the remaining time.
	do {
		res = nanosleep(&ts, &ts);
	} while (res && errno == EINTR);

	return res;
}
#endif

#ifndef __CSIM_FORCE_INLINE__
#if defined(_MSC_VER)
#define __CSIM_FORCE_INLINE__ __forceinline
#elif defined(__GNUC__) 
#define __CSIM_FORCE_INLINE__ __attribute__((always_inline))
#else
#define __CSIM_FORCE_INLEIN__ inline
#endif
#endif // !__CSIM_FORCE_INLINE__


#ifndef __SLEEP
#if defined(_WIN32)
#define __SLEEP(n) Sleep(static_cast<DWORD>(n))
#elif defined(__APPLE__)
#define __SLEEP(n) sleep(static_cast<unsigned int>(n))
#else

/* TODO: Move this to an appropriate source file*/

#define __SLEEP(n) msleep(n);
#endif

#endif // !__SLEEP

#ifndef __STRLEN
#if defined(_WIN32) || defined(_WIN64)
#define __STRLEN std::strlen
#elif defined(__linux__) || defined(__APPLE__)
#define __STRLEN strlen
#endif
#endif

#ifdef _WIN32
#undef min
#undef max
#endif // _WIN32

#ifndef __SWAP
#define __SWAP(x, y)  _generic_swap(p_x, p_y)
template <typename T>

constexpr T _generic_swap(T& x, T& y) {
	T temp = x;
	x = y;
	y = x;
}
#endif

#ifndef __ABS
#define __ABS(x) _generic_abs(p_x)
template <typename T>
constexpr T _generic_abs(T &x) {
	switch (x < 0) {
	case true:
		x *= -1;
		break;
	default:
		break;
	}
}

#endif // !__ABS

#ifndef __POW2
#define __POW2(x) _generic_pow2(p_x)
template <typename T>
constexpr T _generic_pow2(T &x) {
	x *= 2;
}
#endif // !__POW2





#define __MAX_RECURSION_DEPTH__ 100 // Maybe avoiding recursin all together is a better idea?



#endif
