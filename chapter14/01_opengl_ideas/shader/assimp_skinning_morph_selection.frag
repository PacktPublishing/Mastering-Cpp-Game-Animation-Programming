#version 460 core
layout (location = 0) in vec4 color;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 texCoord;
layout (location = 3) flat in float selectInfo;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out float SelectedInstance;

uniform sampler2D tex;

layout (std140, binding = 0) uniform Matrices {
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
  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * max(vec3(lightColor), vec3(0.05, 0.05, 0.05));

  vec3 norm = normalize(vec3(normal));
  vec3 lightDir = normalize(vec3(lightPos));

  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * vec3(lightColor);

  float fogDistance = gl_FragCoord.z / gl_FragCoord.w;
  float fogAmount = 1.0 - clamp(exp(-pow(fogDensity * fogDistance, 2.0)), 0.0, 1.0);
  vec4 fogColor = 0.25 * vec4(vec3(lightColor), 1.0);

  FragColor = mix(vec4(min(ambient + diffuse, vec3(1.0)), 1.0) * texture(tex, texCoord) * color, fogColor * color, fogAmount);
  FragColor.rgb = sRGB(FragColor.rgb);

  /* fill the second color attachment with the ID of our model */
  SelectedInstance = selectInfo;
}

