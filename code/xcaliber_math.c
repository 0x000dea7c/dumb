#include "xcaliber_math.h"

inline float
fast_sse_rsqrt(float n)
{
	float const three_halfs = 1.5f;
	float const n_half = (float)n * 0.5f;
	float rsqrt;

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

	   n = 9
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

inline float
fast_sse_sqrt(float n)
{
	float y;
	__asm__ volatile("sqrtss %1, %%xmm0\n\t"
			 "movss %%xmm0, %0\n\t"
			 : "=m"(y)
			 : "m"(n)
			 : "xmm0");
	return y;
}

inline float
xc_sqrt(float n)
{
	return fast_sse_sqrt(n);
}

inline float
xc_rsqrt(float n)
{
	return fast_sse_rsqrt(n);
}
