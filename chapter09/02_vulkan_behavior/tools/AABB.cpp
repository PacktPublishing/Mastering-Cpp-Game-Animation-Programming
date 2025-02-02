#include <limits>
#include "AABB.h"

AABB::AABB() {
  mAabbMesh = std::make_shared<VkLineMesh>();
  mAabbMesh->vertices.resize(24);

  clear();
}

void AABB::clear() {
  mMinPos = glm::vec3(std::numeric_limits<float>::max());
  mMaxPos = glm::vec3(-std::numeric_limits<float>::max());
}

void AABB::create(glm::vec3 point) {
  mMinPos = point;
  mMaxPos = point;
}

void AABB::addPoint(glm::vec3 point) {
  mMinPos.x = std::min(mMinPos.x, point.x);
  mMinPos.y = std::min(mMinPos.y, point.y);
  mMinPos.z = std::min(mMinPos.z, point.z);

  mMaxPos.x = std::max(mMaxPos.x, point.x);
  mMaxPos.y = std::max(mMaxPos.y, point.y);
  mMaxPos.z = std::max(mMaxPos.z, point.z);
}

glm::vec3 AABB::getMinPos() {
  return mMinPos;
}

glm::vec3 AABB::getMaxPos() {
  return mMaxPos;
}

std::pair<glm::vec3, glm::vec3> AABB::getExtents() {
  return std::make_pair(mMinPos, mMaxPos);
}

void AABB::setMinPos(glm::vec3 pos) {
  mMinPos = pos;
}

void AABB::setMaxPos(glm::vec3 pos) {
  mMaxPos = pos;
}

void AABB::setExtents(glm::vec3 minPos, glm::vec3 maxPos) {
  mMinPos = minPos;
  mMaxPos = maxPos;
}

std::shared_ptr<VkLineMesh> AABB::getAABBLines(glm::vec3 color) {
  mAabbMesh->vertices.at(0) = {{mMinPos.x, mMinPos.y, mMinPos.z}, color};
  mAabbMesh->vertices.at(1) = {{mMaxPos.x, mMinPos.y, mMinPos.z}, color};

  mAabbMesh->vertices.at(2) = {{mMinPos.x, mMinPos.y, mMinPos.z}, color};
  mAabbMesh->vertices.at(3) = {{mMinPos.x, mMaxPos.y, mMinPos.z}, color};

  mAabbMesh->vertices.at(4) = {{mMaxPos.x, mMaxPos.y, mMinPos.z}, color};
  mAabbMesh->vertices.at(5) = {{mMaxPos.x, mMinPos.y, mMinPos.z}, color};

  mAabbMesh->vertices.at(6) = {{mMaxPos.x, mMaxPos.y, mMinPos.z}, color};
  mAabbMesh->vertices.at(7) = {{mMinPos.x, mMaxPos.y, mMinPos.z}, color};


  mAabbMesh->vertices.at(8) = {{mMinPos.x, mMinPos.y, mMaxPos.z}, color};
  mAabbMesh->vertices.at(9) = {{mMaxPos.x, mMinPos.y, mMaxPos.z}, color};

  mAabbMesh->vertices.at(10) = {{mMinPos.x, mMinPos.y, mMaxPos.z}, color};
  mAabbMesh->vertices.at(11) = {{mMinPos.x, mMaxPos.y, mMaxPos.z}, color};

  mAabbMesh->vertices.at(12) = {{mMaxPos.x, mMaxPos.y, mMaxPos.z}, color};
  mAabbMesh->vertices.at(13) = {{mMaxPos.x, mMinPos.y, mMaxPos.z}, color};

  mAabbMesh->vertices.at(14) = {{mMaxPos.x, mMaxPos.y, mMaxPos.z}, color};
  mAabbMesh->vertices.at(15) = {{mMinPos.x, mMaxPos.y, mMaxPos.z}, color};


  mAabbMesh->vertices.at(16) = {{mMinPos.x, mMinPos.y, mMinPos.z}, color};
  mAabbMesh->vertices.at(17) = {{mMinPos.x, mMinPos.y, mMaxPos.z}, color};

  mAabbMesh->vertices.at(18) = {{mMaxPos.x, mMinPos.y, mMinPos.z}, color};
  mAabbMesh->vertices.at(19) = {{mMaxPos.x, mMinPos.y, mMaxPos.z}, color};

  mAabbMesh->vertices.at(20) = {{mMinPos.x, mMaxPos.y, mMinPos.z}, color};
  mAabbMesh->vertices.at(21) = {{mMinPos.x, mMaxPos.y, mMaxPos.z}, color};

  mAabbMesh->vertices.at(22) = {{mMaxPos.x, mMaxPos.y, mMinPos.z}, color};
  mAabbMesh->vertices.at(23) = {{mMaxPos.x, mMaxPos.y, mMaxPos.z}, color};

  return mAabbMesh;
}
