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

u32 shader_create_program(char vertex_path[], char fragment_path[]) {
  u32 create_shader(GLenum type, char path[], char name[]) {
    FILE* file = fopen(path, "r");
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);
    char shader_source[size];
    fread(shader_source, sizeof(char), size - 1, file);
    shader_source[size - 1] = '\0';
    fclose(file);

    u32 shader = glCreateShader(type);
    glShaderSource(shader, 1, (const char * const *) &(const char *) { shader_source }, NULL);
    glCompileShader(shader);
    return shader;
  }

  u32 vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_path, "vertex");
  u32 fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_path, "fragment");
  u32 shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  glUseProgram(shader_program);
  return shader_program;
}
