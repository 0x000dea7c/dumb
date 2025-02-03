#include "xcaliber_game_logic.h"

#include "xcaliber.h"
#include "xcaliber_renderer.h"

#include <SDL3/SDL.h>
#include <stdint.h>

XC_API void
game_update(__attribute__((unused)) xc_context *ctx)
{
}

XC_API void
game_render(xc_context *ctx, stack_arena *arena)
{
  xcr_set_background_colour (ctx->renderer_ctx, 0x00);
  int32_t x = 0;
  int32_t y = 0;

  for (int i = 0; i < 8; ++i)
    {
      xc_shaded_triangle test = {
        .vertices = { { x, 0 }, { x + 128, 0 }, { x + 64, 64 } },
        .colours = { { 0xFF, 0x0, 0x0, 0xFF }, { 0x00, 0xFF, 0x0, 0xFF }, { 0x00, 0x0, 0xFF, 0xFF } }
      };

      xcr_draw_shaded_triangle_filled (ctx->renderer_ctx, arena, &test);

      xc_shaded_triangle test2 = (xc_shaded_triangle) {
        .vertices = { { x, 768 }, { x + 128, 768 }, { x + 64, 704 } },
        .colours = { { 0xFF, 0xAC, 0x0, 0xFF }, { 0x00, 0xFF, 0x90, 0xFF }, { 0x20, 0x0, 0xFF, 0xFF } }
      };

      xcr_draw_shaded_triangle_filled (ctx->renderer_ctx, arena, &test2);

      xc_triangle test3 = {
        .vertices = { { 0, y }, { 0, y + 96 }, { 32, y + 48 } }
      };

      xcr_draw_triangle_filled (ctx->renderer_ctx, arena, test3, (xc_colour) { 0x00, 0xFF, 0x00, 0xFF });

      xc_triangle test4 = {
        .vertices = { { 1023, y }, { 1023, y + 96 }, { 992, y + 48 } }
      };

      xcr_draw_triangle_filled (ctx->renderer_ctx, arena, test4, (xc_colour) { 0xFF, 0xFF, 0x00, 0xFF });

      x += 128;
      y += 96;
    }

  xcr_draw_quad_filled (ctx->renderer_ctx,
                        (xc_quad) { .position = { 128, 128 }, .width = 100, .height = 100 },
                        (xc_colour) { 0x00, 0x00, 0xFF, 0xFF });

  xcr_draw_circle_outline (ctx->renderer_ctx,
                           (xc_circle) { .center = { 512, 184 }, .radius = 50 },
                           (xc_colour) { 0xFF, 0x00, 0x00, 0xFF });

  xcr_draw_circle_filled (ctx->renderer_ctx,
                          (xc_circle) { .center = { 512, 384 }, .radius = 50 },
                          (xc_colour) { 0xFF, 0xCA, 0x70, 0xFF });

  xcr_draw_circle_outline (ctx->renderer_ctx,
                           (xc_circle) { .center = { 512, 584 }, .radius = 50 },
                           (xc_colour) { 0x00, 0xFF, 0x00, 0xFF });

  xcr_draw_line (ctx->renderer_ctx,
                 (xc_line) { .start = { 668, 192 }, .end = { 968, 192 } },
                 (xc_colour) { 0xFF, 0xFF, 0xFF, 0xFF });

  xcr_draw_line (ctx->renderer_ctx,
                 (xc_line) { .start = { 668, 292 }, .end = { 968, 292 } },
                 (xc_colour) { 0xFF, 0xFF, 0xFF, 0xFF });

  xcr_draw_line (ctx->renderer_ctx,
                 (xc_line) { .start = { 668, 392 }, .end = { 968, 392 } },
                 (xc_colour) { 0xFF, 0xFF, 0xFF, 0xFF });

  xcr_draw_line (ctx->renderer_ctx,
                 (xc_line) { .start = { 668, 492 }, .end = { 968, 492 } },
                 (xc_colour) { 0xFF, 0xFF, 0xFF, 0xFF });

  xcr_draw_line (ctx->renderer_ctx,
                 (xc_line) { .start = { 668, 592 }, .end = { 968, 592 } },
                 (xc_colour) { 0xFF, 0xFF, 0xFF, 0xFF });

  xcr_draw_quad_outline (ctx->renderer_ctx,
                         (xc_quad) { .position = { 128, 428 }, .width = 100, .height = 100 },
                         (xc_colour) { 0x00, 0x00, 0xFF, 0xFF });
}
