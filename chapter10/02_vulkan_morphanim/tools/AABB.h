#pragma once

#include <memory>
#include <tuple>
#include <glm/glm.hpp>

#include "VkRenderData.h"

class AABB {
  public:
    AABB();

    void clear();
    void create(glm::vec3 point);
    void addPoint(glm::vec3 point);

    glm::vec3 getMinPos();
    glm::vec3 getMaxPos();
    std::pair<glm::vec3, glm::vec3> getExtents();

    void setMinPos(glm::vec3 pos);
    void setMaxPos(glm::vec3 pos);
    void setExtents(glm::vec3 minPos, glm::vec3 maxPos);

    std::shared_ptr<VkLineMesh> getAABBLines(glm::vec3 color);

  private:
    glm::vec3 mMinPos;
    glm::vec3 mMaxPos;

    std::shared_ptr<VkLineMesh> mAabbMesh = nullptr;
};
