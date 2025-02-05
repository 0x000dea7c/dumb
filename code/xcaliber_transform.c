#include "xcaliber_transform.h"
#include "xcaliber_math.h"

/* void */
/* xct_transform_triangle (xc_transform const* transform, xc_triangle* triangle) */
/* { */
/*   for (int i = 0; i < 3; ++i) */
/*     { */
/*       xc_vec2i const v  = triangle->vertices[i]; */
/*       f32_t    const *m = transform->matrix.values; */
/*       f32_t    const x  = v.x * m[0] + v.y * m[1] + m[2]; */
/*       f32_t    const y  = v.x * m[3] + v.y * m[4] + m[5]; */

/*       triangle->vertices[i].x = (int32_t) x; */
/*       triangle->vertices[i].y = (int32_t) y; */
/*     } */
/* } */
