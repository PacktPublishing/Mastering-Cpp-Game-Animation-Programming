/* OpenGL */
#pragma once
#include <vector>
#include <array>
#include <string>
#include <unordered_map>
#include <memory>
#include <set>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <assimp/material.h>

#include "Enums.h"
#include "Callbacks.h"
#include "BoundingBox3D.h"

/* morph animations only need those two */
struct OGLMorphVertex {
  glm::vec4 position = glm::vec4(0.0f);
  glm::vec4 normal = glm::vec4(0.0f);
};

struct OGLMorphMesh {
  std::vector<OGLMorphVertex> morphVertices{};
};

struct OGLVertex {
  glm::vec4 position = glm::vec4(0.0f); // last float is uv.x
  glm::vec4 color = glm::vec4(1.0f);
  glm::vec4 normal = glm::vec4(0.0f); // last float is uv.y
  glm::ivec4 boneNumber = glm::ivec4(0);
  glm::vec4 boneWeight = glm::vec4(0.0f);
};

struct OGLMesh {
  std::vector<OGLVertex> vertices{};
  std::vector<uint32_t> indices{};
  std::unordered_map<aiTextureType, std::string> textures{};
  /* store optional morph meshes directly in renderer mesh */
  std::vector<OGLMorphMesh> morphMeshes{};
};

struct OGLLineVertex {
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 color = glm::vec3(0.0f);

  OGLLineVertex() {};
  OGLLineVertex(glm::vec3 pos, glm::vec3 col) : position(pos), color(col) {};
};

struct OGLLineMesh {
  std::vector<OGLLineVertex> vertices{};
};

struct PerInstanceAnimData {
  unsigned int firstAnimClipNum;
  unsigned int secondAnimClipNum;
  unsigned int headLeftRightAnimClipNum;
  unsigned int headUpDownAnimClipNum;
  float firstClipReplayTimestamp;
  float secondClipReplayTimestamp;
  float headLeftRightReplayTimestamp;
  float headUpDownReplayTimestamp;
  float blendFactor;
};

struct MeshTriangle {
  int index;
  std::array<glm::vec3, 3> points;
  glm::vec3 normal;
  BoundingBox3D boundingBox;
};

struct TRSMatrixData{
  glm::vec4 translation;
  glm::quat rotation;
  glm::vec4 scale;
};

struct OGLRenderData {
  GLFWwindow *rdWindow = nullptr;

  int rdWidth = 0;
  int rdHeight = 0;
  bool rdFullscreen = false;

  unsigned int rdTriangleCount = 0;
  unsigned int rdLevelTriangleCount = 0;
  unsigned int rdMatricesSize = 0;

  float rdFrameTime = 0.0f;
  float rdMatrixGenerateTime = 0.0f;
  float rdUploadToVBOTime = 0.0f;
  float rdUploadToUBOTime = 0.0f;
  float rdDownloadFromUBOTime = 0.0f;
  float rdUIGenerateTime = 0.0f;
  float rdUIDrawTime = 0.0f;
  float rdCollisionDebugDrawTime = 0.0f;
  float rdCollisionCheckTime = 0.0f;
  float rdBehaviorTime = 0.0f;
  float rdInteractionTime = 0.0f;
  float rdFaceAnimTime = 0.0f;
  float rdLevelCollisionTime = 0.0f;
  float rdIKTime = 0.0f;

  int rdMoveForward = 0;
  int rdMoveRight = 0;
  int rdMoveUp = 0;

  bool rdHighlightSelectedInstance = false;
  float rdSelectedInstanceHighlightValue = 1.0f;

  appMode rdApplicationMode = appMode::edit;
  std::unordered_map<appMode, std::string> mAppModeMap{};

  instanceEditMode rdInstanceEditMode = instanceEditMode::move;

  appExitCallback rdAppExitCallback;
  bool rdRequestApplicationExit = false;
  bool rdNewConfigRequest = false;
  bool rdLoadConfigRequest = false;
  bool rdSaveConfigRequest = false;

  glm::vec3 rdDefaultWorldStartPos = glm::vec3(-160.0f);
  glm::vec3 rdDefaultWorldSize = glm::vec3(320.0f);
  glm::vec3 rdWorldStartPos = rdDefaultWorldStartPos;
  glm::vec3 rdWorldSize = rdDefaultWorldSize;

  collisionChecks rdCheckCollisions = collisionChecks::none;
  size_t rdNumberOfCollisions = 0;

  collisionDebugDraw rdDrawCollisionAABBs = collisionDebugDraw::none;
  collisionDebugDraw rdDrawBoundingSpheres = collisionDebugDraw::none;

  bool rdInteraction = false;
  float rdInteractionMaxRange = 10.0f;
  float rdInteractionMinRange = 1.5f;
  float rdInteractionFOV = 45.0f;
  size_t rdNumberOfInteractionCandidates = 0;
  std::set<int> rdInteractionCandidates{};
  int rdInteractWithInstanceId = 0;

  interactionDebugDraw rdDrawInteractionAABBs = interactionDebugDraw::none;
  bool rdDrawInteractionRange = false;
  bool rdDrawInteractionFOV = false;

  int rdOctreeThreshold = 10;
  int rdOctreeMaxDepth = 5;

  int rdLevelOctreeThreshold = 10;
  int rdLevelOctreeMaxDepth = 5;

  bool rdDrawLevelAABB = false;
  bool rdDrawLevelWireframe = false;
  bool rdDrawLevelOctree = false;
  bool rdDrawLevelCollisionTriangles = false;

  float rdMaxLevelGroundSlopeAngle = 90.0f;
  float rdMaxStairstepHeight = 1.0f;
  glm::vec3 rdLevelCollisionAABBExtension = glm::vec3(0.0f, 1.0f, 0.0f);

  int rdNumberOfCollidingTriangles = 0;
  int rdNumberOfCollidingGroundTriangles = 0;

  bool rdEnableSimpleGravity = false;

  bool rdEnableFeetIK = false;
  int rdNumberOfIkIteratons = 10;
  bool rdDrawIKDebugLines = false;
};
