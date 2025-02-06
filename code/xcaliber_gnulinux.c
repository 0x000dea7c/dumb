#include "hyper.h"
#include "hyper_common.h"
#include "hyper_hot_reload.h"
#include "hyper_linear_arena.h"
#include "hyper_stack_arena.h"
#include "hyper_renderer.h"
#include "hyper_math.h"

#include <SDL3/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define TOTAL_MEMORY HYPER_MEGABYTES(256ull)
#define SCRATCH_MEMORY HYPER_MEGABYTES(32ull)
#define FIXED_TIMESTEP 1.0f / 60.0f
#define GAME_LOGIC_SHARED_LIBRARY_NAME "libgamelogic.so"

/* SDL globals */
static SDL_Window *sdl_window = NULL;
static SDL_Texture *sdl_texture = NULL;
static SDL_Renderer *sdl_renderer = NULL;

/* MY globals, hehe */
static u8 *game_memory = NULL;
static u8 *game_scratch_memory = NULL;

/* TODO: some of these variables should be internal */
static hyper_linear_arena main_arena;
static hyper_stack_arena scratch_arena;
static hyper_hot_reload_library_data game_logic_shared_library;
static hyper_frame_context game_frame_context;
static hyper_config game_config;
static hyper_framebuffer game_framebuffer;
static hyper_renderer_context game_renderer_context;
static hyper_vertex_buffer game_vertex_buffer;

static void
game_quit (void)
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

  SDL_Quit();
}

static void panic (char const *title, char const *msg) __attribute__((noreturn));

static void
panic (char const *title, char const *msg)
{
  (void) fprintf (stderr, "%s - %s\n", title, msg);

  game_quit();

  exit(EXIT_FAILURE);
}

static void
game_sdl_init (void)
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
  game_memory = malloc (TOTAL_MEMORY);
  assert(game_memory);

  game_scratch_memory = malloc (SCRATCH_MEMORY);
  assert(game_scratch_memory);

  hyper_linear_arena_init (&main_arena, game_memory, TOTAL_MEMORY);

  hyper_stack_arena_init (&scratch_arena, game_scratch_memory, SCRATCH_MEMORY);
}

static void
game_frame_context_init (void)
{
  game_frame_context.renderer_context = &game_renderer_context;
  game_frame_context.physics_accumulator = 0.0f;
  game_frame_context.fixed_timestep = FIXED_TIMESTEP;
  game_frame_context.alpha = 0.0f;
  game_frame_context.running = true;
  game_frame_context.last_frame_time = SDL_GetTicks ();

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
  game_framebuffer.width       = game_config.width;
  game_framebuffer.height      = game_config.height;
  game_framebuffer.pitch       = game_framebuffer.width * (i32) sizeof(u32);
  game_framebuffer.pixel_count = (u32) game_framebuffer.width * (u32) game_framebuffer.height;
  game_framebuffer.byte_size   = game_framebuffer.pixel_count * sizeof (u32);
  game_framebuffer.simd_chunks = (i32) (game_framebuffer.pixel_count / 8);
  game_framebuffer.pixels      = hyper_linear_arena_alloc (&main_arena, game_framebuffer.byte_size);
  if (!game_framebuffer.pixels)
    {
      panic ("framebuffer init", "Couldn't allocate space for the framebuffer");
    }
}

static void
game_hot_reload_init (void)
{
  if (!hyper_hot_reload_init (&game_logic_shared_library, GAME_LOGIC_SHARED_LIBRARY_NAME))
    {
      panic ("game_hot_reload_init", "couldn't load game's logic shared library!");
    }
}

static void
game_renderer_init (void)
{
  game_renderer_context.framebuffer = &game_framebuffer;
  game_renderer_context.vertex_buffer = &game_vertex_buffer;
}

