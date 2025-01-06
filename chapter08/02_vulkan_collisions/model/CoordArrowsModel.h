/* coordinate arrows */
#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "VkRenderData.h"

class CoordArrowsModel {
  public:
    VkLineMesh getVertexData();

  private:
    void init();
    VkLineMesh mVertexData{};
};
