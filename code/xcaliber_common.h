#ifndef XCALIBER_COMMON_H
#define XCALIBER_COMMON_H

#define ARRAY_COUNT(arr) (sizeof((arr)) / (sizeof((arr)[0])))
#define KILOBYTES(number) ((number) * 1024ull)
#define MEGABYTES(number) (KILOBYTES(number) * 1024ull)
#define MINIMUM(a, b) ((a) < (b) ? (a) : (b))
#define MAXIMUM(a, b) ((a) > (b) ? (a) : (b))
#define IS_POWER_OF_TWO(a) (((a) != 0) && ((a) & ((a) - 1)) == 0)

#endif
