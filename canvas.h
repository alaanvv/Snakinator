#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <stdio.h>

typedef uint32_t u32;
typedef int16_t  i16;
typedef int32_t  i32;
typedef float    f32;

// --- Type

typedef struct {
  vec3 pos;
  f32  lig;
} Vertice;

typedef Vertice Mesh[];

typedef struct {
  i16 width, height;
  f32 fov, near, far;
  vec3 pos, dir;
} Camera;

typedef struct {
  GLFWwindow* window;
  i16 width, height;
} Canvas;

// --- Function

void generate_proj_mat(Camera cam, mat4 to) {
  glm_mat4_identity(to);
  glm_perspective(cam.fov, (f32) cam.width / cam.height, cam.near, cam.far, to);
}

void generate_view_mat(Camera cam, mat4 to) {
  glm_lookat(cam.pos, cam.dir, (vec3) { 0, 1, 0 }, to);
}

void canvas_init(Canvas* canvas, char title[]) {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  canvas->window = glfwCreateWindow(canvas->width, canvas->height, title, NULL, NULL);
  glfwMakeContextCurrent(canvas->window);

  gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
  glViewport(0, 0, canvas->width, canvas->height);
  glEnable(GL_DEPTH_TEST);
}

u32 canvas_create_VBO() {
  u32 VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  return VBO;
}

u32 canvas_create_VAO() {
  u32 VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  return VAO;
}

u32 shader_create_program(const char* vertex_shader_source, const char* fragment_shader_source) {
  u32 v_shader = glCreateShader(GL_VERTEX_SHADER);
  u32 f_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(v_shader, 1, &vertex_shader_source, NULL);
  glShaderSource(f_shader, 1, &fragment_shader_source, NULL);
  glCompileShader(v_shader);
  glCompileShader(f_shader);

  u32 shader_program = glCreateProgram();
  glAttachShader(shader_program, v_shader);
  glAttachShader(shader_program, f_shader);
  glLinkProgram(shader_program);
  glDeleteShader(v_shader);
  glDeleteShader(f_shader);

  glUseProgram(shader_program);
  return shader_program;
}
