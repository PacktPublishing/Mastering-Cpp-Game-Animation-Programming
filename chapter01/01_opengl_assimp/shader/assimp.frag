#version 460 core
layout (location = 0) in vec4 color;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

layout (location = 0) out vec4 FragColor;

uniform sampler2D tex;

vec3 lightPos = vec3(4.0, 3.0, 6.0);
vec3 lightColor = vec3(1.0, 1.0, 1.0);

void main() {
  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * max(vec3(lightColor), vec3(0.05, 0.05, 0.05));

  vec3 norm = normalize(vec3(normal));
  vec3 lightDir = normalize(vec3(lightPos));

  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * vec3(lightColor);

  FragColor = vec4(ambient + diffuse, 1.0) * texture(tex, texCoord) * color;
}
