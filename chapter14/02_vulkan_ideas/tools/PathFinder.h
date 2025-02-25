#pragma once

#include <vector>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <glm/glm.hpp>

#include "TriangleOctree.h"
#include "BoundingBox3D.h"
#include "VkRenderData.h"

struct NavTriangle {
  int index;
  std::array<glm::vec3, 3> points{};
  glm::vec3 center{};
  glm::vec3 normal{};
  std::unordered_set<int> neighborTris{};
};

struct NavData {
  int triIndex;
  int prevTriIndex;
  float distanceFromSource;
  float heuristicToDest;
  float distanceToDest;
};

class PathFinder {
  public:
    void generateGroundTriangles(VkRenderData& renderData, std::shared_ptr<TriangleOctree> octree, BoundingBox3D worldbox);
    std::vector<int> getGroundTriangleNeighbors(int groundTriIndex);

    std::vector<int> findPath(int startTriIndex, int targetTriIndex);

    glm::vec3 getTriangleCenter(int index);

    std::shared_ptr<VkLineMesh> getGroundLevelMesh();
    std::shared_ptr<VkLineMesh> getAsLineMesh(std::vector<int> indices, glm::vec3 color, glm::vec3 offset);
    std::shared_ptr<VkLineMesh> getAsTriangleMesh(std::vector<int> indices, glm::vec3 color, glm::vec3 normalColor, glm::vec3 offset);

  private:
    std::unordered_map<int, NavTriangle> mNavTriangles{};
    std::shared_ptr<VkLineMesh> mLevelGroundMesh = nullptr;
};
