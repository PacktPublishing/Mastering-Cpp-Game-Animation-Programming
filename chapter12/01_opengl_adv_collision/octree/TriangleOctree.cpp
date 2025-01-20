#include <algorithm>

#include <glm/gtx/string_cast.hpp>

#include "TriangleOctree.h"
#include "Logger.h"

TriangleOctree::TriangleOctree(std::shared_ptr<BoundingBox3D> rootBox, int threshold, int maxDepth) :
   mRootBoundingBox(*rootBox), mThreshold(threshold), mMaxDepth(maxDepth) {
  mRootNode = std::make_shared<TriangleOctreeNode>();
}

bool TriangleOctree::isLeaf(std::shared_ptr<TriangleOctreeNode> node) {
  return node && !node->childs[0];
}

BoundingBox3D TriangleOctree::getChildOctant(BoundingBox3D parentBox, int octantId) {
  glm::vec3 origin = parentBox.getFrontTopLeft();
  glm::vec3 childSize = parentBox.getSize() / 2.0f;

  /* Octants:
   *
   *     +---+---+      +-----+-----+
   *    / 4 / 5 /|     / BNW / BNE /|-- back
   *   +---+---+ +    +-----+-----+ +
   *  / 0 / 1 /|/|   / FNW / FNE /|/|-- front
   * +---+---+ + +  +-----+-----+ + +
   * | 0 | 1 |/|/   | FNW | FNE |/|/
   * +---+---+ +    +-----+-----+ +
   * | 2 | 3 |/     | FSW | FSE |/
   * +---+---+      +-----+-----+
   *
   *  Back octant ID is front octant ID plus 4
   *
   */

  switch(octantId) {
    case 0:
      return BoundingBox3D(origin, childSize);
      break;
    case 1:
      return BoundingBox3D(glm::vec3(origin.x + childSize.x, origin.y, origin.z), childSize);
      break;
    case 2:
      return BoundingBox3D(glm::vec3(origin.x, origin.y + childSize.y, origin.z), childSize);
      break;
    case 3:
      return BoundingBox3D(glm::vec3(origin.x + childSize.x, origin.y + childSize.y, origin.z), childSize);
      break;

    case 4:
      return BoundingBox3D(glm::vec3(origin.x, origin.y, origin.z + childSize.z), childSize);
      break;
    case 5:
      return BoundingBox3D(glm::vec3(origin.x + childSize.x, origin.y, origin.z + childSize.z), childSize);
      break;
    case 6:
      return BoundingBox3D(glm::vec3(origin.x, origin.y + childSize.y, origin.z + childSize.z), childSize);
      break;
    case 7:
      return BoundingBox3D(origin + childSize, childSize);
      break;
    default:
      Logger::log(1, "%s error: invalid octant id %i\n", __FUNCTION__, octantId);
      return BoundingBox3D();
  }
}

int TriangleOctree::getOctantId(BoundingBox3D nodeBox, BoundingBox3D valueBox) {
  glm::vec3 center = nodeBox.getCenter();

  /* Front */
  if (valueBox.getBack() < center.z) {
    /* West */
    if (valueBox.getRight() < center.x) {
      if (valueBox.getBottom() < center.y) {
        /* FNW */
        return 0;
      } else if (valueBox.getFrontTopLeft().y >= center.y) {
        /* FSW */
        return 2;
      } else {
        /* not found */
        return -1;
      }
    /* East */
    } else if (valueBox.getFrontTopLeft().x >= center.x) {
      if (valueBox.getBottom() < center.y) {
        /* FNE */
        return 1;
      } else if ( valueBox.getFrontTopLeft().y >= center.y) {
        /* FSE */
        return 3;
      } else {
        /* not found */
        return -1;
      }
    } else {
      /* not found */
      return -1;
    }
  /* Back */
  } else if (valueBox.getBack() >= center.z) {
    /* West */
    if (valueBox.getRight() < center.x) {
      if (valueBox.getBottom() < center.y) {
        /* BNW */
        return 4;
      } else if (valueBox.getFrontTopLeft().y >= center.y) {
        /* BSW */
        return 6;
      } else {
        /* not found */
        return -1;
      }
    /* East */
    } else if (valueBox.getFrontTopLeft().x >= center.x) {
      if (valueBox.getBottom() < center.y) {
        /* BNE */
        return 5;
      } else if ( valueBox.getFrontTopLeft().y >= center.y) {
        /* BSE */
        return 7;
      } else {
        /* not found */
        return -1;
      }
    } else {
      /* not found */
      return -1;
    }
  } else {
    /* not found */
    return -1;
  }
}

void TriangleOctree::add(MeshTriangle triangle) {
  add(mRootNode, 0, mRootBoundingBox, triangle);
}

