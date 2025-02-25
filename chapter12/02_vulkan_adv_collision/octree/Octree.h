/* based on https://github.com/pvigier/Quadtree
   see LICENSE file for original MIT license */

#pragma once

#include <array>
#include <vector>
#include <tuple>
#include <memory>
#include <set>

#include "Enums.h"
#include "Callbacks.h"
#include "BoundingBox3D.h"

class Octree {
  public:
    Octree(std::shared_ptr<BoundingBox3D> rootBox, int threshold = 16, int maxDepth = 8);

    void add(int instanceId);
    void remove(int instanceId);
    void update(int instanceId);

    std::set<int> query(BoundingBox3D box);
    std::set<std::pair<int, int>> findAllIntersections();

    std::vector<BoundingBox3D> getTreeBoxes();

    void clear();

    instanceGetBoundingBoxCallback mInstanceGetBoundingBoxCallbackFunction;

  private:
    struct OctreeNode {
      std::array<std::shared_ptr<OctreeNode>, 8> childs{};
      std::vector<int> instancIds{};
    };
    BoundingBox3D mRootBoundingBox{};
    std::shared_ptr<OctreeNode> mRootNode = nullptr;

    int mThreshold = 1;
    int mMaxDepth = 1;

    bool isLeaf(const std::shared_ptr<OctreeNode> node);

    BoundingBox3D getChildOctant(BoundingBox3D parentBox, int octantId);
    int getOctantId(BoundingBox3D nodeBox, BoundingBox3D valueBox);

    void add(std::shared_ptr<OctreeNode> node, int depth, BoundingBox3D box, int instanceId);
    void split(std::shared_ptr<OctreeNode> node, BoundingBox3D box);

    bool remove(std::shared_ptr<OctreeNode> node, BoundingBox3D box, int instanceId);
    void removeInstance(std::shared_ptr<OctreeNode> node, int instanceId);
    bool tryMerge(std::shared_ptr<OctreeNode> node);

    std::vector<int> query(std::shared_ptr<OctreeNode> node, BoundingBox3D box, BoundingBox3D queryBox);

    std::set<std::pair<int, int>> findAllIntersections(std::shared_ptr<OctreeNode> node);
    std::set<std::pair<int, int>> findIntersectionsInDescendants(std::shared_ptr<OctreeNode> node, int instanceId);

    std::vector<BoundingBox3D> getTreeBoxes(std::shared_ptr<OctreeNode> node, BoundingBox3D box);
};

