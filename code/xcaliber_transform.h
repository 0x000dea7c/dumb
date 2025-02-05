#ifndef XCALIBER_TRANSFORM_H
#define XCALIBER_TRANSFORM_H

#include "xcaliber_common.h"
#include "xcaliber_geometry.h"

typedef struct xc_transform
{
  xc_mat3x3 matrix;
} xc_transform;

/* void xct_transform_triangle (xc_transform const *, xc_triangle *); */

#endif
