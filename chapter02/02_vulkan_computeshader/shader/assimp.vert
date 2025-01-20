#version 460 core
layout (location = 0) in vec4 aPos; // last float is uv.x :)
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec4 aNormal; // last float is uv.y
layout (location = 3) in uvec4 aBoneNum; // ignored
layout (location = 4) in vec4 aBoneWeight; // ignored

layout (location = 0) out vec4 color;
layout (location = 1) out vec4 normal;
layout (location = 2) out vec2 texCoord;

layout (push_constant) uniform Constants {
  uint modelStride;
  uint worldPosOffset;
  uint skinMatrixOffset;
};

layout (std140, set = 1, binding = 0) uniform Matrices {
  mat4 view;
  mat4 projection;
};

layout (std430, set = 1, binding = 1) readonly buffer WorldPosMatrices {
  mat4 worldPosMat[];
};

void main() {
  mat4 modelMat = worldPosMat[gl_InstanceIndex + worldPosOffset];
  gl_Position = projection * view * modelMat * vec4(aPos.x, aPos.y, aPos.z, 1.0);
  color = aColor;
  normal = transpose(inverse(modelMat)) * vec4(aNormal.x, aNormal.y, aNormal.z, 1.0);
  texCoord = vec2(aPos.w, aNormal.w);
}
