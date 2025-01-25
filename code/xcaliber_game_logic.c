#include "xcaliber_game_logic.h"

#include "xcaliber.h"
#include "xcaliber_renderer.h"
#include "xcaliber_colour.h"
#include <SDL3/SDL.h>
#include <stdint.h>

XC_API void
game_update(__attribute__((unused)) xc_ctx *ctx)
{
	/* Only game physics and logic! */
}

XC_API void
game_render(xc_ctx *ctx, stack_arena *a)
{
	uint32_t line_colour = xc_preset_colour(XC_RED);

	xcr_set_bg_colour(ctx->renderer_ctx, xc_preset_colour(XC_OLIVE));

	/* L */
	xcr_draw_line(ctx->renderer_ctx, (xc_vec2i){ .x = 50, .y = 100 }, (xc_vec2i){ .x = 50, .y = 400 }, line_colour);
	xcr_draw_line(ctx->renderer_ctx, (xc_vec2i){ .x = 50, .y = 400 }, (xc_vec2i){ .x = 200, .y = 400 },
		      line_colour);

	/* A */
	xcr_draw_line(ctx->renderer_ctx, (xc_vec2i){ .x = 320, .y = 100 }, (xc_vec2i){ .x = 220, .y = 400 },
		      line_colour);
	xcr_draw_line(ctx->renderer_ctx, (xc_vec2i){ .x = 255, .y = 300 }, (xc_vec2i){ .x = 385, .y = 300 },
		      line_colour);
	xcr_draw_line(ctx->renderer_ctx, (xc_vec2i){ .x = 320, .y = 100 }, (xc_vec2i){ .x = 420, .y = 400 },
		      line_colour);

	/* I */
	xcr_draw_line(ctx->renderer_ctx, (xc_vec2i){ .x = 440, .y = 100 }, (xc_vec2i){ .x = 440, .y = 400 },
		      line_colour);

	/* N */
	xcr_draw_line(ctx->renderer_ctx, (xc_vec2i){ .x = 520, .y = 100 }, (xc_vec2i){ .x = 520, .y = 400 },
		      line_colour);
	xcr_draw_line(ctx->renderer_ctx, (xc_vec2i){ .x = 520, .y = 100 }, (xc_vec2i){ .x = 650, .y = 400 },
		      line_colour);
	xcr_draw_line(ctx->renderer_ctx, (xc_vec2i){ .x = 650, .y = 100 }, (xc_vec2i){ .x = 650, .y = 400 },
		      line_colour);

	xcr_draw_quad_outline(ctx->renderer_ctx, (xc_vec2i){ .x = 700, .y = 500 }, 200, 200,
			      xc_preset_colour(XC_WHITE));

	xcr_draw_triangle_outline(ctx->renderer_ctx,
				  (xcr_triangle){ .vertices = { { .x = 550, .y = 500 },
								{ .x = 550, .y = 600 },
								{ .x = 600, .y = 550 } } },
				  xc_preset_colour(XC_BLACK));

	xcr_draw_circle_outline(ctx->renderer_ctx, (xc_vec2i){ .x = 200, .y = 500 }, 50, xc_preset_colour(XC_WHITE));

	xcr_draw_quad_filled(ctx->renderer_ctx, (xc_vec2i){ .x = 400, .y = 500 }, 50, 50, xc_preset_colour(XC_BLACK));

	xcr_draw_quad_filled(ctx->renderer_ctx, (xc_vec2i){ .x = 400, .y = 750 }, 50, 50, xc_preset_colour(XC_RED));

	xcr_draw_quad_filled(ctx->renderer_ctx, (xc_vec2i){ .x = 900, .y = 150 }, 50, 50, xc_preset_colour(XC_WHITE));

	xcr_draw_triangle_filled(ctx->renderer_ctx, a,
				 (xcr_triangle){ { { .x = 0, .y = 0 }, { .x = 256, .y = 0 }, { .x = 128, .y = 128 } } },
				 xc_preset_colour(XC_PURPLE));

	xcr_draw_triangle_filled(
		ctx->renderer_ctx, a,
		(xcr_triangle){ { { .x = 0, .y = 768 }, { .x = 256, .y = 768 }, { .x = 128, .y = 640 } } },
		xc_preset_colour(XC_WHITE));

	xcr_draw_circle_filled(ctx->renderer_ctx, (xc_vec2i){ .x = 750, .y = 350 }, 50, xc_preset_colour(XC_PURPLE));

	xcr_triangle_colours test = {
		.vertices = { { 500, 700 }, { 1024, 700 }, { 672, 500 } },
		.colours = { { 0xFF, 0x0, 0x0, 0xFF }, { 0x00, 0xFF, 0x0, 0xFF }, { 0x00, 0x0, 0xFF, 0xFF } }
	};

	xcr_draw_triangle_filled_colours(ctx->renderer_ctx, &test);
}
