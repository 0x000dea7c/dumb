#include "xcaliber_game_logic.h"

#include "xcaliber.h"
#include "xcaliber_renderer.h"
#include "xcaliber_transform.h"

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

static void
demo1 (xc_context *ctx, stack_arena *arena)
{
  /* draws a bunch of shit */
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

XC_API void
game_update(__attribute__((unused)) xc_context *ctx)
{
}

XC_API void
game_render(xc_context *ctx, stack_arena *arena)
{
  // demo1 (ctx, arena);

  xcr_set_background_colour (ctx->renderer_ctx, 0x00);

  xc_triangle triangle = {
    .vertices = { { 300, 256 }, { 600, 256 }, { 450, 512 } }
  };

  static f32_t rotation_radians = 0.0f;

  f32_t const centroid_x = (triangle.vertices[0].x + triangle.vertices[1].x + triangle.vertices[2].x) / 3.0f;
  f32_t const centroid_y = (triangle.vertices[0].y + triangle.vertices[1].y + triangle.vertices[2].y) / 3.0f;

  xc_transform const to_origin = {
    .matrix = {
      .values = {
        1.0f, 0.0f, -centroid_x,
        0.0f, 1.0f, -centroid_y,
        0.0f, 0.0f, 1.0f
      }
    }
  };

  xc_transform const rotation_matrix = {
    .matrix = {
      .values = {
        xc_cos (rotation_radians), -xc_sin (rotation_radians), 0.0f,
        xc_sin (rotation_radians),  xc_cos (rotation_radians), 0.0f,
        0.0f,                       0.0f,                      1.0f }
    }
  };

  xc_transform const to_position = {
    .matrix = { .values = { 1.0f, 0.0f, centroid_x,
                            0.0f, 1.0f, centroid_y,
                            0.0f, 0.0f, 1.0f } }
  };

  xct_transform_triangle (&to_origin, &triangle);
  xct_transform_triangle (&rotation_matrix, &triangle);
  xct_transform_triangle (&to_position, &triangle);

  xcr_draw_triangle_filled (ctx->renderer_ctx, arena, triangle, xc_preset_colour (XC_GREEN));

  xc_triangle triangle2 = triangle;
  triangle2.vertices[0].x += 300;
  triangle2.vertices[1].x += 300;
  triangle2.vertices[2].x += 300;

  xc_triangle triangle3 = triangle;
  triangle3.vertices[0].x -= 250;
  triangle3.vertices[1].x -= 250;
  triangle3.vertices[2].x -= 250;

  xcr_draw_triangle_filled (ctx->renderer_ctx, arena, triangle2, xc_preset_colour (XC_RED));

  xcr_draw_triangle_filled (ctx->renderer_ctx, arena, triangle3, xc_preset_colour (XC_PURPLE));

  rotation_radians = fmodf (rotation_radians + (ctx->fixed_timestep * 0.2f), XC_TWO_PI);
}
