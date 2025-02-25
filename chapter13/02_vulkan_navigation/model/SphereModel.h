#pragma once

#include <glm/glm.hpp>

#include "VkRenderData.h"

class SphereModel {
  public:
    SphereModel(const float radius = 1.0f, const float verticalDiv = 5, const float horizontalDiv = 8, const glm::vec3 color = glm::vec3(1.0f)) :
      mRadius(radius), mVertDiv(verticalDiv), mHorDiv(horizontalDiv), mColor(color) {};

    VkLineMesh getVertexData();

  private:
    void init();

    float mRadius = 1.0f;
    unsigned int mVertDiv = 10;
    unsigned int mHorDiv = 20;

    glm::vec3 mColor = glm::vec3(1.0f);

    VkLineMesh mVertexData{};
};
