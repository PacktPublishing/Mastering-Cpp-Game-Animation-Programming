#version 460 core
layout (location = 0) in vec4 aPos;

layout (location = 0) out vec3 texCoord;

layout (std140, set = 1, binding = 0) uniform Matrices {
  mat4 view;
  mat4 projection;
  vec4 lightPos;
  vec4 lightColor;
  float fogDensity;
};

void main() {
  mat4 inverseProjection = inverse(projection);

  /* remove translation part */
  mat3 inverseView = transpose(mat3(view));

  texCoord = inverseView * (inverseProjection * aPos).xyz;

  /* set z to 1.0 to force drawing on the far z plane */
  gl_Position = aPos.xyww;
}
