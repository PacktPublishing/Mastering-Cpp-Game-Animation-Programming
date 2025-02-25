#version 460 core
layout (location = 0) in vec4 aPos; // last float is uv.x :)
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec4 aNormal; // last float is uv.y
layout (location = 3) in uvec4 aBoneNum;
layout (location = 4) in vec4 aBoneWeight;

layout (location = 0) out vec4 color;
layout (location = 1) out vec4 normal;
layout (location = 2) out vec2 texCoord;

layout (std140, binding = 0) uniform Matrices {
  mat4 view;
  mat4 projection;
};

layout (std430, binding = 1) readonly restrict buffer BoneMatrices {
  mat4 boneMat[];
};

layout (std430, binding = 2) readonly restrict buffer WorldPosMatrices {
  mat4 worldPos[];
};

uniform int aModelStride;

void main() {

  int modelStride = gl_InstanceID * aModelStride;

  mat4 skinMat =
    aBoneWeight.x * boneMat[aBoneNum.x + modelStride] +
    aBoneWeight.y * boneMat[aBoneNum.y + modelStride] +
    aBoneWeight.z * boneMat[aBoneNum.z + modelStride] +
    aBoneWeight.w * boneMat[aBoneNum.w + modelStride];

  mat4 worldPosSkinMat = worldPos[gl_InstanceID] * skinMat;
  gl_Position = projection * view * worldPosSkinMat * vec4(aPos.x, aPos.y, aPos.z, 1.0);
  color = aColor;
  normal = transpose(inverse(worldPosSkinMat)) * vec4(aNormal.x, aNormal.y, aNormal.z, 1.0);
  texCoord = vec2(aPos.w, aNormal.w);
}
