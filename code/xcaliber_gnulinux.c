#include "xcaliber.h"
#include "xcaliber_common.h"
#include "xcaliber_hot_reload.h"
#include "xcaliber_linear_arena.h"
#include "xcaliber_stack_arena.h"
#include "xcaliber_renderer.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define XC_GAME_TOTAL_MEMORY XC_MEGABYTES(256ull)
#define XC_SCRATCH_MEMORY XC_MEGABYTES(32ull)
#define FIXED_TIMESTEP 1.0f / 60.0f
#define GAME_LOGIC_SHARED_LIBRARY_NAME "libgamelogic.so"

/* SDL globals */
static SDL_Window *sdl_window = NULL;
static SDL_Texture *sdl_texture = NULL;
static SDL_Renderer *sdl_renderer = NULL;

/* MY globals, hehe */
static unsigned char *game_memory = NULL;
static unsigned char *game_scratch_memory = NULL;
static linear_arena *main_arena = NULL;
static stack_arena *scratch_arena = NULL;
static xc_hot_reload_lib_info game_logic_shared_library;
static xc_ctx game_context;
static xc_cfg game_config;
static xc_framebuffer game_framebuffer;

static void
quit (void)
{
  if (sdl_texture)
    {
      SDL_DestroyTexture (sdl_texture);
    }

  if (sdl_renderer)
    {
      SDL_DestroyRenderer (sdl_renderer);
    }

  if (sdl_window)
    {
      SDL_DestroyWindow (sdl_window);
    }

  if (game_memory)
    {
      free (game_memory);
    }

  if (game_scratch_memory)
    {
      free (game_scratch_memory);
    }

  if (main_arena)
    {
      linear_arena_destroy (main_arena);
    }

  if (scratch_arena)
    {
      stack_arena_destroy (scratch_arena);
    }

  SDL_Quit();
}

static void panic (char const *title, char const *msg) __attribute__((noreturn));

static void
panic (char const *title, char const *msg)
{
  (void) fprintf (stderr, "%s - %s\n", title, msg);

  quit();

  exit(EXIT_FAILURE);
}

static void
sdl_init (void)
{
  if (!SDL_Init (SDL_INIT_VIDEO))
    {
      panic ("SDL_Init", SDL_GetError());
    }

  sdl_window = SDL_CreateWindow ("X-Caliber", game_config.width, game_config.height, 0);
  if (!sdl_window)
    {
      panic ("SDL_CreateWindow", SDL_GetError());
    }

  sdl_renderer = SDL_CreateRenderer (sdl_window, NULL);
  if (!sdl_renderer)
    {
      panic ("SDL_CreateRenderer", SDL_GetError ());
    }

  sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                  game_config.width, game_config.height);
  if (!sdl_texture)
    {
      panic ("SDL_CreateTexture", SDL_GetError ());
    }
}

static void
game_config_init (void)
{
  game_config.width = 1024;
  game_config.height = 768;
  game_config.target_fps = FIXED_TIMESTEP;
  game_config.vsync = false;
}

static void
game_memory_init (void)
{
  main_arena = linear_arena_create ();
  assert(main_arena);

  scratch_arena = stack_arena_create ();
  assert(scratch_arena);

  game_memory = malloc (XC_GAME_TOTAL_MEMORY);
  assert(game_memory);

  game_scratch_memory = malloc (XC_SCRATCH_MEMORY);
  assert(game_scratch_memory);

  linear_arena_init (main_arena, game_memory, XC_GAME_TOTAL_MEMORY);

  stack_arena_init (scratch_arena, game_scratch_memory, XC_SCRATCH_MEMORY);
}

static void
game_context_init (void)
{
  game_context.renderer_ctx = NULL;
  game_context.physics_accumulator = 0.0f;
  game_context.fixed_timestep = FIXED_TIMESTEP;
  game_context.alpha = 0.0f;
  game_context.running = true;
  game_context.last_frame_time = SDL_GetTicks ();

  if (game_config.vsync)
    {
      SDL_SetRenderVSync (sdl_renderer, 1);
    }
}

static void
toggle_vsync (void)
{
  SDL_SetRenderVSync (sdl_renderer, game_config.vsync ? 0 : 1);
  game_config.vsync = !game_config.vsync;
  printf ("VSync value changed to: %d\n", game_config.vsync);
}

