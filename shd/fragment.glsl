# version 330 core

uniform vec3 COLOR;
in float _lig;
out vec4 color;

void main() {
  float lig = 0.8 + _lig * 0.2;
  color = vec4(COLOR * lig, 1);
}