void
game_run (void)
{
  u64 frame_count = 0;
  u64 last_time = SDL_GetTicks ();
  u64 fps_update_time = last_time;
  f32 current_fps = 0.0f;
  char window_title[32];
  SDL_Event event;

  while (game_frame_context.running)
    {
      /* check if the lib was modified, if so, reload it. This is non blocking! */
      if (hyper_hot_reload_library_was_updated ())
        {
          hyper_hot_reload_load (&game_logic_shared_library);
        }

      u64 const current_time = SDL_GetTicks();
      float frame_time = (f32)(current_time - last_time) / 1000.0f;
      last_time = current_time;

      /* Cap max frame rate, avoid spiral of death, that is to say, constantly trying to catch up
         if I miss a deadline */
      if (frame_time > 0.25f)
        {
          frame_time = 0.25f;
        }

      /* FPS display every second */
      u64 const time_since_fps_update = current_time - fps_update_time;
      if (time_since_fps_update > 1000)
        {
          current_fps = (f32)frame_count * 1000.0f / (f32)time_since_fps_update;
          (void) snprintf (window_title, sizeof (window_title), "X-Caliber FPS: %.2f", current_fps);
          SDL_SetWindowTitle (sdl_window, window_title);
          frame_count = 0;
          fps_update_time = current_time;
        }

      game_frame_context.physics_accumulator += frame_time;

      /* TEMP: make room for the vertex buffer */
      game_vertex_buffer.positions = (f32 *) hyper_stack_arena_alloc (&scratch_arena, sizeof (f32) * 6);
      game_vertex_buffer.normals = (f32 *) hyper_stack_arena_alloc (&scratch_arena, sizeof (f32) * 2);
      game_vertex_buffer.texture_coordinates = (f32 *) hyper_stack_arena_alloc (&scratch_arena, sizeof (f32) * 2);
      game_vertex_buffer.colours = (f32 *) hyper_stack_arena_alloc (&scratch_arena, sizeof (f32) * 12);

      game_vertex_buffer.positions = (f32 [6]) { -1.0f, -1.0f, 1.0f, -1.0f, 0.5f, 0.5f };
      game_vertex_buffer.normals = (f32 [2]) { 0.0f, 0.0f };
      game_vertex_buffer.texture_coordinates = (f32 [2]) { 0.0f, 0.0f };
      game_vertex_buffer.colours = (f32 [12]) { 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f };
      game_vertex_buffer.count = 1;

      /* process input */
      while (SDL_PollEvent(&event))
        {

          if (event.type == SDL_EVENT_QUIT)
            {
              game_frame_context.running = false;
              break;
            }

          if (event.type == SDL_EVENT_KEY_DOWN)
            {
              SDL_Keycode const key = event.key.key;
              switch (key)
                {
                case SDLK_ESCAPE:
                  game_frame_context.running = false;
                  break;
                case SDLK_Q:
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
      while (game_frame_context.physics_accumulator >= game_frame_context.fixed_timestep)
        {
          game_logic_shared_library.update (&game_frame_context);
          game_frame_context.physics_accumulator -= game_frame_context.fixed_timestep;
        }

      /* render as fast as possible with interpolation */
      game_frame_context.alpha = game_frame_context.physics_accumulator / game_frame_context.fixed_timestep;
      game_logic_shared_library.render (&game_frame_context);

      /* copy my updated framebuffer to the GPU texture */
      SDL_UpdateTexture (sdl_texture, NULL, game_framebuffer.pixels, game_framebuffer.pitch);

      /* go render brr */
      SDL_RenderClear (sdl_renderer);
      SDL_RenderTexture (sdl_renderer, sdl_texture, NULL, NULL);
      SDL_RenderPresent (sdl_renderer);

      ++frame_count;

      hyper_stack_arena_free_all (&scratch_arena);
    }
}

int
main(void)
{
  game_config_init ();
  game_sdl_init();
  game_memory_init ();
  game_frame_context_init ();
  game_framebuffer_init ();
  game_renderer_init ();
  game_hot_reload_init ();

  game_run ();

  game_quit ();

  return EXIT_SUCCESS;
}
