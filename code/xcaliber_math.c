#include "xcaliber_math.h"
#include <assert.h>

/* NOTE: the attribute is needed because compiling with O0 triggers a linker error... */
__attribute__((always_inline)) inline f32_t
fast_sse_rsqrt(f32_t n)
{
	f32_t const three_halfs = 1.5f;
	f32_t const n_half = (f32_t)n * 0.5f;
	f32_t rsqrt;

	/* computes 1 / √n (AN APPROXIMATION) and stores it in xmm0 and y */
	/* AT&T syntax src, dst */
	__asm__ volatile("rsqrtss %1, %%xmm0\n\t"
			 "movss %%xmm0, %0\n\t"
			 : "=m"(rsqrt)
			 : "m"(n)
			 : "xmm0");

	/* for better accuracy, I can do one more Newton-Raphson iteration!!

	   if the approximation was perfect, then (n_half * rsqrt * rsqrt) would be 0.5.

	   Example:

	   - (n / 2) * (1 / √n) * (1 / √n)
	   - (n / 2) * (1 / (√n)²)
	   - (n / 2) * (1 / n)
	   - 1 / 2

	   so, three_halfs - (n_half * rsqrt * rsqrt) would equal to 1:

	   - (3/2) - (1/2)
	   - (2/2)
	   - 1

	   and rsqrt = rsqrt * 1 wouldn't change. done.

	   if the approximation was too high, the correction factor would be smaller than 1,
	   making the approximation smaller.

	   if the approximation was too low, it would push the approximation up.
	*/
	rsqrt = rsqrt * (three_halfs - (n_half * rsqrt * rsqrt));

	return rsqrt;
}

__attribute__((always_inline)) inline f32_t
fast_sse_sqrt(f32_t n)
{
	f32_t y;
	__asm__ volatile("sqrtss %1, %%xmm0\n\t"
			 "movss %%xmm0, %0\n\t"
			 : "=m"(y)
			 : "m"(n)
			 : "xmm0");
	return y;
}

inline f32_t
xc_sqrt(f32_t n)
{
	return fast_sse_sqrt(n);
}

inline f32_t
xc_rsqrt(f32_t n)
{
	return fast_sse_rsqrt(n);
}

inline xc_vec2i
xc_vec2i_sub(xc_vec2i a, xc_vec2i b)
{
	return (xc_vec2i){ .x = a.x - b.x, .y = a.y - b.y };
}

inline xc_vec2i
xc_vec2_add(xc_vec2i a, xc_vec2i b)
{
	return (xc_vec2i){ .x = a.x + b.x, .y = a.y + b.y };
}

inline int32_t
xc_vec2i_cross_product(xc_vec2i a, xc_vec2i b)
{
	return a.x * b.y - a.y * b.x;
}

inline int32_t *
xc_interpolate_array(stack_arena *a, int32_t y0, int32_t x0, int32_t y1, int32_t x1, int32_t points)
{
	if (y0 > y1) {
		XC_SWAP(int32_t, y0, y1);
		XC_SWAP(int32_t, x0, x1);
	}

	int32_t const dx = x1 - x0;
	int32_t const dy = y1 - y0;
	int32_t *x_values = stack_arena_alloc(a, (uint32_t)points * sizeof(int32_t));

	if (dy == 0) {
		for (int32_t i = 0; i < points; ++i) {
			x_values[i] = (x0 + dx * i) / points;
		}
	} else {
		for (int32_t i = 0; i < points; ++i) {
			x_values[i] = x0 + ((dx * i) / dy);
		}
	}

	return x_values;
}
