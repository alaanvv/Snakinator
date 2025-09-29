#include "canvas.h"
#include <time.h>

#define UPSCALE 0.3

#define TICK_WAIT 0.4
#define START_SIZE 3
#define MAX_TILES 25
#define TILES 10

u32 shader, hud_shader;

CanvasConfig config = {
  .title = "SNAKINATOR",
  .fullscreen = 1,
  .screen_size = 1,
  .capture_mouse = 1,
  .clear_color = PASTEL_PURPLE
};

Camera cam = {
  .fov = PI4,
  .far_plane = 100,
  .near_plane = 0.01,
  .pos = { TILES * 0.6, TILES * 1.5, TILES * 1.3 }
};

// ---

enum { UP, RIGHT, DOWN, LEFT, FRONT, BACK } Direction;

typedef struct {
  i8 body[MAX_TILES * MAX_TILES * 2][3];
  u8 size;
  u8 dir;
  u8 last_dir;
  u8 last_plane_dir;
} Snake;

void key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods);
void draw_shadow(Model* mo_shadow, u8 x, u8 y);
f32 wave(f32 freq, f32 intensity, f32 delay);
void cursor_callback(Camera* cam);
void randomize_apple();
void lookat_center();
void game_loop();

// ---

Snake snake = { { { TILES / 2, 0, TILES / 2 }, { TILES / 2 + 1, 0, TILES / 2 }, { TILES / 2 + 2, 0, TILES / 2 } }, 3, RIGHT, RIGHT, RIGHT };
u8 apple[3];

vec3 center = { TILES / 2.0, 0.5, TILES / 2.0 };
f32 tick, last_tick, tick_wait = TICK_WAIT;
u8 menu = 1, game_end = 0, tiles = TILES;
f32 target_fov = PI4;
vec3 target_pos = { TILES * 0.6, TILES * 1.5, TILES * 1.3 };

// ---

