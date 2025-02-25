#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "VkRenderData.h"

class SkyboxModel {
  public:
    void init();

    VkSkyboxMesh getVertexData();

  private:
    VkSkyboxMesh mVertexData{};
};
