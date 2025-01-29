#include "xcaliber_renderer.h"
#include "xcaliber_math.h"
#include "xcaliber_common.h"
#include "xcaliber.h"
#include "xcaliber_linear_arena.h"
#include "xcaliber_stack_arena.h"
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

typedef uint8_t pixel_mask;

static inline int32_t
get_simd_width(void)
{
#if defined(__AVX2__)
	return 8;
#elif defined(__SSE4_2__)
	return 4;
#else
	#error "fuck you, game engine needs at least SSE4.2"
#endif
}

#if defined(__AVX2__)
#define XC_FILL_PIXELS_SIMD_ALIGNED(dst, colour, count) \
    __asm__ volatile( \
        /* Load chunk count into rax */ \
        "movl %2, %%eax\n\t" \
        /* Broadcast 32-bit colour to all 8 lanes of ymm0 */ \
        "vpbroadcastd %0, %%ymm0\n\t" \
        /* Load destination buffer address into rcx */ \
        "movq %1, %%rcx\n\t" \
        /* Main loop label */ \
        "1:\n\t" \
        /* Store 8 pixels (256 bits) */ \
	"vmovdqa %%ymm0, (%%rcx)\n\t" \
        /* Move to next chunk (32 bytes = 8 pixels × 4 bytes) */ \
        "addq $32, %%rcx\n\t" \
        /* Decrement counter */ \
        "dec %%eax\n\t" \
        /* Loop if counter not zero */ \
        "jnz 1b\n\t" \
        : /* No outputs */ \
        : "m"(colour), /* %0: colour to broadcast */ \
          "r"(dst),    /* %1: destination buffer */ \
          "r"(count)   /* %2: number of chunks */ \
        : "ymm0", "eax", "rcx", "memory" \
    )

#define XC_FILL_PIXELS_SIMD_UNALIGNED(dst, colour, count) \
    __asm__ volatile( \
        /* Load chunk count into rax */ \
        "movl %2, %%eax\n\t" \
        /* Broadcast 32-bit colour to all 8 lanes of ymm0 */ \
        "vpbroadcastd %0, %%ymm0\n\t" \
        /* Load destination buffer address into rcx */ \
        "movq %1, %%rcx\n\t" \
        /* Main loop label */ \
        "1:\n\t" \
        /* Store 8 pixels (256 bits) */ \
	"vmovdqu %%ymm0, (%%rcx)\n\t" \
        /* Move to next chunk (32 bytes = 8 pixels × 4 bytes) */ \
        "addq $32, %%rcx\n\t" \
        /* Decrement counter */ \
        "dec %%eax\n\t" \
        /* Loop if counter not zero */ \
        "jnz 1b\n\t" \
        : /* No outputs */ \
        : "m"(colour), /* %0: colour to broadcast */	\
          "r"(dst),    /* %1: destination buffer */ \
          "r"(count)   /* %2: number of chunks */ \
        : "ymm0", "eax", "rcx", "memory" \
    )

#elif defined(__SSE4_2__)
#define XC_FILL_PIXELS_SIMD_ALIGNED(dst, colour, count, aligned) \
    __asm__ volatile( \
        /* Load chunk count into rax */ \
        "movl %2, %%eax\n\t" \
        /* Move 32-bit colour into lowest lane of xmm0 */ \
        "movd %0, %%xmm0\n\t" \
        /* Broadcast to all 4 lanes */ \
        "pshufd $0, %%xmm0, %%xmm0\n\t" \
        /* Load destination buffer address */ \
        "movq %1, %%rcx\n\t" \
        /* Main loop label */ \
        "1:\n\t" \
        /* Store 4 pixels (128 bits) aligned */ \
	"movdqa %%xmm0, (%%rcx)\n\t" \
        /* Move to next chunk (16 bytes = 4 pixels × 4 bytes) */ \
        "addq $16, %%rcx\n\t" \
        /* Decrement counter */ \
        "dec %%eax\n\t" \
        /* Loop if counter not zero */ \
        "jnz 1b\n\t" \
        : /* No outputs */ \
        : "m"(colour), /* %0: colour to broadcast */ \
          "r"(dst),    /* %1: destination buffer */ \
          "r"(count)   /* %2: number of chunks */ \
        : "xmm0", "eax", "rcx", "memory" \
    )