static void
game_framebuffer_init (void)
{
  game_framebuffer.width = game_config.width;
  game_framebuffer.height = game_config.height;
  game_framebuffer.pitch = game_framebuffer.width * (int32_t)sizeof(game_framebuffer.width);
  game_framebuffer.pixel_count = (uint32_t)game_framebuffer.width * (uint32_t)game_framebuffer.height;
  game_framebuffer.byte_size = game_framebuffer.pixel_count * sizeof(uint32_t);
  game_framebuffer.simd_chunks = (int32_t)(game_framebuffer.pixel_count / 8);
  game_framebuffer.pixels = linear_arena_alloc(main_arena, game_framebuffer.byte_size);
  if (!game_framebuffer.pixels)
    {
      panic ("framebuffer init", "Couldn't allocate space for the framebuffer");
    }
}

static void
game_renderer_init (void)
{
  game_context.renderer_ctx = xcr_create (main_arena, &game_framebuffer);

  if (!game_context.renderer_ctx)
    {
      panic ("game_renderer_init", "Couldn't create renderer");
    }
}

static void
game_hot_reload_init (void)
{
  if (!xc_hot_reload_init (&game_logic_shared_library, GAME_LOGIC_SHARED_LIBRARY_NAME))
    {
      panic ("game_hot_reload_init", "couldn't load game's logic shared library!");
    }
}

void
run (void)
{
  uint64_t frame_count = 0;
  uint64_t last_time = SDL_GetTicks ();
  uint64_t fps_update_time = last_time;
  f32_t current_fps = 0.0f;
  char window_title[32];
  SDL_Event event;

  while (game_context.running)
    {
      /* check if the lib was modified, if so, reload it. This is non blocking! */
      if (xc_hot_reload_lib_was_modified())
        {
          xc_hot_reload_update(&game_logic_shared_library);
        }

      uint64_t const current_time = SDL_GetTicks();
      float frame_time = (f32_t)(current_time - last_time) / 1000.0f;
      last_time = current_time;

      /* Cap max frame rate, avoid spiral of death, that is to say, constantly trying to catch up
         if I miss a deadline */
      if (frame_time > 0.25f)
        {
          frame_time = 0.25f;
        }

      /* FPS display every second */
      uint64_t const time_since_fps_update = current_time - fps_update_time;
      if (time_since_fps_update > 1000)
        {
          current_fps = (f32_t)frame_count * 1000.0f / (f32_t)time_since_fps_update;
          (void) snprintf (window_title, sizeof (window_title), "X-Caliber FPS: %.2f", current_fps);
          SDL_SetWindowTitle (sdl_window, window_title);
          frame_count = 0;
          fps_update_time = current_time;
        }

      game_context.physics_accumulator += frame_time;

      /* process input */
      while (SDL_PollEvent(&event))
        {

          if (event.type == SDL_EVENT_QUIT)
            {
              game_context.running = false;
              break;
            }

          if (event.type == SDL_EVENT_KEY_DOWN)
            {
              SDL_Keycode const key = event.key.key;
              switch (key)
                {
                case SDLK_ESCAPE:
                  game_context.running = false;
                  break;
                case SDLK_Q:
                  printf("Pressed Q!\n");
                  break;
                case SDLK_F1:
                  toggle_vsync();
                  break;
                default:
                  break;
                }
            }
        }

      /* fixed timestep physics and logic updates */
      while (game_context.physics_accumulator >= game_context.fixed_timestep)
        {
          game_logic_shared_library.update (&game_context);
          game_context.physics_accumulator -= game_context.fixed_timestep;
        }

      /* render as fast as possible with interpolation */
      game_context.alpha = game_context.physics_accumulator / game_context.fixed_timestep;
      game_logic_shared_library.render (&game_context, scratch_arena);

      /* copy my updated framebuffer to the GPU texture */
      SDL_UpdateTexture (sdl_texture, NULL, game_framebuffer.pixels, game_framebuffer.pitch);

      /* go render brr */
      SDL_RenderClear (sdl_renderer);
      SDL_RenderTexture (sdl_renderer, sdl_texture, NULL, NULL);
      SDL_RenderPresent (sdl_renderer);

      ++frame_count;

      stack_arena_free_all (scratch_arena);
    }
}

int
main(void)
{
  game_config_init();
  sdl_init();
  game_memory_init();
  game_context_init();
  game_framebuffer_init();
  game_renderer_init();
  game_hot_reload_init();

  run();

  quit();

  return EXIT_SUCCESS;
}
