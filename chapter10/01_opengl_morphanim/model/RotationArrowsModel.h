/* rotation arrows */
#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "OGLRenderData.h"

class RotationArrowsModel {
  public:
    OGLLineMesh getVertexData();

  private:
    void init();
    OGLLineMesh mVertexData;
};