#define XC_FILL_PIXELS_SIMD_UNALIGNED(dst, colour, count)	\
    __asm__ volatile( \
        /* Load chunk count into rax */ \
        "movl %2, %%eax\n\t" \
        /* Broadcast 32-bit colour to all 8 lanes of ymm0 */ \
        "vpbroadcastd %0, %%ymm0\n\t" \
        /* Load destination buffer address into rcx */ \
        "movq %1, %%rcx\n\t" \
        /* Main loop label */ \
        "1:\n\t" \
        /* Store 8 pixels (256 bits) */ \
	"movdqu %%xmm0, (%%rcx)\n\t" \
        /* Move to next chunk (32 bytes = 8 pixels × 4 bytes) */ \
        "addq $32, %%rcx\n\t" \
        /* Decrement counter */ \
        "dec %%eax\n\t" \
        /* Loop if counter not zero */ \
        "jnz 1b\n\t" \
        : /* No outputs */ \
        : "m"(colour), /* %0: colour to broadcast */ \
          "r"(dst),    /* %1: destination buffer */ \
          "r"(count)   /* %2: number of chunks */ \
        : "ymm0", "eax", "rcx", "memory" \
    )

#else
#error "engine needs at least SSE4.2 support"
#endif

struct xcr_context {
	xc_framebuffer *fb;
};

static inline void
xcr_put_pixel(xcr_context *ctx, int32_t x, int32_t y, uint32_t colour)
{
#ifdef DEBUG
	if (x < 0 || x > ctx->fb->width || y < 0 || y > ctx->fb->height) {
		(void)fprintf(
			stderr,
			"WARNING: OUT OF BOUNDS FRAMEBUFFER UPDATE x: %d, y: %d\n",
			x, y);
		(void)fflush(stderr);
		return;
	}
#endif
	ctx->fb->pixels[y * ctx->fb->width + x] = colour;
}

static inline void
plot_points(xcr_context *ctx, xc_vec2i center, xc_vec2i p, uint32_t c)
{
	/* each point I compute gives me 8 points on the circle (symmetry) */

	/* octant 1 */
	xcr_put_pixel(ctx, (center.x + p.x), (center.y + p.y), c);
	/* octant 2 */
	xcr_put_pixel(ctx, (center.x + p.y), (center.y + p.x), c);
	/* octant 3 */
	xcr_put_pixel(ctx, (center.x - p.y), (center.y + p.x), c);
	/* octant 4 */
	xcr_put_pixel(ctx, (center.x - p.x), (center.y + p.y), c);

	/* octant 5 */
	xcr_put_pixel(ctx, (center.x - p.x), (center.y - p.y), c);
	/* octant 6 */
	xcr_put_pixel(ctx, (center.x - p.y), (center.y - p.x), c);
	/* octant 7 */
	xcr_put_pixel(ctx, (center.x + p.y), (center.y - p.x), c);
	/* octant 8 */
	xcr_put_pixel(ctx, (center.x + p.x), (center.y - p.y), c);
}

static void
draw_horizontal_line_bresenham(xcr_context *ctx, xc_vec2i p0, xc_vec2i p1,
			       uint32_t colour, int32_t dx, int32_t dy,
			       int32_t dy_abs)
{
	if (p1.x < p0.x) {
		XC_SWAP(int, p0.x, p1.x);
		XC_SWAP(int, p0.y, p1.y);
		dx = p1.x - p0.x;
		dy = p1.y - p0.y;
		dy_abs = XC_ABS(dy);
	}

	int32_t D = 2 * dy - dx;
	int32_t y = p0.y;
	int32_t y_step = (dy < 0) ? -1 : 1;

	for (int32_t x = p0.x; x <= p1.x; ++x) {
		xcr_put_pixel(ctx, x, y, colour);

		if (D > 0) {
			y += y_step;
			D += 2 * (dy_abs - dx);
		} else {
			D += 2 * dy_abs;
		}
	}
}

static void
draw_vertical_line_bresenham(xcr_context *ctx, xc_vec2i p0, xc_vec2i p1,
			     uint32_t colour, int32_t dx, int32_t dy,
			     int32_t dy_abs)
{
	XC_SWAP(int, p0.x, p0.y);
	XC_SWAP(int, p1.x, p1.y);

	if (p1.x < p0.x) {
		XC_SWAP(int, p0.x, p1.x);
		XC_SWAP(int, p0.y, p1.y);
	}

	dx = p1.x - p0.x;
	dy = p1.y - p0.y;
	dy_abs = XC_ABS(dy);

	int32_t D = 2 * dy - dx;
	int32_t y = p0.y;
	int32_t y_step = (dy < 0) ? -1 : 1;

	for (int32_t x = p0.x; x <= p1.x; ++x) {
		xcr_put_pixel(ctx, y, x, colour);

		if (D > 0) {
			y += y_step;
			D += 2 * (dy_abs - dx);
		} else {
			D += 2 * dy_abs;
		}
	}
}

