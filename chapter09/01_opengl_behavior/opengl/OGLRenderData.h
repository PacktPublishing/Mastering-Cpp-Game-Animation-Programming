/* OpenGL */
#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <set>

#include <glm/glm.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <assimp/material.h>

#include "Enums.h"
#include "Callbacks.h"

struct OGLVertex {
  glm::vec4 position = glm::vec4(0.0f); // last float is uv.x
  glm::vec4 color = glm::vec4(1.0f);
  glm::vec4 normal = glm::vec4(0.0f); // last float is uv.y
  glm::uvec4 boneNumber = glm::uvec4(0);
  glm::vec4 boneWeight = glm::vec4(0.0f);
};

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
  uint32_t firstAnimClipNum;
  uint32_t secondAnimClipNum;
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
  float rdBehaviorTime = 0.0f;
  float rdInteractionTime = 0.0f;

  int rdMoveForward = 0;
  int rdMoveRight = 0;
  int rdMoveUp = 0;

  bool rdHighlightSelectedInstance = false;
  float rdSelectedInstanceHighlightValue = 1.0f;

  appMode rdApplicationMode = appMode::edit;
  std::unordered_map<appMode, std::string> mAppModeMap{};

  instanceEditMode rdInstanceEditMode = instanceEditMode::move;

  appExitCallback rdAppExitCallbackFunction;
  bool rdRequestApplicationExit = false;
  bool rdNewConfigRequest = false;
  bool rdLoadConfigRequest = false;
  bool rdSaveConfigRequest = false;

  glm::vec2 rdWorldStartPos = glm::vec2(-160.0f);
  glm::vec2 rdWorldSize = glm::vec2(320.0f);

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
};
