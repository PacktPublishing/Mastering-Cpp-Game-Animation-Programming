#version 460 core
layout (location = 0) in vec4 aPos; // last float is uv.x :)
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec4 aNormal; // last float is uv.y
layout (location = 3) in uvec4 aBoneNum;
layout (location = 4) in vec4 aBoneWeight;

layout (location = 0) out vec4 color;
layout (location = 1) out vec4 normal;
layout (location = 2) out vec2 texCoord;

struct MorphVertex {
  vec4 position;
  vec4 normal;
};

layout (push_constant) uniform Constants {
  uint modelStride;
  uint worldPosOffset;
  uint skinMatrixOffset;
};

layout (std140, set = 1, binding = 0) uniform Matrices {
  mat4 view;
  mat4 projection;
};

layout (std430, set = 1, binding = 1) readonly restrict buffer BoneMatrices {
  mat4 boneMat[];
};

layout (std430, set = 1, binding = 2) readonly restrict buffer WorldPosMatrices {
  mat4 worldPos[];
};

layout (std430, set = 1, binding = 3) readonly restrict buffer InstanceSelected {
  vec2 selected[];
};

layout (std430, set = 1, binding = 4) readonly restrict buffer AnimMorphData {
  vec4 vertsPerMorphAnim[];
};

layout (std430, set = 2, binding = 0) readonly restrict buffer AnimMorphBuffer {
  MorphVertex morphVertices[];
};

void main() {
  uint skinMatOffset = gl_InstanceIndex * modelStride + skinMatrixOffset;

  mat4 skinMat =
    aBoneWeight.x * boneMat[aBoneNum.x + skinMatOffset] +
    aBoneWeight.y * boneMat[aBoneNum.y + skinMatOffset] +
    aBoneWeight.z * boneMat[aBoneNum.z + skinMatOffset] +
    aBoneWeight.w * boneMat[aBoneNum.w + skinMatOffset];

  mat4 worldPosSkinMat = worldPos[gl_InstanceIndex + worldPosOffset] * skinMat;

  /* y and z data contain the offset into the morph anim buffer */
  int morphAnimIndex = int(vertsPerMorphAnim[gl_InstanceIndex + worldPosOffset].y *
    vertsPerMorphAnim[gl_InstanceIndex + worldPosOffset].z);

  vec4 origVertex = vec4(aPos.x, aPos.y, aPos.z, 1.0);
  vec4 morphVertex = vec4(morphVertices[gl_VertexIndex + morphAnimIndex].position.xyz, 1.0);

  gl_Position = projection * view * worldPosSkinMat *
    mix(origVertex, morphVertex, vertsPerMorphAnim[gl_InstanceIndex + worldPosOffset].x);

  color = aColor * selected[gl_InstanceIndex + worldPosOffset].x;
  /* draw the instance always on top when highlighted, helps to find it better */
  if (selected[gl_InstanceIndex + worldPosOffset].x != 1.0f) {
    gl_Position.z -= 1.0f;
  }

  vec4 origNormal = vec4(aNormal.x, aNormal.y, aNormal.z, 1.0);
  vec4 morphNormal = vec4(morphVertices[gl_VertexIndex + morphAnimIndex].normal.xyz, 1.0);
  normal = transpose(inverse(worldPosSkinMat)) *
    mix(origNormal, morphNormal, vertsPerMorphAnim[gl_InstanceIndex + worldPosOffset].x);

  texCoord = vec2(aPos.w, aNormal.w);
}
