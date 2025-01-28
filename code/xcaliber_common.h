#ifndef XCALIBER_COMMON_H
#define XCALIBER_COMMON_H

#define XC_ARRAY_COUNT(arr) (sizeof((arr)) / (sizeof((arr)[0])))
#define XC_KILOBYTES(number) ((number) * 1024ull)
#define XC_MEGABYTES(number) (XC_KILOBYTES(number) * 1024ull)
#define XC_IS_POWER_OF_TWO(a) (((a) != 0) && ((a) & ((a) - 1)) == 0)
#define XC_SWAP(type, a, b)        \
	do {                       \
		type SWAP_tmp = b; \
		b = a;             \
		a = SWAP_tmp;      \
	} while (0)

#define XC_POPCNT __builtin_popcount

/* stands for count trailing zeroes */
#define XC_CTZ(v) __builtin_ctz(v)

#define f32_t float
#define f64_t double

#endif
