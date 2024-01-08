#include "canvas.h"
#include <time.h>

#define RAND(min, max) (rand() % (max - min) + min)
#define UNI(shader, name) (glGetUniformLocation(shader, name))
#define CMP_VEC3(v1, v2) (v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2])
#define PI 3.14159

#define WIDTH  800
#define HEIGHT 800
#define TILES 10
#define C_BACKGROUND { 0.11, 0.16, 0.32 }
#define C_PLATAFORM  { 0.98, 0.93, 0.35 }
#define C_APPLE      { 1.00, 0.00, 0.30 }
#define C_SNAKE      { 0.49, 0.14, 0.32 }

typedef uint8_t  u8;
typedef uint32_t u32;
typedef int8_t   i8;
typedef int32_t  i32;
typedef float    f32;
typedef double   f64;

// --- Shader

const char* vertex_shader = "# version 330 core\nlayout (location = 0) in vec3 aPos;\nlayout (location = 1) in float aLig;\nuniform mat4 MODEL;\nuniform mat4 VIEW;\nuniform mat4 PROJ;\nout float _lig;\nvoid main() {\n  _lig = aLig;\n  gl_Position = PROJ * VIEW * MODEL * vec4(aPos, 1);\n}";
const char* fragment_shader = "# version 330 core\nuniform vec3 COLOR;\nin float _lig;\nout vec4 color;\nvoid main() {\n  float lig = 0.8 + _lig * 0.2;\n  color = vec4(COLOR * lig, 1);\n}";

// --- Type

enum { UP, RIGHT, DOWN, LEFT, FRONT, BACK } Direction;

typedef struct {
  i8 body[TILES * TILES * 2][3];
  u8 size;
  u8 dir;
  u8 last_dir;
  u8 last_plane_dir;
} Snake;

void key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods);
void cursor_callback(GLFWwindow* window, f64 x, f64 y);
u8 game_loop();

// --- Setup

#include "mesh.h" 

Canvas canvas = { NULL, WIDTH, HEIGHT };
Camera cam = { WIDTH, HEIGHT, PI / 4, 0.1, 100, { TILES * 0.6, TILES * 1.1, TILES * -0.9 }, { (f32) TILES / 2, 0.5, (f32) TILES / 2 } };
mat4 view, proj;

const f32 c_background[3] = C_BACKGROUND;
const f32 c_plataform[3] = C_PLATAFORM;
const f32 c_apple[3] = C_APPLE;
const f32 c_snake[3] = C_SNAKE;

u8 apple[3] = { TILES / 4, 0, TILES / 4 };
Snake snake = { { TILES / 2, 0, TILES / 2 }, 1, LEFT, LEFT, LEFT };

f32 last_tick;
f32 tick_wait = 0.3;
u8 game_end;

// --- Main

i8 main() {
  srand(time(0));

  canvas_init(&canvas, "Snakinator");
  glfwSetKeyCallback(canvas.window, key_callback);
  glfwSetCursorPosCallback(canvas.window, cursor_callback);

  u32 VBO = canvas_create_VBO();
  glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);
  u32 VAO = canvas_create_VAO();
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*) 0);
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*) (sizeof(f32) * 3));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  u32 shader_program = shader_create_program(vertex_shader, fragment_shader);

  mat4 model;
  generate_proj_mat(cam, proj);
  generate_view_mat(cam, view);

  while (!glfwWindowShouldClose(canvas.window)) {
    f32 tick = glfwGetTime();
    if (tick - last_tick > tick_wait) {
      last_tick = tick;
      if (game_loop() == 0) break;     
    }

    // Draw plataform
    glUniform3fv(UNI(shader_program, "COLOR"), 1, c_plataform);

    glm_mat4_identity(model);
    glm_scale(model, (vec3) { TILES, 1, TILES });
    glUniformMatrix4fv(UNI(shader_program, "MODEL"), 1, GL_FALSE, (const f32*) { model[0] });
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // Draw apple
    glUniform3fv(UNI(shader_program, "COLOR"), 1, c_apple);

    glm_mat4_identity(model);
    glm_translate(model, (vec3) { apple[0], apple[1] + 1, apple[2] });
    glUniformMatrix4fv(UNI(shader_program, "MODEL"), 1, GL_FALSE, (const f32*) { model[0] });
    glDrawArrays(GL_TRIANGLES, 0, 36);

    if (apple[1] == 1) {
      glUniform3fv(UNI(shader_program, "COLOR"), 1, c_plataform);

      glm_mat4_identity(model);
      glm_translate(model, (vec3) { apple[0], 1.01, apple[2] });
      glUniformMatrix4fv(UNI(shader_program, "MODEL"), 1, GL_FALSE, (const f32*) { model[0] });

      glDrawArrays(GL_TRIANGLES, 24, 6);
    }

    // Draw snake
    for (u8 i = 0; i < snake.size; i++) {
      glUniform3fv(UNI(shader_program, "COLOR"), 1, c_snake);
      glm_mat4_identity(model);
      glm_translate(model, (vec3) { snake.body[i][0], snake.body[i][1] + 1, snake.body[i][2] });
      glUniformMatrix4fv(UNI(shader_program, "MODEL"), 1, GL_FALSE, (const f32*) { model[0] });
      glDrawArrays(GL_TRIANGLES, 0, 36);

      if (snake.body[i][1] == 1) {
        glUniform3fv(UNI(shader_program, "COLOR"), 1, c_plataform);

        glm_mat4_identity(model);
        glm_translate(model, (vec3) { snake.body[i][0], 1.01, snake.body[i][2] });
        glUniformMatrix4fv(UNI(shader_program, "MODEL"), 1, GL_FALSE, (const f32*) { model[0] });

        glDrawArrays(GL_TRIANGLES, 24, 6);
      }
    }

    // Update frame
    glfwSwapBuffers(canvas.window); 
    glfwPollEvents();
    glClearColor(c_background[0], c_background[1], c_background[2], 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniformMatrix4fv(UNI(shader_program, "PROJ"),  1, GL_FALSE, (const f32*) { proj[0] });
    glUniformMatrix4fv(UNI(shader_program, "VIEW"),  1, GL_FALSE, (const f32*) { view[0] });
  }

  glfwTerminate();
  return 0;
}

