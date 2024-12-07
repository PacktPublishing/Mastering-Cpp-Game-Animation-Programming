#version 460 core
layout (location = 0) in vec3 texCoord;

layout (location = 0) out vec4 FragColor;

uniform samplerCube tex;

layout (std140, binding = 0) uniform Matrices {
  mat4 view;
  mat4 projection;
  vec4 lightPos;
  vec4 lightColor;
  float fogDensity;
};

void main() {
  float boxFogDensity = 200.0 * fogDensity;
  float fogDistance = gl_FragCoord.z / gl_FragCoord.w;
  float fogAmount = 1.0 - clamp(exp(-pow(boxFogDensity * fogDistance, 2.0)), 0.0, 1.0);
  vec4 fogColor = 0.25 * vec4(vec3(lightColor), 1.0);

  FragColor = mix(texture(tex, texCoord) * vec4(vec3(lightColor), 1.0), fogColor, fogAmount);
}
