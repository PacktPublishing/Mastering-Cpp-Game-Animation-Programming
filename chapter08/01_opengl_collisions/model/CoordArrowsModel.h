/* coordinate arrows */
#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "OGLRenderData.h"

class CoordArrowsModel {
  public:
    OGLLineMesh getVertexData();

  private:
    void init();
    OGLLineMesh mVertexData;
};
