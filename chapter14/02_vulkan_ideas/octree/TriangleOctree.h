/* based on https://github.com/pvigier/Quadtree
   see LICENSE file for original MIT license */

#pragma once

#include <array>
#include <vector>
#include <tuple>
#include <memory>

#include "Enums.h"
#include "Callbacks.h"
#include "VkRenderData.h"
#include "BoundingBox3D.h"

class TriangleOctree {
  public:
    TriangleOctree(std::shared_ptr<BoundingBox3D> rootBox, int threshold = 16, int maxDepth = 8);

    void add(MeshTriangle triangle);

    std::vector<MeshTriangle> query(BoundingBox3D box);

    std::vector<BoundingBox3D> getTreeBoxes();

    void clear();

  private:
    struct TriangleOctreeNode {
      std::array<std::shared_ptr<TriangleOctreeNode>, 8> childs{};
      std::vector<MeshTriangle> triangles{};
    };
    BoundingBox3D mRootBoundingBox{};
    std::shared_ptr<TriangleOctreeNode> mRootNode = nullptr;

    int mThreshold = 1;
    int mMaxDepth = 1;

    bool isLeaf(std::shared_ptr<TriangleOctreeNode> node);

    BoundingBox3D getChildOctant(BoundingBox3D parentBox, int octantId);
    int getOctantId(BoundingBox3D nodeBox, BoundingBox3D valueBox);

    void add(std::shared_ptr<TriangleOctreeNode> node, int depth, BoundingBox3D box, MeshTriangle triangle);
    void split(std::shared_ptr<TriangleOctreeNode> node, BoundingBox3D box);

    std::vector<MeshTriangle> query(std::shared_ptr<TriangleOctreeNode> node, BoundingBox3D box, BoundingBox3D queryBox);

    std::vector<BoundingBox3D> getTreeBoxes(std::shared_ptr<TriangleOctreeNode> node, BoundingBox3D box);
};

