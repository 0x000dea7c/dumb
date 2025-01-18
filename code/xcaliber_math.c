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

	/* for better accuracy, I can do one more Newton-Raphson iteration
	   if y was perfect, then:
	   - y          = 1 / √n
	   - y²         = 1 / n
	   - n * y²     = 1
	   - n * y² - 1 = 0

	   the statement below is trying to get close to this ideal!!

	   n_half * y * y -> n/2 * y²

	   the subtraction gives a correction factor, and multiplying this factor by
	   y somehow improves the approximation */
	rsqrt = rsqrt * (three_halfs - (n_half * rsqrt * rsqrt));

	return rsqrt;
}

inline float
xcr_sqrt(float n)
{
	return n * fast_sse_rsqrt(n);
}
