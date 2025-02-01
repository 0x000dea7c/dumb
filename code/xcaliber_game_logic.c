#include "xcaliber_game_logic.h"

#include "xcaliber.h"
#include "xcaliber_renderer.h"
#include <SDL3/SDL.h>
#include <stdint.h>

XC_API void
game_update(__attribute__((unused)) xc_ctx *ctx)
{
	/* Only game physics and logic! */
}

XC_API void
game_render(xc_ctx *ctx, stack_arena *arena)
{
	xcr_triangle_colours test = {
		.vertices = { { 0, 0 }, { 1024, 0 }, { 512, 768 } },
		.colours = { { 0xFF, 0x0, 0x0, 0xFF }, { 0x00, 0xFF, 0x0, 0xFF }, { 0x00, 0x0, 0xFF, 0xFF } }
	};

	xcr_draw_triangle_filled_colours(ctx->renderer_ctx, arena, &test);

	// xcr_draw_triangle_filled(ctx->renderer_ctx, a, (xcr_triangle) { .vertices = { { 0, 0 }, { 1024, 0 }, { 512, 768 } } }, 0xFF00FFFF);
}
