#include <SDL.h>

static const int WIDTH = 800;
static const int HEIGHT = 600;

int main(void) {
  SDL_Init(SDL_INIT_EVERYTHING);

  SDL_Window *window =
    SDL_CreateWindow("Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                     WIDTH, HEIGHT, 0);

  SDL_Surface *window_surface = SDL_GetWindowSurface(window);

  SDL_Surface *canvas = SDL_CreateRGBSurfaceWithFormat(
    0, WIDTH, HEIGHT, 32, SDL_PIXELFORMAT_RGBA8888);

  Uint32 *buffer = (Uint32 *)canvas->pixels;

  SDL_LockSurface(canvas);
  for (int row = 0; row < HEIGHT; row++) {
    for (int column = 0; column < WIDTH; column++) {
      int offset = row * WIDTH + column;
      buffer[offset] = SDL_MapRGBA(canvas->format, 255, 0, 0, 255);
    }
  }
  SDL_UnlockSurface(canvas);

  int quit = 0;
  SDL_Event event;
  while (!quit) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        quit = 1;
      }
    }

    SDL_BlitSurface(canvas, 0, window_surface, 0);
    SDL_UpdateWindowSurface(window);

    SDL_Delay(10);
  }

  SDL_Quit();

  return 0;
}
