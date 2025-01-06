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

layout (std140, binding = 0) uniform Matrices {
  mat4 view;
  mat4 projection;
};

layout (std430, binding = 1) readonly restrict buffer BoneMatrices {
  mat4 boneMat[];
};

uniform int aModelStride;

void main() {

  mat4 skinMat =
    aBoneWeight.x * boneMat[aBoneNum.x + gl_InstanceID * aModelStride] +
    aBoneWeight.y * boneMat[aBoneNum.y + gl_InstanceID * aModelStride] +
    aBoneWeight.z * boneMat[aBoneNum.z + gl_InstanceID * aModelStride] +
    aBoneWeight.w * boneMat[aBoneNum.w + gl_InstanceID * aModelStride];

  gl_Position = projection * view * skinMat * vec4(aPos, 1.0);
  color = aColor;
  normal = vec3(transpose(inverse(skinMat)) * vec4(aNormal, 1.0));
  texCoord = aTexCoord;
}
