#include "canvas.h"
#include <time.h>

#define UPSCALE 0.3
#define TILES 8

u32 shader, hud_shader;

CanvasConfig config = {
  .title = "SNAKINATOR",
  .capture_mouse = 1,
  .fullscreen = 1,
  .screen_size = 1,
  .clear_color = PASTEL_PURPLE
};

Camera cam = {
  .fov = PI4,
  .near_plane = 0.01,
  .far_plane = 100,
  .pos = { TILES * 0.6, TILES * 1.7, TILES * 1.3 },
};

// ---

enum { UP, RIGHT, DOWN, LEFT, FRONT, BACK } Direction;

typedef struct {
  i8 body[TILES * TILES * 2][3];
  u8 size;
  u8 dir;
  u8 last_dir;
  u8 last_plane_dir;
} Snake;

void key_callback(GLFWwindow *window, i32 key, i32 scancode, i32 action, i32 mods);
void cursor_callback(Camera* cam);
void game_loop();

// ---

u8 apple[3] = { TILES / 4, 0, TILES / 4 };
Snake snake = { { TILES / 2 + 2, 0, TILES / 2, TILES / 2 + 1, 0, TILES / 2, TILES / 2, 0, TILES / 2 }, 3, LEFT, LEFT, LEFT };

f32 tick, last_tick, tick_wait = 0.4;
u8 menu = 1, game_end = 0;

// ---

