#version 460 core
layout (location = 0) in vec3 texCoord;

layout (location = 0) out vec4 FragColor;

layout (set = 0, binding = 0) uniform samplerCube tex;

layout (std140, set = 1, binding = 0) uniform Matrices {
  mat4 view;
  mat4 projection;
  vec4 lightPos;
  vec4 lightColor;
  float fogDensity;
};

float toSRGB(float x) {
if (x <= 0.0031308)
        return 12.92 * x;
    else
        return 1.055 * pow(x, (1.0/2.4)) - 0.055;
}
vec3 sRGB(vec3 c) {
    return vec3(toSRGB(c.x), toSRGB(c.y), toSRGB(c.z));
}

void main() {
  /* scale up fog density to hide skybox */
  float boxFogDensity = 200.0 * fogDensity;

  float fogDistance = gl_FragCoord.z / gl_FragCoord.w;
  float fogAmount = 1.0 - clamp(exp(-pow(boxFogDensity * fogDistance, 2.0)), 0.0, 1.0);
  vec4 fogColor = 0.25 * vec4(vec3(lightColor), 1.0);

  FragColor = mix(texture(tex, texCoord) * vec4(vec3(lightColor), 1.0), fogColor, fogAmount);
  FragColor.rgb = sRGB(FragColor.rgb);
}
