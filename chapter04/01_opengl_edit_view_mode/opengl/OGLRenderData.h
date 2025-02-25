/* OpenGL */
#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

#include <glm/glm.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <assimp/material.h>

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
};

struct OGLLineMesh {
  std::vector<OGLLineVertex> vertices{};
};

/* data format to be uploaded to compute shader */
struct NodeTransformData {
  glm::vec4 translation = glm::vec4(0.0f);
  glm::vec4 scale = glm::vec4(1.0f);
  glm::vec4 rotation = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // this is a quaternion
};

enum class appMode : uint8_t {
  edit = 0,
  view
};

enum class instanceEditMode : uint8_t {
  move = 0,
  rotate,
  scale
};

using appExitCallback = std::function<void(void)>;

struct OGLRenderData {
  GLFWwindow *rdWindow = nullptr;

  int rdWidth = 0;
  int rdHeight = 0;

  unsigned int rdTriangleCount = 0;
  unsigned int rdMatricesSize = 0;

  int rdFieldOfView = 60;

  float rdFrameTime = 0.0f;
  float rdMatrixGenerateTime = 0.0f;
  float rdUploadToVBOTime = 0.0f;
  float rdUploadToUBOTime = 0.0f;
  float rdUIGenerateTime = 0.0f;
  float rdUIDrawTime = 0.0f;

  int rdMoveForward = 0;
  int rdMoveRight = 0;
  int rdMoveUp = 0;

  float rdViewAzimuth = 330.0f;
  float rdViewElevation = -20.0f;
  glm::vec3 rdCameraWorldPosition = glm::vec3(2.0f, 5.0f, 7.0f);

  bool rdHighlightSelectedInstance = false;
  float rdSelectedInstanceHighlightValue = 1.0f;

  appMode rdApplicationMode = appMode::edit;
  instanceEditMode rdInstanceEditMode = instanceEditMode::move;
};
