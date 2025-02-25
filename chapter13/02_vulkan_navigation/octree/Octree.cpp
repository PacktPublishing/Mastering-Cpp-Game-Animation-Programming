#include <algorithm>

#include "Octree.h"
#include "Logger.h"

Octree::Octree(std::shared_ptr<BoundingBox3D> rootBox, int threshold, int maxDepth) :
   mRootBoundingBox(*rootBox), mThreshold(threshold), mMaxDepth(maxDepth) {
  mRootNode = std::make_shared<OctreeNode>();
}

bool Octree::isLeaf(const std::shared_ptr<OctreeNode> node) {
  return node && !node->childs[0];
}

BoundingBox3D Octree::getChildOctant(BoundingBox3D parentBox, int octantId) {
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

int Octree::getOctantId(BoundingBox3D nodeBox, BoundingBox3D valueBox) {
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
  } else {
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
  }
}

void Octree::add(int instanceId) {
  /* do not add instance when outside of octree */
  if (!mRootBoundingBox.intersects(mInstanceGetBoundingBoxCallbackFunction(instanceId))) {
    return;
  }

  add(mRootNode, 0, mRootBoundingBox, instanceId);
}

void Octree::add(std::shared_ptr<OctreeNode> node, int depth, BoundingBox3D box, int instanceId) {
  if (!box.intersects(mInstanceGetBoundingBoxCallbackFunction(instanceId))) {
    Logger::log(1, "%s error: current octree node bounding box does not contain the bounding box of instance %i \n", __FUNCTION__, instanceId);
    return;
  }

  if (isLeaf(node)) {
    /* insert into node if possible */
    if (depth >= mMaxDepth || node->instancIds.size() < mThreshold) {
      node->instancIds.emplace_back(instanceId);
    } else {
      split(node, box);
      add(node, depth, box, instanceId);
    }
  } else {
    int i = getOctantId(box, mInstanceGetBoundingBoxCallbackFunction(instanceId));
    if (i != -1) {
      add(node->childs.at(i), depth + 1, getChildOctant(box, i), instanceId);
    } else {
      node->instancIds.emplace_back(instanceId);
    }
  }
}

void Octree::split(std::shared_ptr<OctreeNode> node, BoundingBox3D box) {
  if (!isLeaf(node)) {
    Logger::log(1, "%s error: only leafs can be splitted\n", __FUNCTION__);
    return;
  }

  for (auto& child : node->childs) {
    child = std::make_shared<OctreeNode>();
  }

  std::vector<int> newInstanceIds{};

  for (const auto& instanceId : node->instancIds) {
    int i = getOctantId(box, mInstanceGetBoundingBoxCallbackFunction(instanceId));
    if (i != -1) {
      /* found child, store in child ids */
      node->childs[i]->instancIds.emplace_back(instanceId);
    } else {
      /* not found, store in parent */
      newInstanceIds.emplace_back(instanceId);
    }
  }
  node->instancIds = std::move(newInstanceIds);
}

void Octree::remove(int instanceId) {
  remove(mRootNode, mRootBoundingBox, instanceId);
}

bool Octree::remove(std::shared_ptr<OctreeNode> node, BoundingBox3D box, int instanceId) {
  if (!box.intersects(mInstanceGetBoundingBoxCallbackFunction(instanceId))) {
    Logger::log(1, "%s error: current octree node bounding box does not contain the bounding box of instance %i \n", __FUNCTION__, instanceId);
    return false;
  }

  if (isLeaf(node)) {
    removeInstance(node, instanceId);
    return true;
  } else {
    int i = getOctantId(box, mInstanceGetBoundingBoxCallbackFunction(instanceId));
    if (i != -1) {
      if (remove(node->childs[i], getChildOctant(box, i), instanceId)) {
        return tryMerge(node);
      }
    } else {
      removeInstance(node, instanceId);
    }
    return false;
  }
}

void Octree::removeInstance(std::shared_ptr<OctreeNode> node, int instanceId) {
  auto it = std::find_if(std::begin(node->instancIds), std::end(node->instancIds),
    [&instanceId](const auto& rhs) { return instanceId == rhs; });
  if (it == std::end(node->instancIds)) {
    Logger::log(1, "%s error: could not remove not existing instance with id %i\n", __FUNCTION__, instanceId);
    return;
  }
  // Swap with the last element and pop back
  *it = std::move(node->instancIds.back());
  node->instancIds.pop_back();
}