i32 main() {
  canvas_init(&cam, config);
  glfwSetKeyCallback(cam.window, key_callback);

  init_audio_engine((char*[16]) { "apple", "move", "hit", "death", "start", "song" }, 6);
  set_volume("move",  0.07);
  set_volume("death", 0.05);
  set_volume("hit",   0.05);

  Material ma_floor   = { YELLOW,            .lig = 1 };
  Material ma_shadow  = { { 0.7, 0.7, 0.2 }, .lig = 1 };
  Material ma_snake   = { DEEP_PURPLE,       .lig = 1 };
  Material ma_apple   = { DEEP_RED,          .lig = 1 };
  Material ma_apple_h = { DEEP_RED,          .lig = 1, .tex = GL_TEXTURE1 };

  Model* mo_floor   = model_create("cube", &ma_floor,  1);
  Model* mo_shadow  = model_create("cube", &ma_shadow, 1);
  Model* mo_snake   = model_create("cube", &ma_snake,  1);
  Model* mo_apple   = model_create("cube", &ma_apple,  1);
  Model* mo_apple_h = model_create("cube", &ma_apple_h, 1);

  Font font = { GL_TEXTURE0, 30, 5, 7.0 / 5 };

  canvas_create_texture(GL_TEXTURE0, "font",   TEXTURE_DEFAULT);
  canvas_create_texture(GL_TEXTURE1, "hidden", TEXTURE_DEFAULT);

  shader = shader_create_program("obj");
  generate_proj_mat(&cam, shader);
  lookat_center();

  hud_shader = shader_create_program("hud");
  generate_ortho_mat(&cam, hud_shader);

  // FBO
  u32 lowres_fbo = canvas_create_FBO(cam.width * UPSCALE, cam.height * UPSCALE, GL_NEAREST, GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // ---

  srand(time(0));
  randomize_apple();
  play_audio_loop("song");

  while (!glfwWindowShouldClose(cam.window)) {
    tick = glfwGetTime();
    if (tick - last_tick > tick_wait) {
      last_tick = tick;
      game_loop();
    }

    cam.pos[0] -= sin(glfwGetTime() * PI / 6) * 0.003;
    cam.pos[1] -= sin(glfwGetTime() * PI / 4) * 0.020;
    cam.pos[2] += sin(glfwGetTime() * PI / 9) * 0.007;

    if (center[0]  < tiles / 2.0)   center[0] += 0.001;
    if (center[2]  < tiles / 2.0)   center[2] += 0.001;
    if (cam.fov    < target_fov)    cam.fov   += 0.001;
    if (cam.pos[0] < target_pos[0]) cam.pos[0] += 0.01;
    if (cam.pos[1] < target_pos[1]) cam.pos[1] += 0.01;
    if (cam.pos[2] < target_pos[2]) cam.pos[2] += 0.01;

    if (!menu) {
      lookat_center();

      // 3D Drawing
      glUseProgram(shader);

      // Floor
      model_bind(mo_floor, shader);
      glm_scale(mo_floor->model, (vec3) { tiles, 1, tiles });
      model_draw(mo_floor, shader);

      // Apple
      model_bind(mo_apple, shader);
      glm_translate(mo_apple->model, (vec3) { apple[0], apple[1] + 1, apple[2] });
      model_draw(mo_apple, shader);

      if (apple[1]) draw_shadow(mo_shadow, apple[0], apple[2]);

      // Snake
      for (u8 i = 0; i < snake.size; i++) {
        model_bind(mo_snake, shader);
        canvas_uni3f(shader, "MAT.COL", ma_snake.col[0] - (snake.size - i) * 0.003, ma_snake.col[1] - (snake.size - i) * 0.003, ma_snake.col[2] - (snake.size - i) * 0.003);
        glm_translate(mo_snake->model, (vec3) { snake.body[i][0], snake.body[i][1] + 1, snake.body[i][2] });
        model_draw(mo_snake, shader);

        if (snake.body[i][1]) draw_shadow(mo_shadow, snake.body[i][0], snake.body[i][2]);
      }

      // Apple Outline
      glDisable(GL_DEPTH_TEST);
      model_bind(mo_apple_h, shader);
      glm_mat4_copy(mo_apple->model, mo_apple_h->model);
      model_draw(mo_apple_h, shader);
      glEnable(GL_DEPTH_TEST);
    }

    // Lowres
    glBlitNamedFramebuffer(0, lowres_fbo, 0, 0, cam.width, cam.height, 0, 0, cam.width * UPSCALE, cam.height * UPSCALE, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBlitNamedFramebuffer(lowres_fbo, 0, 0, 0, cam.width * UPSCALE, cam.height * UPSCALE, 0, 0, cam.width, cam.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // HUD Drawing
    glUseProgram(hud_shader);

    // Draw Text
    if (menu) {
      hud_draw_text(hud_shader, "snakinator", cam.width / 2.0 - canvas_text_width("snakinator", font, 1) / 2.0 + wave(2, 15, 0.00), cam.height / 2.0 - font.size / 2.0 + wave(1, 12, 0.00), font, (vec3) PASTEL_GREEN);
      hud_draw_text(hud_shader, "snakinator", cam.width / 2.0 - canvas_text_width("snakinator", font, 1) / 2.0 + wave(2, 15, 5.00), cam.height / 2.0 - font.size / 2.0 + wave(1, 12, 5.00), font, (vec3) DEEP_GREEN);
    }
    else {
      char buffer[16];
      sprintf(buffer, "%d", snake.size);
      hud_draw_text(hud_shader, buffer, 20 + wave(8, 6, 0.00), 20 + wave(9, 4, 0.00), font, (vec3) DEEP_PURPLE);
      hud_draw_text(hud_shader, buffer, 20 + wave(8, 6, 0.04), 20 + wave(9, 4, 0.00), font, (vec3) DEEP_PURPLE);
    }

    // Finish
    glUseProgram(shader);
    glfwSwapBuffers(cam.window);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwPollEvents();
    cursor_callback(&cam);
  }

  glfwTerminate();
  return 0;
}

f32 wave(f32 freq, f32 intensity, f32 delay) {
  return sin((glfwGetTime() - delay) * freq) * intensity;
}

void draw_shadow(Model* mo_shadow, u8 x, u8 y) {
  model_bind(mo_shadow, shader);
  glm_translate(mo_shadow->model, (vec3) { x, 1.01, y });
  glm_scale(mo_shadow->model, (vec3) { 1, 0, 1 });
  model_draw(mo_shadow, shader);
}

void lookat_center() {
  glm_lookat(cam.pos, center, (vec3) { 0, 1, 0 }, cam.view);
  glUniformMatrix4fv(UNI(shader, "VIEW"), 1, GL_FALSE, (const f32 *) { cam.view[0] });
}

void key_callback(GLFWwindow *window, i32 key, i32 scancode, i32 action, i32 mods) {
  if (action != GLFW_PRESS) return;

  if (menu && key != GLFW_KEY_ESCAPE) {
    menu = 0;
    play_audio("start");
  }

  switch (key) {
    case GLFW_KEY_ESCAPE: 
      if (menu) glfwSetWindowShouldClose(window, 1); 
      else menu = 1; 
      return;

    case GLFW_KEY_S: if (snake.last_dir !=  DOWN) { snake.dir =    UP; last_tick -= 0.5; } break;
    case GLFW_KEY_W: if (snake.last_dir !=    UP) { snake.dir =  DOWN; last_tick -= 0.5; } break;
    case GLFW_KEY_D: if (snake.last_dir !=  LEFT) { snake.dir = RIGHT; last_tick -= 0.5; } break;
    case GLFW_KEY_A: if (snake.last_dir != RIGHT) { snake.dir =  LEFT; last_tick -= 0.5; } break;

    case GLFW_KEY_E:
      if      (snake.body[snake.size - 1][1] == 0 && snake.last_dir !=  BACK) { snake.dir = FRONT; last_tick -= 0.5; }
      else if (snake.body[snake.size - 1][1] == 1 && snake.last_dir != FRONT) { snake.dir =  BACK; last_tick -= 0.5; }
      break;

    default: return;
  }

  
}

void cursor_callback(Camera* cam) {
  static vec2 mouse;
  f64 x, y;
  glfwGetCursorPos(cam->window, &x, &y);

  if (!mouse[0]) { mouse[0] = x; mouse[1] = y; }
  if (x == mouse[0] && y == mouse[1]) return;

  cam->pos[0] -= 0.010 * (x - mouse[0]) * ((f32) tiles / TILES);
  cam->pos[1] -= 0.001 * (y - mouse[1]) * ((f32) tiles / TILES);
  cam->pos[2] += 0.010 * (y - mouse[1]) * ((f32) tiles / TILES);

  mouse[0] = x;
  mouse[1] = y;
}

// ---

void randomize_apple() {
  apple[0] = RAND(0, tiles);
  apple[1] = RAND(0, 2);
  apple[2] = RAND(0, tiles);
 
  for (u8 i = 0; i < snake.size; i++)
    if (VEC3_COMPARE(snake.body[i], apple)) 
      randomize_apple();
}

void game_loop() {
  if (menu) return;

  if (game_end) {
    if (!snake.size && tick_wait == TICK_WAIT * 5) {
      for (u8 i = 0; i < START_SIZE; i++) {
        snake.body[i][0] = snake.body[game_end - 3 + i][0];
        snake.body[i][1] = snake.body[game_end - 3 + i][1];
        snake.body[i][2] = snake.body[game_end - 3 + i][2];
      }

      tick_wait = TICK_WAIT;
      snake.size = START_SIZE;
      game_end = 0;
      play_audio("start");
    }
    else if (!snake.size) tick_wait = TICK_WAIT * 5;
    else {
      snake.size--;
      tick_wait *= 0.95;
      stop_audio("hit");
      play_audio("hit");
    }
    return;
  }

  // If going to hit a border on y-axis, get back to a plane direction
  if (snake.dir == FRONT && snake.body[snake.size - 1][1] == 1 || snake.dir == BACK && snake.body[snake.size - 1][1] == 0)
    snake.dir = snake.last_plane_dir;

  // Create the new head outbound before shifting the snake
  snake.body[snake.size][0] = snake.body[snake.size - 1][0];
  snake.body[snake.size][1] = snake.body[snake.size - 1][1];
  snake.body[snake.size][2] = snake.body[snake.size - 1][2];

  switch (snake.dir) {
    case UP:    snake.body[snake.size][2]++; break;
    case DOWN:  snake.body[snake.size][2]--; break;
    case RIGHT: snake.body[snake.size][0]++; break;
    case LEFT:  snake.body[snake.size][0]--; break;
    case FRONT: snake.body[snake.size][1]++; break;
    case BACK:  snake.body[snake.size][1]--; break;
  }

  // Teleport through walls
  if (snake.body[snake.size][0] >= tiles) snake.body[snake.size][0] = 0;
  if (snake.body[snake.size][2] >= tiles) snake.body[snake.size][2] = 0;
  if (snake.body[snake.size][0] < 0) snake.body[snake.size][0] = tiles - 1;
  if (snake.body[snake.size][2] < 0) snake.body[snake.size][2] = tiles - 1;

  // Check for snake collision
  for (u8 i = 1; i < snake.size; i++)
    if (VEC3_COMPARE(snake.body[i], snake.body[snake.size])) {
      tick_wait = TICK_WAIT * 0.25;
      game_end = snake.size;
      play_audio("death");
    }

  // Check for apple collision
  if (VEC3_COMPARE(snake.body[snake.size], apple)) {
    snake.size++;
    if (snake.size > tiles * tiles / 3) {
      tiles += 1;
      target_fov += PI4 / 20;
      VEC3_COPY(VEC3(tiles * 0.6, tiles * 1.7, tiles * 1.6), target_pos);
    }

    play_audio("apple");
    randomize_apple();
  }
  // If didn't eat apple, remove last block
  else
    for (u8 i = 1; i < snake.size + 1; i++) 
      VEC3_COPY(snake.body[i], snake.body[i - 1]);

  play_audio("move");
  snake.last_dir = snake.dir;
  if (snake.dir != FRONT && snake.dir != BACK) snake.last_plane_dir = snake.dir;
  return;
}
