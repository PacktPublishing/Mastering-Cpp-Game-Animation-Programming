/* scale arrows */
#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "OGLRenderData.h"

class ScaleArrowsModel {
  public:
    OGLLineMesh getVertexData();

  private:
    void init();
    OGLLineMesh mVertexData;
};