static void
draw_line_bresenham(xcr_context *ctx, xc_vec2i p0, xc_vec2i p1, uint32_t colour)
{
	int32_t dx = p1.x - p0.x;
	int32_t dy = p1.y - p0.y;
	int32_t dy_abs = XC_ABS(dy);
	int32_t dx_abs = XC_ABS(dx);
	bool steep = dy_abs > dx_abs;

	if (steep) {
		draw_vertical_line_bresenham(ctx, p0, p1, colour, dx, dy, dy_abs);
	} else {
		draw_horizontal_line_bresenham(ctx, p0, p1, colour, dx, dy, dy_abs);
	}
}

static void
draw_circle_midpoint(xcr_context *ctx, xc_vec2i center, int32_t r,
		     uint32_t colour)
{
	/* start at the top! */
	xc_vec2i curr = { .x = 0, .y = r };
	int32_t D = 3 - (2 * r);

	plot_points(ctx, center, curr, colour);

	while (curr.y > curr.x) {
		/* move inward or not */
		if (D > 0) {
			--curr.y;
			D = D + 4 * (curr.x - curr.y) + 10;
		} else {
			D = D + 4 * curr.x + 6;
		}
		++curr.x;
		plot_points(ctx, center, curr, colour);
	}
}

/* puts everything into arr0 and frees arr1 */
static int32_t *
array_append(stack_arena *a, int32_t *arr0, int32_t *arr1, int32_t arr0_size, int32_t arr1_size)
{
	arr0 = stack_arena_resize(a, arr0, (uint64_t)arr0_size * sizeof(int32_t), (uint64_t)(arr0_size + arr1_size) * sizeof(int32_t));
	assert(arr0);

	int32_t j = 0;
	int32_t const n = arr0_size + arr1_size;

	for (int32_t i = arr0_size; i < n; ++i) {
		arr0[i] = arr1[j++];
	}

	/* don't need arr1 anymore */
	stack_arena_free(a, arr1);

	return arr0;
}

static void
draw_triangle_filled_interpolation(stack_arena *a, xcr_context *ctx, xcr_triangle T, uint32_t colour)
{
	/* algorithm from computers graphics from scratch, way better than my previous version */

	/* sort vertices so that the first vertex is always at the top */
	if (T.vertices[1].y < T.vertices[0].y) {
		XC_SWAP(xc_vec2i, T.vertices[0], T.vertices[1]);
	}

	if (T.vertices[2].y < T.vertices[0].y) {
		XC_SWAP(xc_vec2i, T.vertices[2], T.vertices[0]);
	}

	if (T.vertices[2].y < T.vertices[1].y) {
		XC_SWAP(xc_vec2i, T.vertices[2], T.vertices[1]);
	}

	int32_t const x01_size = T.vertices[1].y - T.vertices[0].y + 1;
	int32_t const x12_size = T.vertices[2].y - T.vertices[1].y + 1;
	int32_t const x02_size = T.vertices[2].y - T.vertices[0].y + 1;

	int32_t *x01 = xc_interpolate_array(a, T.vertices[0].y, T.vertices[0].x, T.vertices[1].y, T.vertices[1].x, x01_size);
	assert(x01);
	int32_t *x12 = xc_interpolate_array(a, T.vertices[1].y, T.vertices[1].x, T.vertices[2].y, T.vertices[2].x, x12_size);
	assert(x12);
	int32_t *x02 = xc_interpolate_array(a, T.vertices[0].y, T.vertices[0].x, T.vertices[2].y, T.vertices[2].x, x02_size);
	assert(x02);

	/* concatenate short sides */
	int32_t *x012 = array_append(a, x01, x12, x01_size - 1, x12_size);

	/* determine which array is the left and which one is the right */
	int32_t *x_left, *x_right;
	int32_t const mid = (x01_size + x12_size) >> 1;
	int32_t const simd_width = get_simd_width();

	if (x02[mid] < x012[mid]) {
		x_left = x02;
		x_right = x012;
	} else {
		x_left = x012;
		x_right = x02;
	}

	/* draw horizontally */
	for (int32_t y = T.vertices[0].y; y < T.vertices[2].y; ++y) {
		int32_t const x_start = x_left[y - T.vertices[0].y];
		int32_t const x_end = x_right[y - T.vertices[0].y];
		int32_t const w = x_end - x_start + 1;
		int32_t const chunks = w / simd_width;

		if (chunks > 0) {
			uint32_t *row = &ctx->fb->pixels[y * ctx->fb->width + x_start];
			XC_FILL_PIXELS_SIMD_UNALIGNED(row, colour, chunks);

			for (int32_t x = x_start + (chunks * simd_width); x <= x_end; ++x) {
				xcr_put_pixel(ctx, x, y, colour);
			}
		} else {
			for (int32_t x = x_start; x <= x_end; ++x) {
				xcr_put_pixel(ctx, x, y, colour);
			}
		}
	}
}