bool Octree::tryMerge(std::shared_ptr<OctreeNode> node) {
  int numInstanceIds = node->instancIds.size();

  for (const auto& child : node->childs) {
    if (!isLeaf(child)) {
      return false;
    }
    numInstanceIds += child->instancIds.size();
  }

  if (numInstanceIds <= mThreshold) {
    for (const auto& child : node->childs) {
      node->instancIds.insert(node->instancIds.end(), child->instancIds.begin(), child->instancIds.end());
    }
    /* remove the childs */
    for (auto& child : node->childs) {
      child.reset();
    }
    return true;
  } else {
    return false;
  }
}

void Octree::update(int instanceId) {
  remove(instanceId);
  add(instanceId);
}

std::set<int> Octree::query(BoundingBox3D box) {
  std::vector<int> values;
  values = query(mRootNode, mRootBoundingBox, box);
  return std::set<int>(values.begin(), values.end());
}

std::vector<int> Octree::query(std::shared_ptr<OctreeNode> node, BoundingBox3D box, BoundingBox3D queryBox) {
  std::vector<int> values;

  for (const auto& instanceId : node->instancIds) {
    if (queryBox.intersects(mInstanceGetBoundingBoxCallbackFunction(instanceId))) {
      values.emplace_back(instanceId);
    }
  }

  if (!isLeaf(node)) {
    for (int i = 0; i < node->childs.size(); ++i) {
      BoundingBox3D childBox = getChildOctant(box, i);
      if (queryBox.intersects(childBox)) {
        std::vector<int> childValues = query(node->childs.at(i), childBox, queryBox);
        values.insert(values.end(), childValues.begin(), childValues.end());
      }
    }
  }
  return values;
}

void Octree::clear() {
  mRootNode.reset();
  mRootNode = std::make_shared<OctreeNode>();
}

std::set<std::pair<int, int>> Octree::findAllIntersections() {
  std::set<std::pair<int, int>> values;
  values = findAllIntersections(mRootNode);

  for (auto it = values.begin(); it != values.end(); ) {
    auto reversePair = std::make_pair((*it).second, (*it).first);
    if (values.count(reversePair) > 0) {
      values.erase(it++);
    } else {
      ++it;
    }
  }

  return values;
}

std::set<std::pair<int, int>> Octree::findAllIntersections(std::shared_ptr<OctreeNode> node) {
  std::set<std::pair<int, int>> values;

  for (int i = 0; i < node->instancIds.size(); ++i) {
    for (int j = 0; j < i; ++j) {
      if (mInstanceGetBoundingBoxCallbackFunction(node->instancIds.at(i)).intersects(mInstanceGetBoundingBoxCallbackFunction(node->instancIds.at(j)))) {
        values.insert({node->instancIds[i], node->instancIds[j]});
      }
    }
  }

  if (!isLeaf(node)) {
    for (const auto& child : node->childs) {
      for (const auto& value : node->instancIds) {
        std::set<std::pair<int, int>> childValues;
        childValues = findIntersectionsInDescendants(child, value);
        values.insert(childValues.begin(), childValues.end());
      }
    }
    for (const auto& child : node->childs) {
      std::set<std::pair<int, int>> childValues;
      childValues = findAllIntersections(child);
      values.insert(childValues.begin(), childValues.end());
    }
  }
  return values;
}

std::set<std::pair<int, int>> Octree::findIntersectionsInDescendants(std::shared_ptr<OctreeNode> node, int instanceId) {
  std::set<std::pair<int, int>> values;

  for (const auto& other : node->instancIds) {
    if (mInstanceGetBoundingBoxCallbackFunction(instanceId).intersects(mInstanceGetBoundingBoxCallbackFunction(other))) {
      values.insert({instanceId, other});
    }
  }

  if (!isLeaf(node)) {
    for (const auto& child : node->childs) {
      std::set<std::pair<int, int>> childValues;
      childValues = findIntersectionsInDescendants(child, instanceId);
      values.insert(childValues.begin(), childValues.end());
    }
  }
  return values;
}

std::vector<BoundingBox3D> Octree::getTreeBoxes() {
  std::vector<BoundingBox3D> values;
  values = getTreeBoxes(mRootNode, mRootBoundingBox);
  return values;
}

std::vector<BoundingBox3D> Octree::getTreeBoxes(std::shared_ptr<OctreeNode> node, BoundingBox3D box) {
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
