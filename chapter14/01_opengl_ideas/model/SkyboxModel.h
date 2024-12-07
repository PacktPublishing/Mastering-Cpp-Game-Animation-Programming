#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "OGLRenderData.h"

class SkyboxModel {
  public:
    void init();

    OGLSkyboxMesh getVertexData();

  private:
    OGLSkyboxMesh mVertexData;
};