static pixel_mask
test_edge_simd(int32_t x, int32_t y, int32_t A, int32_t B, int32_t C)
{
	/* NOTE: it's a shame that I'm losing the multiplications of pixels * A, because they are needed later */
	pixel_mask result;

#if defined(__AVX2__)
	__asm__ volatile(
		/* load x into xmm0 */
		"vmovd %[x], %%xmm0\n\t"
		/* broadcast x to all 8 lanes, ymm0: [x, x, x, x ... x] */
		"vpbroadcastd %%xmm0, %%ymm0\n\t"
		/* create increment vector [0,1,2,3,4,5,6,7] */
		"vpcmpeqd %%ymm1, %%ymm1, %%ymm1\n\t"
		/* now ymm1 has [1,1,1,1,1,1,1,1] */
		"vpsrld $31, %%ymm1, %%ymm1\n\t"
		/* shift each lane by its position */
		"vpsllvd %%ymm1, %%ymm1, %%ymm1\n\t"
		/* add increments to x to get [x, x + 1, x + 2, ... x + 7] */
		"vpaddd %%ymm1, %%ymm0, %%ymm0\n\t"

		/* multiply by A, store A in xmm1 first */
		"vmovd %[A], %%xmm1\n\t"
		/* broadcast A, ymm1 = [A, A, A, ... A] */
		"vpbroadcastd %%xmm1, %%ymm1\n\t"
		/* do A * x */
		"vpmulld %%ymm0, %%ymm1, %%ymm0\n\t"

		/* add B * y */
		"movl %[y], %%eax\n\t"
		/* multiplies eax by B, result in eax */
		"imull %[B]\n\t"
		"vmovd %%eax, %%xmm1\n\t"
		"vpbroadcastd %%xmm1, %%ymm1\n\t"
		"vpaddd %%ymm1, %%ymm0, %%ymm0\n\t"

		/* add C */
		"vmovd %[C], %%xmm1\n\t"
		"vpbroadcastd %%xmm1, %%ymm1\n\t"
		"vpaddd %%ymm1, %%ymm0, %%ymm0\n\t"

		/* compare with zero to see which points are inside */

		/* zero register */
		"vpxor %%ymm1, %%ymm1, %%ymm1\n\t"
		/* is result > 0? */
		"vpcmpgtd %%ymm1, %%ymm0, %%ymm0\n\t"

		/* extract mask of results */
		"vpmovmskb %%ymm0, %%eax\n\t"
		"movb %%al, %[result]\n\t"

		/* = means that this is an output, and r tells the compiler to put this in any GPR */
		: [result] "=r"(result)
		: [x] "r"(x), [y] "r"(y), [A] "r"(A), [B] "r"(B), [C] "r"(C)
		: "eax", "ymm0", "ymm1", "memory");

#elif defined(__SSE4_2__)
	#error "not implemented yet"

#else
	#error "error: at least SSE4.2 support is expected"
#endif

	return result;
}

