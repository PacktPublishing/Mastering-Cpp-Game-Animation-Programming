#pragma once

#include <memory>
#include <tuple>
#include <glm/glm.hpp>

#include "OGLRenderData.h"

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

    std::shared_ptr<OGLLineMesh> getAABBLines(bool collided);

  private:
    glm::vec3 mMinPos;
    glm::vec3 mMaxPos;

    std::shared_ptr<OGLLineMesh> mAabbMesh = nullptr;
};
