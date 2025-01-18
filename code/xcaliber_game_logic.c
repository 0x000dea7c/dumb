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
game_render(xc_ctx *ctx)
{
	uint32_t line_colour = xc_preset_colour(XC_BLACK);

	xcr_set_bg_colour(ctx->renderer_ctx, xc_preset_colour(XC_BLUE));

	/* L */
	xcr_draw_line(ctx->renderer_ctx, (xcr_point){ .x = 50, .y = 100 },
		      (xcr_point){ .x = 50, .y = 400 }, line_colour);
	xcr_draw_line(ctx->renderer_ctx, (xcr_point){ .x = 50, .y = 400 },
		      (xcr_point){ .x = 200, .y = 400 }, line_colour);

	/* A */
	xcr_draw_line(ctx->renderer_ctx, (xcr_point){ .x = 320, .y = 100 },
		      (xcr_point){ .x = 220, .y = 400 }, line_colour);
	xcr_draw_line(ctx->renderer_ctx, (xcr_point){ .x = 255, .y = 300 },
		      (xcr_point){ .x = 385, .y = 300 }, line_colour);
	xcr_draw_line(ctx->renderer_ctx, (xcr_point){ .x = 320, .y = 100 },
		      (xcr_point){ .x = 420, .y = 400 }, line_colour);

	/* I */
	xcr_draw_line(ctx->renderer_ctx, (xcr_point){ .x = 440, .y = 100 },
		      (xcr_point){ .x = 440, .y = 400 }, line_colour);

	/* N */
	xcr_draw_line(ctx->renderer_ctx, (xcr_point){ .x = 520, .y = 100 },
		      (xcr_point){ .x = 520, .y = 400 }, line_colour);
	xcr_draw_line(ctx->renderer_ctx, (xcr_point){ .x = 520, .y = 100 },
		      (xcr_point){ .x = 650, .y = 400 }, line_colour);
	xcr_draw_line(ctx->renderer_ctx, (xcr_point){ .x = 650, .y = 100 },
		      (xcr_point){ .x = 650, .y = 400 }, line_colour);

	xcr_draw_quad_outline(ctx->renderer_ctx,
			      (xcr_point){ .x = 700, .y = 500 }, 200, 200,
			      xc_preset_colour(XC_WHITE));

	xcr_draw_triangle_outline(
		ctx->renderer_ctx,
		(xcr_triangle){ .p0 = { .x = 550, .y = 500 },
				.p1 = { .x = 550, .y = 600 },
				.p2 = { .x = 600, .y = 550 } },
		xc_preset_colour(XC_BLACK));

	xcr_draw_circle_outline(ctx->renderer_ctx,
				(xcr_point){ .x = 200, .y = 500 }, 50,
				xc_preset_colour(XC_WHITE));

	xcr_draw_quad_filled(ctx->renderer_ctx,
			     (xcr_point){ .x = 400, .y = 500 }, 50, 50,
			     xc_preset_colour(XC_BLACK));

	xcr_draw_quad_filled(ctx->renderer_ctx,
			     (xcr_point){ .x = 400, .y = 750 }, 50, 50,
			     xc_preset_colour(XC_RED));

	xcr_draw_quad_filled(ctx->renderer_ctx,
			     (xcr_point){ .x = 900, .y = 150 }, 50, 50,
			     xc_preset_colour(XC_WHITE));

	xcr_draw_triangle_filled(ctx->renderer_ctx,
				 (xcr_triangle){ .p0 = { .x = 350, .y = 400 },
						 .p1 = { .x = 350, .y = 500 },
						 .p2 = { .x = 400, .y = 450 } },
				 xc_preset_colour(XC_RED));

	xcr_draw_circle_filled(ctx->renderer_ctx,
			       (xcr_point){ .x = 750, .y = 350 }, 50,
			       xc_preset_colour(XC_PURPLE));
}
