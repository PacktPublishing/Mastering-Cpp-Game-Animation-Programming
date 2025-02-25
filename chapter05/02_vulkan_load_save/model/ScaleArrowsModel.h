/* scale arrows */
#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "VkRenderData.h"

class ScaleArrowsModel {
  public:
    VkLineMesh getVertexData();

  private:
    void init();
    VkLineMesh mVertexData{};
};
