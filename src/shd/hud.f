# version 330 core

uniform sampler2D S_TEX;
uniform vec3 COL;

in  vec2 tex;
out vec4 color;

void main() {
  color = texture(S_TEX, tex);
  if (vec3(color) == vec3(0, 1, 0)) { discard; }
  color = color * vec4(COL, 1);
  gl_FragDepth = 0;
}
