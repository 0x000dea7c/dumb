#pragma once

#define ARRAY_COUNT(arr) (sizeof((arr)) / (sizeof((arr)[0])))
#define KILOBYTES(number) ((number)*1024ull)
#define MEGABYTES(number) (Kilobytes(number) * 1024ull)
#define MINIMUM(a, b) ((a) < (b) ? (a) : (b))
#define MAXIMUM(a, b) ((a) > (b) ? (a) : (b))
