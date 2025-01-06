/* rotation arrows */
#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "VkRenderData.h"

class RotationArrowsModel {
  public:
    VkLineMesh getVertexData();

  private:
    void init();
    VkLineMesh mVertexData{};
};
