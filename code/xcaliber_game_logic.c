#include "xcaliber_game_logic.h"

#include "xcaliber.h"
#include "xcaliber_renderer.h"
#include <SDL3/SDL.h>

GAME_API void
game_update(__attribute__((unused)) xc_ctx *ctx)
{
	/* Only game physics and logic! */
}

GAME_API void
game_render(xc_ctx *ctx)
{
	xcr_colour line_colour = { .r = 0x1B, .g = 0x17, .b = 0x28, .a = 0xFF };

	xcr_set_bg_colour(
		ctx->renderer_ctx,
		(xcr_colour){ .r = 0x00, .g = 0x00, .b = 0xFF, .a = 0xFF });

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

	xcr_draw_quad_outline(
		ctx->renderer_ctx, (xcr_point){ .x = 700, .y = 500 }, 200, 200,
		(xcr_colour){ .r = 0xFF, .g = 0, .b = 0, .a = 0xFF });

	xcr_draw_triangle_outline(
		ctx->renderer_ctx, (xcr_point){ .x = 550, .y = 500 },
		(xcr_point){ .x = 550, .y = 600 },
		(xcr_point){ .x = 600, .y = 550 },
		(xcr_colour){ .r = 0xA9, .g = 0x89, .b = 0x8D, .a = 0xFF });

	xcr_draw_circle_outline(
		ctx->renderer_ctx, (xcr_point){ .x = 200, .y = 500 }, 50,
		(xcr_colour){ .r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF });
}
