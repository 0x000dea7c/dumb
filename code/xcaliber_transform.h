#ifndef XCALIBER_TRANSFORM_H
#define XCALIBER_TRANSFORM_H

#include "xcaliber_common.h"
#include "xcaliber_geometry.h"

typedef struct xc_transform
{
  xc_mat3x3 matrix;
} xc_transform;

/* a transformation can contain multiple things like rotation, scale and translations, so I
 * think it's a good idea to design it this way */
void xct_transform_triangle (xc_transform const *, xc_triangle *);

#endif
