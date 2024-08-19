/* based on https://github.com/pvigier/Quadtree
   see LICENSE file for original MIT license */

#pragma once

#include <array>
#include <vector>
#include <tuple>
#include <memory>
#include <set>

#include "Enums.h"
#include "AssimpInstance.h"
#include "BoundingBox2D.h"

class QuadTree {
  public:
    QuadTree(std::shared_ptr<BoundingBox2D> rootBox, int threshold = 16, int maxDepth = 8);

    void add(std::shared_ptr<AssimpInstance> instance);
    void remove(std::shared_ptr<AssimpInstance> instance);
    void update(std::shared_ptr<AssimpInstance> instance);

    std::vector<int> query(BoundingBox2D box);
    std::set<std::pair<int, int>> findAllIntersections();

    std::vector<BoundingBox2D> getTreeBoxes();

    void clear();

    instanceGetBoundingBox2D instanceGetBoundingBox2DCallback;

  private:
    struct QuadTreeNode {
      std::array<std::shared_ptr<QuadTreeNode>, 4> childs{};
      std::vector<int> instancIds{};
    };
    BoundingBox2D mRootBoundingBox{};
    std::shared_ptr<QuadTreeNode> mRootNode = nullptr;

    int mThreshold = 1;
    int mMaxDepth = 1;

    bool isLeaf(const std::shared_ptr<QuadTreeNode> node);

    BoundingBox2D getChildQuadrant(BoundingBox2D parentBox, int quadrantId);
    int getQuadrantId(BoundingBox2D nodeBox, BoundingBox2D valueBox);

    void add(std::shared_ptr<QuadTreeNode> node, int depth, BoundingBox2D box, std::shared_ptr<AssimpInstance> instance);
    void split(std::shared_ptr<QuadTreeNode> node, BoundingBox2D box);

    bool remove(std::shared_ptr<QuadTreeNode> node, BoundingBox2D box, std::shared_ptr<AssimpInstance> instance);
    void removeInstance(std::shared_ptr<QuadTreeNode> node, std::shared_ptr<AssimpInstance> instance);
    bool tryMerge(std::shared_ptr<QuadTreeNode> node);

    std::vector<int> query(std::shared_ptr<QuadTreeNode> node, BoundingBox2D box, BoundingBox2D queryBox);

    std::set<std::pair<int, int>> findAllIntersections(std::shared_ptr<QuadTreeNode> node);
    std::set<std::pair<int, int>> findIntersectionsInDescendants(std::shared_ptr<QuadTreeNode> node, int instanceId);

    std::vector<BoundingBox2D> getTreeBoxes(std::shared_ptr<QuadTreeNode> node, BoundingBox2D box);
};