static void
compute_barycentric_coordinates_simd(int32_t A, int32_t B, int32_t C, f32_t area, int32_t x, int32_t y, f32_t *arr)
{
	/* NOTE: "rm" -> integer value, "m" floating point value*/
	/* NOTE 2: vmulps %%ymm1, %%ymm0, %%ymm0, source1, source2, destination, take the value of ymm1, multiply it with ymm0, store in ymm0 */
	/* NOTE 3: I already know this, but in case I forget: double %% is needed for GCC specific reasons (operand templates conflict) */
	f32_t const y_flt = (f32_t)B * (f32_t)y;
	f32_t const c_flt = (f32_t)C;
	f32_t const denom = 2.0f * area;

#if defined(__AVX2__)
	__asm__ volatile(
		/* load x into xmm0 */
		"vmovd %[x], %%xmm0\n\t"
		/* broadcast x to all 8 lanes, ymm0: [x, x, x, x ... x] */
		"vpbroadcastd %%xmm0, %%ymm0\n\t"
		/* create increment vector [0,1,2,3,4,5,6,7] */
		"vpcmpeqd %%ymm1, %%ymm1, %%ymm1\n\t"
		/* now ymm1 has [1,1,1,1,1,1,1,1] */
		"vpsrld $31, %%ymm1, %%ymm1\n\t"
		/* shift each lane by its position */
		"vpsllvd %%ymm1, %%ymm1, %%ymm1\n\t"
		/* add increments to x to get [x, x + 1, x + 2, ... x + 7] */
		"vpaddd %%ymm1, %%ymm0, %%ymm0\n\t"

		/* multiply by A, store A in xmm1 first */
		"vmovd %[A], %%xmm1\n\t"
		/* broadcast A, ymm1 = [A, A, A, ... A] */
		"vpbroadcastd %%xmm1, %%ymm1\n\t"
		/* do A * x */
		"vpmulld %%ymm0, %%ymm1, %%ymm0\n\t"

		/* now I will be dealing with floating points, so convert */
		"vcvtdq2ps %%ymm0, %%ymm0\n\t"

		/* load and add y_flt to each lane */
		"vbroadcastss %[y_flt], %%ymm1\n\t"
		"vaddps %%ymm1, %%ymm0, %%ymm0\n\t"

		/* load and add c_flt to each lane */
		"vbroadcastss %[c_flt], %%ymm1\n\t"
		"vaddps %%ymm1, %%ymm0, %%ymm0\n\t"

		/* now divide each lane by denom */
		"vbroadcastss %[denom], %%ymm1\n\t"
		"vdivps %%ymm1, %%ymm0, %%ymm0\n\t"

		/* store result in arr */
		"vmovups %%ymm0, %[result]\n\t"

		: [result] "=m" (*arr)
		: [x] "rm"(x), [A] "rm"(A), [y_flt] "m"(y_flt), [c_flt] "m"(c_flt), [denom] "m"(denom)
		: "ymm0", "ymm1", "memory"
		);
#elif defined(__SSE4_2__)
	#error "not implemented yet"
#endif
}

xcr_context *
xcr_create(linear_arena *arena, xc_framebuffer *fb)
{
	xcr_context *ctx = linear_arena_alloc(arena, sizeof(xcr_context));
	if (!ctx) {
		(void)fprintf(stderr, "Couldn't allocate space for renderer\n");
		return NULL;
	}

	ctx->fb = fb;

	return ctx;
}

void
xcr_set_bg_colour(xcr_context *ctx, uint32_t colour)
{
	uint32_t *pixels = ctx->fb->pixels;
	XC_FILL_PIXELS_SIMD_ALIGNED(pixels, colour, ctx->fb->simd_chunks);
}

void
xcr_draw_line(xcr_context *ctx, xc_vec2i p0, xc_vec2i p1, uint32_t colour)
{
	draw_line_bresenham(ctx, p0, p1, colour);
}

void
xcr_draw_quad_outline(xcr_context *ctx, xc_vec2i p0, int32_t width,
		      int32_t height, uint32_t colour)
{
	xc_vec2i p1 = { .x = p0.x, .y = p0.y + height };

	xc_vec2i p2 = { .x = p0.x + width, .y = p0.y };

	xc_vec2i p3 = { .x = p0.x + width, .y = p0.y + height };

	xcr_draw_line(ctx, p0, p2, colour);
	xcr_draw_line(ctx, p0, p1, colour);
	xcr_draw_line(ctx, p1, p3, colour);
	xcr_draw_line(ctx, p3, p2, colour);
}

void
xcr_draw_triangle_outline(xcr_context *ctx, xcr_triangle T, uint32_t colour)
{
	xcr_draw_line(ctx, T.vertices[0], T.vertices[1], colour);
	xcr_draw_line(ctx, T.vertices[1], T.vertices[2], colour);
	xcr_draw_line(ctx, T.vertices[2], T.vertices[0], colour);
}

void
xcr_draw_circle_outline(xcr_context *ctx, xc_vec2i center, int32_t r,
			uint32_t colour)
{
	draw_circle_midpoint(ctx, center, r, colour);
}

