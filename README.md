# xcaliber

Game inspired by XCaliber from TempleOS.

# Dependencies

- SDL2

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
- Consistent code style.
- Comments explain why, not what.
- Optimise for clarity first, then if needed for performance.

# Resources

- https://github.com/xuwd1/rdtsc-notes

# Tasks

- Use arenas.
- Render a black red window from scratch.
- Use a fixed time step for physics, not variable. Render as fast as possible, but interpolate.
