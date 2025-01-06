#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

layout (location = 0) out vec4 lineColor;

layout (std140, set = 0, binding = 0) uniform Matrices {
  mat4 view;
  mat4 projection;
};

layout (std430, set = 0, binding = 1) readonly restrict buffer BoundingSpheres {
  vec4 sphereData[];
};

mat3 createScaleMatrix(float s) {
  return mat3 (
    s, 0.0, 0.0,
    0.0,   s, 0.0,
    0.0, 0.0,   s
  );
}

void main() {
  vec3 boneCenter = sphereData[gl_InstanceIndex].xyz;
  float radius = sphereData[gl_InstanceIndex].w;

  mat3 scaleMat = createScaleMatrix(radius);

  gl_Position = projection * view * vec4(scaleMat * aPos + boneCenter, 1.0);
  lineColor = vec4(aColor, 1.0);
}
