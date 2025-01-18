#ifndef XCALIBER_MATH_H
#define XCALIBER_MATH_H

#include <stdint.h>

#define XC_ABS(a) ((a) >= 0 ? (a) : (-(a)))
#define XC_MIN(a, b) ((a) < (b) ? (a) : (b))
#define XC_MAX(a, b) ((a) > (b) ? (a) : (b))

float xc_sqrt(float n);
float xc_rsqrt(float n);

#endif
