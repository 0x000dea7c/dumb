#include "xcaliber_math.h"

inline float
fast_sse_sqrt(float n)
{
	float const three_halfs = 1.5f;
	float x2 = (float)n * 0.5f;
	float y;

	__asm__ volatile("rsqrtss %1, %%xmm0\n\t"
			 "movss %%xmm0, %0\n\t"
			 : "=m"(y)
			 : "m"(n)
			 : "xmm0");

	/* for better accuracy, I can do one more Newton iteration */
	y = y * (three_halfs - (x2 * y * y));

	return y;
}

inline float
xcr_sqrt(float n)
{
	return n * fast_sse_sqrt(n);
}