void key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods) {
  if (action != GLFW_PRESS) return;

  switch (key) {
    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, 1); break;
    case GLFW_KEY_W: if (snake.last_dir != DOWN)  snake.dir = UP; break;
    case GLFW_KEY_S: if (snake.last_dir != UP)    snake.dir = DOWN; break;
    case GLFW_KEY_D: if (snake.last_dir != LEFT)  snake.dir = RIGHT; break;
    case GLFW_KEY_A: if (snake.last_dir != RIGHT) snake.dir = LEFT; break;
    case GLFW_KEY_E:
                       if      (snake.body[snake.size - 1][1] == 0 && (snake.size == 1 || snake.last_dir != BACK))  snake.dir = FRONT; 
                       else if (snake.body[snake.size - 1][1] == 1 && (snake.size == 1 || snake.last_dir != FRONT)) snake.dir = BACK; 
                       break;
  }
}

void cursor_callback(GLFWwindow* window, f64 x, f64 y) {
  cam.pos[0] = (TILES * 0.6) +  (TILES * 0.9 * ((x - WIDTH / 2) / (WIDTH / 2)));
  cam.pos[1] = (TILES * 1.1) +  (TILES * 0.9 * ((y - HEIGHT / 2) / (HEIGHT / 2)));
  cam.pos[2] = (-TILES * 0.9) + (TILES * 0.9 * ((y - HEIGHT / 2) / (HEIGHT / 2)));
  generate_view_mat(cam, view);
}

u8 game_loop() {
  if (game_end) {
    if (snake.size == 0 && tick_wait == 2) return 0;
    else if (snake.size == 0) tick_wait = 2;
    else snake.size--;
    return 1;
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
    case RIGHT: snake.body[snake.size][0]--; break;
    case LEFT:  snake.body[snake.size][0]++; break;
    case FRONT: snake.body[snake.size][1]++; break;
    case BACK:  snake.body[snake.size][1]--; break;
  }

  // Teleport through walls
  if (snake.body[snake.size][0] >= TILES) snake.body[snake.size][0] = 0;
  if (snake.body[snake.size][2] >= TILES) snake.body[snake.size][2] = 0;
  if (snake.body[snake.size][0] <  0)     snake.body[snake.size][0] = TILES - 1;
  if (snake.body[snake.size][2] <  0)     snake.body[snake.size][2] = TILES - 1;

  // Check for snake collision
  for (u8 i = 1; i < snake.size; i++)
    if (CMP_VEC3(snake.body[i], snake.body[snake.size])) {
      game_end = 1;
      tick_wait = 0.1;
    }

  // Check for apple collision
  if (CMP_VEC3(snake.body[snake.size], apple)) {
    u8 apple_free() {
      for (u8 i = 0; i < snake.size; i++)
        if (CMP_VEC3(snake.body[i], apple)) return 0;
    }

    do {
      apple[0] = RAND(0, TILES);
      apple[1] = RAND(0, 2);
      apple[2] = RAND(0, TILES);
    } while (!apple_free());

    snake.size++;
  }
  // If didn't eat apple, remove last block
  else {
    for (u8 i = 1; i < snake.size + 1; i++) {
      snake.body[i - 1][0] = snake.body[i][0];
      snake.body[i - 1][1] = snake.body[i][1];
      snake.body[i - 1][2] = snake.body[i][2];
    }
  }

  snake.last_dir = snake.dir;
  if (snake.dir != FRONT && snake.dir != BACK) snake.last_plane_dir = snake.dir;
  return 1;
}