void
xcr_draw_quad_filled(xcr_context *ctx, xc_vec2i p0, int32_t width,
		     int32_t height, uint32_t colour)
{
	/* Just in case I pass some weird shit as arguments... Like intentionally
	   drawing at the edge of the screen */
	int32_t const xstart = XC_MAX(p0.x, 0);
	int32_t const ystart = XC_MAX(p0.y, 0);

	/* Careful not to go outside boundaries */
	int32_t const ymax = XC_MIN(p0.y + height, ctx->fb->height);
	int32_t const xmax = XC_MIN(p0.x + width, ctx->fb->width);

	for (int32_t y = ystart; y < ymax; ++y) {
		for (int32_t x = xstart; x < xmax; ++x) {
			xcr_put_pixel(ctx, x, y, colour);
		}
	}
}

void
xcr_draw_triangle_filled(xcr_context *ctx, stack_arena *a, xcr_triangle T, uint32_t colour)
{
	draw_triangle_filled_interpolation(a, ctx, T, colour);
}

void
xcr_draw_circle_filled(xcr_context *ctx, xc_vec2i center, int32_t r,
		       uint32_t colour)
{
	/* x² + y² = r² */
	int32_t const r_sq = r * r;
	int32_t ystart = XC_MAX(center.y - r, 0);
	int32_t yend = XC_MIN(center.y + r, ctx->fb->height - 1);

	for (int32_t y = ystart; y <= yend; ++y) {
		int32_t dy = y - center.y;
		int32_t width_sq = r_sq - (dy * dy);
		if (width_sq >= 0) {
			int32_t width = (int32_t)xc_sqrt((float)width_sq);
			int32_t xstart = XC_MAX(center.x - width, 0);
			int32_t xend = XC_MIN(center.x + width, ctx->fb->width - 1);

			for (int32_t x = xstart; x <= xend; ++x) {
				xcr_put_pixel(ctx, x, y, colour);
			}
		}
	}
}

void
xcr_draw_triangle_filled_colours(xcr_context *ctx, xcr_triangle_colours *T)
{
	/* sort triangles so that v0 is on top and v0 < v1 < v2 */
	if (T->vertices[1].y < T->vertices[0].y) {
		XC_SWAP(xc_vec2i, T->vertices[0], T->vertices[1]);
		XC_SWAP(xc_colour, T->colours[0], T->colours[1]);
	}

	if (T->vertices[2].y < T->vertices[0].y) {
		XC_SWAP(xc_vec2i, T->vertices[2], T->vertices[0]);
		XC_SWAP(xc_colour, T->colours[2], T->colours[0]);
	}

	if (T->vertices[2].y < T->vertices[1].y) {
		XC_SWAP(xc_vec2i, T->vertices[2], T->vertices[1]);
		XC_SWAP(xc_colour, T->colours[2], T->colours[1]);
	}

	/* grab triangle bounding box, doing bounds checking too */
	int32_t const xmin = XC_MAX(XC_MIN3(T->vertices[0].x, T->vertices[1].x, T->vertices[2].x), 0);
	int32_t const ymin = XC_MAX(XC_MIN3(T->vertices[0].y, T->vertices[1].y, T->vertices[2].y), 0);
	int32_t const xmax = XC_MIN(XC_MAX3(T->vertices[0].x, T->vertices[1].x, T->vertices[2].x), ctx->fb->width - 1);
	int32_t const ymax = XC_MIN(XC_MAX3(T->vertices[0].y, T->vertices[1].y, T->vertices[2].y), ctx->fb->height - 1);

	f32_t u, v, w;

	for (int32_t y = ymin; y <= ymax; ++y) {
		for (int32_t x = xmin; x <= xmax; ++x) {
			xc_barycentric((xc_vec2i){ x, y }, T->vertices, &u, &v, &w);

			/* point outside the triangle, skip */
			if (u > 1.0f || v > 1.0f || w > 1.0f || u < 0.0f || v < 0.0f || w < 0.0f) {
				continue;
			}

			uint8_t const r = (uint8_t)(T->colours[0].r * u + T->colours[1].r * v + T->colours[2].r * w);
			uint8_t const g = (uint8_t)(T->colours[0].g * u + T->colours[1].g * v + T->colours[2].g * w);
			uint8_t const b = (uint8_t)(T->colours[0].b * u + T->colours[1].b * v + T->colours[2].b * w);
			uint8_t const a = (uint8_t)(T->colours[0].a * u + T->colours[1].a * v + T->colours[2].a * w);

			uint32_t const c = (uint32_t)((r << 24) | (g << 16) | (b << 8) | a);

			xcr_put_pixel(ctx, x, y, c);
		}
	}
}
