# version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in float aLig;
uniform mat4 MODEL;
uniform mat4 VIEW;
uniform mat4 PROJ;
out float _lig;

void main() {
  _lig = aLig;
  gl_Position = PROJ * VIEW * MODEL * vec4(aPos, 1);
}
