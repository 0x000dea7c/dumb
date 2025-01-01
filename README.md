# xcaliber

Game inspired by XCaliber from TempleOS.

# Dependencies

- SDL3

# Why C

- To force myself to simplify as much as possible.
- To better understand what my computer is doing.
- To implement as much as I can by myself and learn.

# Philosophy

- KISS: aim for simplicity in design and implementation.
- Clean naming (very hard): descriptive and concise.
- Minimalism: avoid unnecessary complexity. Not needed? don't include it.
- Modularity: break into modules, as independent as possible.
- Clean interfaces (.h files).
- Fail fast and provide informative error messages.
- Avoid using malloc/free within a game's frame. Use memory pools.
- Consistent code style. Try to emulate OpenBSD's.
- Comments explain why, not what.
- Optimise for clarity first, then if needed for performance.

# Resources

- https://github.com/xuwd1/rdtsc-notes
- 

# Tasks

- X Use arenas (pool, stack, linear). Try to build the game with only those.
- X Use SDL3, not 2.
- X Render a black red window from scratch. (using my own framebuffer)
- Study what you just did. Write about it on your website.
- Prepare hot reloading code. Investigate how it works, that is to say, if I save a file, is there a way for files to recompile automatically and see changes instantly?
- Study what you just did. Write about it on your website.
- Is there a problem in relying on VSYNC? Investigate. If yes, try to handle the time spent in a frame manually.
- Use double buffers to avoid tearing!
- Prepare function to draw lines (projectiles), squares (idk yet), circles (plasma bombs), triangles (entities and everything else). Study them.
- Leverage AVX2 or investigate how to use it. Also intrinsics or ASM.
- Use a fixed time step for physics, not variable. Render as fast as possible, but interpolate.
