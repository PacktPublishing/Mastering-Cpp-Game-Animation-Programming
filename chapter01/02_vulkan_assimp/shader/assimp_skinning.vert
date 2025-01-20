#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;
layout (location = 4) in uvec4 aBoneNum;
layout (location = 5) in vec4 aBoneWeight;

layout (location = 0) out vec4 color;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec2 texCoord;

layout (push_constant) uniform Constants {
  int modelStride;
  int worldPosOffset;
};

layout (std140, set = 1, binding = 0) uniform Matrices {
  mat4 view;
  mat4 projection;
};

layout (std430, set = 1, binding = 1) readonly restrict buffer BoneMatrices {
  mat4 boneMat[];
};

void main() {
  int skinMatOffset = gl_InstanceIndex * modelStride + worldPosOffset;

  mat4 skinMat =
    aBoneWeight.x * boneMat[aBoneNum.x + skinMatOffset] +
    aBoneWeight.y * boneMat[aBoneNum.y + skinMatOffset] +
    aBoneWeight.z * boneMat[aBoneNum.z + skinMatOffset] +
    aBoneWeight.w * boneMat[aBoneNum.w + skinMatOffset];

  gl_Position = projection * view * skinMat * vec4(aPos, 1.0);
  color = aColor;
  normal = vec3(transpose(inverse(skinMat)) * vec4(aNormal, 1.0));
  texCoord = aTexCoord;
}
