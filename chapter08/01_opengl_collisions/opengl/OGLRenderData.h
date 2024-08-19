/* OpenGL */
#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <assimp/material.h>

#include "Enums.h"
#include "Callbacks.h"

#pragma pack(push, 1) // exact fit to match std430
struct OGLVertex {
  glm::vec4 position = glm::vec4(0.0f); // last float is uv.x
  glm::vec4 color = glm::vec4(1.0f);
  glm::vec4 normal = glm::vec4(0.0f); // last float is uv.y
  glm::ivec4 boneNumber = glm::ivec4(0);
  glm::vec4 boneWeight = glm::vec4(0.0f);
};
#pragma pack(pop) // exact fit

struct OGLMesh {
  std::vector<OGLVertex> vertices{};
  std::vector<uint32_t> indices{};
  std::unordered_map<aiTextureType, std::string> textures{};
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
  float firstClipReplayTimestamp;
  float secondClipReplayTimestamp;
  float blendFactor;
};

struct OGLRenderData {
  GLFWwindow *rdWindow = nullptr;

  int rdWidth = 0;
  int rdHeight = 0;
  bool rdFullscreen = false;

  unsigned int rdTriangleCount = 0;
  unsigned int rdMatricesSize = 0;

  float rdFrameTime = 0.0f;
  float rdMatrixGenerateTime = 0.0f;
  float rdUploadToVBOTime = 0.0f;
  float rdUploadToUBOTime = 0.0f;
  float rdUIGenerateTime = 0.0f;
  float rdUIDrawTime = 0.0f;
  float rdCollisionDebugDrawTime = 0.0f;
  float rdCollisionCheckTime = 0.0f;

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

  glm::vec2 rdWorldStartPos = glm::vec2(-128.0f);
  glm::vec2 rdWorldSize = glm::vec2(256.0f);

  collisionChecks rdCheckCollisions = collisionChecks::none;
  size_t rdNumberOfCollisions = 0;

  collisionDebugDraws rdDrawCollisionAABBs = collisionDebugDraws::none;
  collisionDebugDraws rdDrawBoundingSpheres = collisionDebugDraws::none;
};
