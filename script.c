#include <SDL2/SDL.h>
#include <stdio.h>
#include <time.h>

#define RAND(min, max) (random() % (max - min) + min)
#define FILE_NAME "me.ppm"
#define R_MUL 1
#define G_MUL 1
#define B_MUL 1
#define INVERT 0
#define V_MIRROR 0
#define H_MIRROR 0
#define RANDOMIZE_X 0
#define RANDOMIZE_Y 1
#define RANDOM_OFFSET 50
#define ROTATE 1
#define CLEAR 0

// ---

SDL_Window* win;
SDL_Renderer* ren;

FILE* img;
int w, h;
int buffer[3]; 
float angle;

// ---

int main() {
  srand(time(0));

  img = fopen(FILE_NAME, "r");
  fscanf(img, "%*s %d %d %*s", &w, &h);

  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(w, h, 0, &win, &ren);
  
  SDL_Event e;
  int running = 1;
  while (running) {
    while(SDL_PollEvent(&e)) { if (e.type == SDL_QUIT) running = 0; }
    if (CLEAR) { SDL_RenderClear(ren); }

    for (int i = 0; i < w * h; i++) {
      fscanf(img, "%d %d %d", &buffer[0], &buffer[1], &buffer[2]);

      int x = i % w;
      int y = i / w;

      if (V_MIRROR) x = w - x;
      if (H_MIRROR) y = h - y;

      if (RANDOMIZE_X) x += RAND(0, RANDOM_OFFSET);
      if (RANDOMIZE_Y) y += RAND(0, RANDOM_OFFSET);

      int new_w = w * cos(angle);
      x = new_w * (float) x / w;

      int new_h = h + (h * 0.5 * (new_w / 2 - x) / (new_w / 2) * sin(angle));
      y = ((h - new_h) / 2) + new_h * (float) y / h;

      int r = buffer[0] * R_MUL;
      int g = buffer[1] * G_MUL;
      int b = buffer[2] * B_MUL;
      if (INVERT) {
        r = 255 - r;
        g = 255 - g;
        b = 255 - b;
      }

      SDL_SetRenderDrawColor(ren, r, g, b, 255);
      SDL_RenderDrawPoint(ren, ((w - new_w) / 2) + x, y);
    }

    SDL_RenderPresent(ren);
    if (!ROTATE) break;

    fclose(img);  
    img = fopen(FILE_NAME, "r"); 
    fscanf(img, "%*s %*d %*d %*s");

    angle += 3.14159 / 1000;
  }

  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
  fclose(img);
  return 0;
}
