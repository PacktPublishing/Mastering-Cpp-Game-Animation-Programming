#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

layout (location = 0) out vec4 color;

layout (std140, set = 0, binding = 0) uniform Matrices {
  mat4 view;
  mat4 projection;
};

void main() {
  gl_Position = projection * view * vec4(aPos.x, aPos.y, aPos.z, 1.0);
  color = vec4(aColor, 0.3);
}