void TriangleOctree::add(std::shared_ptr<TriangleOctreeNode> node, int depth, BoundingBox3D box, MeshTriangle triangle) {
  if (!box.intersects(triangle.boundingBox)) {
    Logger::log(1, "%s error: current octree node bounding box at depth %i does not contain the bounding box of triangle %i\n",
                __FUNCTION__, depth, triangle.index);
    Logger::log(1, "%s: Triangle data: %s/%s/%s\n", __FUNCTION__,
                glm::to_string(triangle.points.at(0)).c_str(),
                glm::to_string(triangle.points.at(1)).c_str(),
                glm::to_string(triangle.points.at(2)).c_str());
    return;
  }

  if (isLeaf(node)) {
    /* insert into node if possible */
    if (depth >= mMaxDepth || node->triangles.size() < mThreshold) {
      node->triangles.emplace_back(triangle);
    } else {
      split(node, box);
      add(node, depth, box, triangle);
    }
  } else {
    int intersectingChildren = 0;
    /* check how many children we would intersect */
    for (int i = 0; i < node->childs.size(); ++i) {
      BoundingBox3D childBox = getChildOctant(box, i);
      if (childBox.intersects(triangle.boundingBox)) {
        intersectingChildren++;
      }
    }
    /* insert into root node if we would have multiple children  */
    if (intersectingChildren > 1) {
      node->triangles.emplace_back(triangle);
    } else {
      /* or insert into approriate child */
      int i = getOctantId(box, triangle.boundingBox);
      if (i != -1) {
        add(node->childs.at(i), depth + 1, getChildOctant(box, i), triangle);
      }
    }
  }
}

void TriangleOctree::split(std::shared_ptr<TriangleOctreeNode> node, BoundingBox3D box) {
  if (!isLeaf(node)) {
    Logger::log(1, "%s error: only leafs can be splitted\n", __FUNCTION__);
    return;
  }

  for (auto& child : node->childs) {
    child = std::make_shared<TriangleOctreeNode>();
  }

  std::vector<MeshTriangle> newTriangles{};

  for (const auto& triangle : node->triangles) {
    int intersectingChildren = 0;
    /* check how many children we would intersect */
    for (int i = 0; i < node->childs.size(); ++i) {
      BoundingBox3D childBox = getChildOctant(box, i);
      if (childBox.intersects(triangle.boundingBox)) {
        intersectingChildren++;
      }
    }
    /* keep into node if we would have multiple children  */
    if (intersectingChildren > 1) {
      newTriangles.emplace_back(triangle);
    } else {
      /* or insert into approriate child */
      int i = getOctantId(box, triangle.boundingBox);
      if (i != -1) {
        node->childs.at(i)->triangles.emplace_back(triangle);
      }
    }
  }

  node->triangles = std::move(newTriangles);
}

std::vector<MeshTriangle> TriangleOctree::query(BoundingBox3D box) {
  std::vector<MeshTriangle> values;
  values = query(mRootNode, mRootBoundingBox, box);
  return values;
}

std::vector<MeshTriangle> TriangleOctree::query(std::shared_ptr<TriangleOctreeNode> node, BoundingBox3D box, BoundingBox3D queryBox) {
  std::vector<MeshTriangle> values;

  for (const auto& tri : node->triangles) {
    if (queryBox.intersects(tri.boundingBox)) {
      values.emplace_back(tri);
    }
  }

  if (!isLeaf(node)) {
    for (int i = 0; i < node->childs.size(); ++i) {
      BoundingBox3D childBox = getChildOctant(box, i);
      if (queryBox.intersects(childBox)) {
        std::vector<MeshTriangle> childValues = query(node->childs.at(i), childBox, queryBox);
        values.insert(values.end(), childValues.begin(), childValues.end());
      }
    }
  }
  return values;
}

void TriangleOctree::clear() {
  mRootNode.reset();
  mRootNode = std::make_shared<TriangleOctreeNode>();
}

std::vector<BoundingBox3D> TriangleOctree::getTreeBoxes() {
  std::vector<BoundingBox3D> values;
  values = getTreeBoxes(mRootNode, mRootBoundingBox);
  return values;
}

std::vector<BoundingBox3D> TriangleOctree::getTreeBoxes(std::shared_ptr<TriangleOctreeNode> node, BoundingBox3D box) {
  std::vector<BoundingBox3D> values;

  if (isLeaf(node)) {
    values.emplace_back(box);
  } else {
    for (int i = 0; i < node->childs.size(); ++i) {
      BoundingBox3D childBox = getChildOctant(box, i);
      std::vector<BoundingBox3D> childValues = getTreeBoxes(node->childs.at(i), childBox);
      values.insert(values.end(), childValues.begin(), childValues.end());
    }
  }
  return values;
}