i32 main() {
  srand(time(0));
  canvas_init(&cam, config);
  init_audio_engine((char*[32]) { "apple", "move", "hit", "death", "start", "song" }, 6);
  set_volume("move", 0.07);
  set_volume("death", 0.05);
  set_volume("hit", 0.05);

  Material m_snake   = { DEEP_PURPLE,       .lig = 1 };
  Material m_apple   = { DEEP_RED,          .lig = 1 };
  Material m_floor   = { YELLOW,            .lig = 1 };
  Material m_shadow  = { { 0.7, 0.7, 0.2 }, .lig = 1 };
  Material m_text    = { DEEP_GREEN,        .lig = 1 };
  Material m_apple_h = { DEEP_RED,          .lig = 1, .tex = GL_TEXTURE1 };

  Model* mo_snake   = model_create("cube", &m_snake,  1);
  Model* mo_floor   = model_create("cube", &m_floor,  1);
  Model* mo_apple   = model_create("cube", &m_apple,  1);
  Model* mo_shadow  = model_create("cube", &m_shadow, 1);
  Model* mo_apple_h = model_create("cube", &m_apple_h, 1);

  canvas_create_texture(GL_TEXTURE0, "font",   TEXTURE_DEFAULT);
  canvas_create_texture(GL_TEXTURE1, "hidden", TEXTURE_DEFAULT);

  Font font = { GL_TEXTURE0, 30, 5, 7.0 / 5 };

  shader = shader_create_program("obj");
  VEC3_COPY(VEC3((f32) TILES / 2, 0.5, (f32) TILES / 2), cam.dir);
  generate_proj_mat(&cam, shader);
  glm_lookat(cam.pos, cam.dir, (vec3) { 0, 1, 0 }, cam.view);
  glUniformMatrix4fv(UNI(shader, "VIEW"), 1, GL_FALSE, (const f32 *) { cam.view[0] });

  hud_shader = shader_create_program("hud");
  generate_ortho_mat(&cam, hud_shader);

  glfwSetKeyCallback(cam.window, key_callback);

  // FBO
  u32 lowres_fbo = canvas_create_FBO(cam.width * UPSCALE, cam.height * UPSCALE, GL_NEAREST, GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // ---

  play_audio_loop("song");

  while (!glfwWindowShouldClose(cam.window)) {
    tick = glfwGetTime();
    if (tick - last_tick > tick_wait) {
      last_tick = tick;
      game_loop();
    }

    cam.pos[0] -= sin(glfwGetTime() * PI / 6) * 0.003;
    cam.pos[1] -= sin(glfwGetTime() * PI / 4) * 0.040;
    cam.pos[2] += sin(glfwGetTime() * PI / 9) * 0.007;

    if (!menu) {
      glm_lookat(cam.pos, cam.dir, (vec3) { 0, 1, 0 }, cam.view);
      glUniformMatrix4fv(UNI(shader, "VIEW"), 1, GL_FALSE, (const f32 *) { cam.view[0] });

      // 3D Drawing
      glUseProgram(shader);

      // Floor
      model_bind(mo_floor, shader);
      glm_scale(mo_floor->model, (vec3) { TILES, 1, TILES });
      model_draw(mo_floor, shader);

      // Apple
      model_bind(mo_apple, shader);
      glm_translate(mo_apple->model, (vec3) { apple[0], apple[1] + 1, apple[2] });
      model_draw(mo_apple, shader);

      if (apple[1]) {
        model_bind(mo_shadow, shader);
        glm_translate(mo_shadow->model, (vec3) { apple[0], 1.01, apple[2] });
        glm_scale(mo_shadow->model, (vec3) { 1, 0, 1 });
        model_draw(mo_shadow, shader);
      }

      // Snake
      for (u8 i = 0; i < snake.size; i++) {
        model_bind(mo_snake, shader);
        canvas_uni3f(shader, "MAT.COL", m_snake.col[0] - (snake.size - i) * 0.003, m_snake.col[1] - (snake.size - i) * 0.003, m_snake.col[2] - (snake.size - i) * 0.003);
        glm_translate(mo_snake->model, (vec3) { snake.body[i][0], snake.body[i][1] + 1, snake.body[i][2] });
        model_draw(mo_snake, shader);

        if (snake.body[i][1]) {
          model_bind(mo_shadow, shader);
          glm_translate(mo_shadow->model, (vec3) { snake.body[i][0], 1.01, snake.body[i][2] });
          glm_scale(mo_shadow->model, (vec3) { 1, 0.001, 1 });
          model_draw(mo_shadow, shader);
        }
      }

      glDisable(GL_DEPTH_TEST);
      model_bind(mo_apple_h, shader);
      glm_translate(mo_apple_h->model, (vec3) { apple[0], apple[1] + 1, apple[2] });
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
      hud_draw_text(hud_shader, "snakinator", cam.width / 2.0 - canvas_text_width("snakinator", font, 1) / 2.0 + sin((glfwGetTime() - 0.00) * 8) * 15, cam.height / 2.0 - font.size / 2.0 + sin((glfwGetTime() - 0.00) * 9) * 12, font, (vec3) PASTEL_GREEN);
      hud_draw_text(hud_shader, "snakinator", cam.width / 2.0 - canvas_text_width("snakinator", font, 1) / 2.0 + sin((glfwGetTime() - 0.06) * 8) * 15, cam.height / 2.0 - font.size / 2.0 + sin((glfwGetTime() - 0.06) * 9) * 12, font, (vec3) DEEP_GREEN);
    }
    else {
      char buffer[16];
      sprintf(buffer, "%d/%d", snake.size, TILES * TILES * 2);
      hud_draw_text(hud_shader, buffer, 18 + sin((glfwGetTime() - 0.00) * 8) * 6, 18 + sin((glfwGetTime() - 0.00) * 9) * 4, font, (vec3) DEEP_PURPLE);
      hud_draw_text(hud_shader, buffer, 18 + sin((glfwGetTime() - 0.04) * 8) * 6, 18 + sin((glfwGetTime() - 0.04) * 9) * 4, font, (vec3) DEEP_PURPLE);
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

void key_callback(GLFWwindow *window, i32 key, i32 scancode, i32 action, i32 mods) {
  if (action != GLFW_PRESS) return;

  switch (key) {
    case GLFW_KEY_ESCAPE: 
      if (menu) glfwSetWindowShouldClose(window, 1); 
      else menu = 1; 
      return;
    case GLFW_KEY_ENTER: 
      if (menu) {
        menu = 0;
        play_audio("start");
      }
      return;

    case GLFW_KEY_S: if (snake.last_dir !=  DOWN) snake.dir =    UP; break;
    case GLFW_KEY_W: if (snake.last_dir !=    UP) snake.dir =  DOWN; break;
    case GLFW_KEY_D: if (snake.last_dir !=  LEFT) snake.dir = RIGHT; break;
    case GLFW_KEY_A: if (snake.last_dir != RIGHT) snake.dir =  LEFT; break;

    case GLFW_KEY_E:
      if      (snake.body[snake.size - 1][1] == 0 && snake.last_dir !=  BACK) snake.dir = FRONT;
      else if (snake.body[snake.size - 1][1] == 1 && snake.last_dir != FRONT) snake.dir =  BACK;
      break;

    default: return;
  }

  last_tick -= 0.5;
}

void cursor_callback(Camera* cam) {
  static vec2 mouse;
  f64 x, y;
  glfwGetCursorPos(cam->window, &x, &y);

  if (!mouse[0]) { mouse[0] = x; mouse[1] = y; }
  if (x == mouse[0] && y == mouse[1]) return;

  cam->pos[0] -= 0.0010 * (x - mouse[0]);
  cam->pos[1] -= 0.0001 * (y - mouse[1]);
  cam->pos[2] += 0.0005 * (y - mouse[1]);

  mouse[0] = x;
  mouse[1] = y;
}

// ---

u8 apple_free() {
  for (u8 i = 0; i < snake.size; i++)
    if (VEC3_COMPARE(snake.body[i], apple))
      return 0;
  return 1;
}

void game_loop() {
  if (menu) return;
  if (game_end) {
    if (snake.size == 0 && tick_wait == 2) {
      tick_wait = 0.4;
      snake.size = 3;
      game_end = 0;
      play_audio("start");
    }
    else if (snake.size == 0) tick_wait = 2;
    else {
      snake.size--;
      tick_wait *= 0.98;
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
  if (snake.body[snake.size][0] >= TILES) snake.body[snake.size][0] = 0;
  if (snake.body[snake.size][2] >= TILES) snake.body[snake.size][2] = 0;
  if (snake.body[snake.size][0] < 0) snake.body[snake.size][0] = TILES - 1;
  if (snake.body[snake.size][2] < 0) snake.body[snake.size][2] = TILES - 1;

  // Check for snake collision
  for (u8 i = 1; i < snake.size; i++)
    if (VEC3_COMPARE(snake.body[i], snake.body[snake.size])) {
      game_end = 1;
      play_audio("death");
      tick_wait = 0.1;
    }

  // Check for apple collision
  if (VEC3_COMPARE(snake.body[snake.size], apple)) {
    snake.size++;
    play_audio("apple");

    do {
      apple[0] = RAND(0, TILES);
      apple[1] = RAND(0, 2);
      apple[2] = RAND(0, TILES);
    } while (!apple_free());
  }
  // If didn't eat apple, remove last block
  else {
    for (u8 i = 1; i < snake.size + 1; i++) {
      snake.body[i - 1][0] = snake.body[i][0];
      snake.body[i - 1][1] = snake.body[i][1];
      snake.body[i - 1][2] = snake.body[i][2];
    }
  }

  play_audio("move");

  snake.last_dir = snake.dir;
  if (snake.dir != FRONT && snake.dir != BACK) snake.last_plane_dir = snake.dir;
  return;
}
