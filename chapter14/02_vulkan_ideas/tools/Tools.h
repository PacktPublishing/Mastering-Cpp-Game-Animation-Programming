/* Tools functions */
#pragma once
#include <string>
#include <optional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <assimp/matrix4x4.h>

#include "VkRenderData.h"

class Tools {
  public:
    static std::string getFilenameExt(std::string filename);
    static std::string loadFileToString(std::string fileName);

    static glm::mat4 convertAiToGLM(aiMatrix4x4 inMat);

    static std::optional<glm::vec3> rayTriangleIntersection(glm::vec3 rayOrigin, glm::vec3 rayDirection, MeshTriangle triangle);

    static glm::vec4 extractGlobalPosition(glm::mat4 nodeMatrix);
    static glm::quat extractGlobalRotation(glm::mat4 nodeMatrix);

    static std::vector<std::string> getDirectoryContent(std::string path, std::string extension);
};
