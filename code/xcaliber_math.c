#include "xcaliber_math.h"

int32_t
xcr_cross_product(v2i v1, v2i v2)
{
	return (v1.x * v2.y) - (v2.y * v1.x);
}
