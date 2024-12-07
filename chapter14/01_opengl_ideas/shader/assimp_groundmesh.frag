#version 460 core
layout (location = 0) in vec4 color;

out vec4 FragColor;

void main() {
  FragColor = color;
}
