#include <stddef.h>
#include <cstdint>

#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32

#ifdef __UNIX__
#include <stdlib.h>
#endif

#ifdef __APPLE__
#include <stdlib.h>
#endif // __APPLE__


#pragma once

#ifndef __COBALT_FORCE_INLINE__
#if defined(_MSC_VER)
#define __COBALT_FORCE_INLINE__ __forceinline
#elif defined(__GNUC__) 
#define __COBALT_FORCE_INLINE__ __attribute__((always_inline)) inline
#else
#define __COBALT_FORCE_INLEIN__ inline
#endif
#endif // !__COBALT_FORCE_INLINE__


#ifndef __SLEEP
#if defined(_WIN32)
#define __SLEEP(n) Sleep(n)
#else
#define __SLEEP(n) sleep(n);
#endif

#endif // !__SLEEP


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



