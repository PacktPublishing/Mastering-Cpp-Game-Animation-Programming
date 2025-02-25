#include <algorithm>

#include "Quadtree.h"
#include "Logger.h"

QuadTree::QuadTree(std::shared_ptr<BoundingBox2D> rootBox, int threshold, int maxDepth) :
   mRootBoundingBox(*rootBox), mThreshold(threshold), mMaxDepth(maxDepth) {
  mRootNode = std::make_shared<QuadTreeNode>();
}

bool QuadTree::isLeaf(const std::shared_ptr<QuadTreeNode> node) {
  return node && !node->childs[0];
}

BoundingBox2D QuadTree::getChildQuadrant(BoundingBox2D parentBox, int quadrantId) {
  glm::vec2 origin = parentBox.getTopLeft();
  glm::vec2 childSize = parentBox.getSize() / 2.0f;

  /* Quadrants:
   * +---+---+  +----+----+
   * | 0 | 1 |  | NW | NE |
   * +---+---+  +----+----+
   * | 2 | 3 |  | SW | SE |
   * +---+---+  +----+----+
   */

  switch(quadrantId) {
    case 0:
      return BoundingBox2D(origin, childSize);
      break;
    case 1:
      return BoundingBox2D(glm::vec2(origin.x + childSize.x, origin.y), childSize);
      break;
    case 2:
      return BoundingBox2D(glm::vec2(origin.x, origin.y + childSize.y), childSize);
      break;
    case 3:
      return BoundingBox2D(origin + childSize, childSize);
      break;
    default:
      Logger::log(1, "%s error: invalid quadrant id %i\n", __FUNCTION__, quadrantId);
      return BoundingBox2D();
  }
}

int QuadTree::getQuadrantId(BoundingBox2D nodeBox, BoundingBox2D valueBox) {
  glm::vec2 center = nodeBox.getCenter();

  /* West */
  if (valueBox.getRight() < center.x) {
    if (valueBox.getBottom() < center.y) {
      /* NW */
      return 0;
    } else if (valueBox.getTopLeft().y >= center.y) {
      /* SW */
      return 2;
    } else {
      /* not found */
      return -1;
    }
  /* East */
  } else if (valueBox.getTopLeft().x >= center.x) {
    if (valueBox.getBottom() < center.y) {
      /* NE */
      return 1;
    } else if ( valueBox.getTopLeft().y >= center.y) {
      /* SE */
      return 3;
    } else {
      /* not found */
      return -1;
    }
  } else {
    /* not found */
    return -1;
  }
}

void QuadTree::add(int instanceId) {
  /* do not add instance when outside of quadtree */
  if (!mRootBoundingBox.intersects(mInstanceGetBoundingBox2DCallbackFunction(instanceId))) {
    return;
  }

  add(mRootNode, 0, mRootBoundingBox, instanceId);
}

void QuadTree::add(std::shared_ptr<QuadTreeNode> node, int depth, BoundingBox2D box, int instanceId) {
  if (!box.intersects(mInstanceGetBoundingBox2DCallbackFunction(instanceId))) {
    Logger::log(1, "%s error: current quadtree node bounding box does not contain the bounding box of instance %i \n", __FUNCTION__, instanceId);
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
    int i = getQuadrantId(box, mInstanceGetBoundingBox2DCallbackFunction(instanceId));
    if (i != -1) {
      add(node->childs.at(i), depth + 1, getChildQuadrant(box, i), instanceId);
    } else {
      node->instancIds.emplace_back(instanceId);
    }
  }
}

void QuadTree::split(std::shared_ptr<QuadTreeNode> node, BoundingBox2D box) {
  if (!isLeaf(node)) {
    Logger::log(1, "%s error: only leafs can be splitted\n", __FUNCTION__);
    return;
  }

  for (auto& child : node->childs) {
    child = std::make_shared<QuadTreeNode>();
  }

  std::vector<int> newInstanceIds{};

  for (const auto& instanceId : node->instancIds) {
    int i = getQuadrantId(box, mInstanceGetBoundingBox2DCallbackFunction(instanceId));
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

void QuadTree::remove(int instanceId) {
  remove(mRootNode, mRootBoundingBox, instanceId);
}

bool QuadTree::remove(std::shared_ptr<QuadTreeNode> node, BoundingBox2D box, int instanceId) {
  if (!box.intersects(mInstanceGetBoundingBox2DCallbackFunction(instanceId))) {
    Logger::log(1, "%s error: current quadtree node bounding box does not contain the bounding box of instance %i \n", __FUNCTION__, instanceId);
    return false;
  }

  if (isLeaf(node)) {
    removeInstance(node, instanceId);
    return true;
  } else {
    int i = getQuadrantId(box, mInstanceGetBoundingBox2DCallbackFunction(instanceId));
    if (i != -1) {
      if (remove(node->childs[i], getChildQuadrant(box, i), instanceId)) {
        return tryMerge(node);
      }
    } else {
      removeInstance(node, instanceId);
    }
    return false;
  }
}

void QuadTree::removeInstance(std::shared_ptr<QuadTreeNode> node, int instanceId) {
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

bool QuadTree::tryMerge(std::shared_ptr<QuadTreeNode> node) {
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

void QuadTree::update(int instanceId) {
  remove(instanceId);
  add(instanceId);
}

std::set<int> QuadTree::query(BoundingBox2D box) {
  std::vector<int> values;
  values = query(mRootNode, mRootBoundingBox, box);
  return std::set<int>(values.begin(), values.end());
}

std::vector<int> QuadTree::query(std::shared_ptr<QuadTreeNode> node, BoundingBox2D box, BoundingBox2D queryBox) {
  std::vector<int> values;

  for (const auto& instanceId : node->instancIds) {
    if (queryBox.intersects(mInstanceGetBoundingBox2DCallbackFunction(instanceId))) {
      values.emplace_back(instanceId);
    }
  }

  if (!isLeaf(node)) {
    for (int i = 0; i < node->childs.size(); ++i) {
      BoundingBox2D childBox = getChildQuadrant(box, i);
      if (queryBox.intersects(childBox)) {
        std::vector<int> childValues = query(node->childs.at(i), childBox, queryBox);
        values.insert(values.end(), childValues.begin(), childValues.end());
      }
    }
  }
  return values;
}

void QuadTree::clear() {
  mRootNode.reset();
  mRootNode = std::make_shared<QuadTreeNode>();
}

std::set<std::pair<int, int>> QuadTree::findAllIntersections() {
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

std::set<std::pair<int, int>> QuadTree::findAllIntersections(std::shared_ptr<QuadTreeNode> node) {
  std::set<std::pair<int, int>> values;

  for (int i = 0; i < node->instancIds.size(); ++i) {
    for (int j = 0; j < i; ++j) {
      if (mInstanceGetBoundingBox2DCallbackFunction(node->instancIds.at(i)).intersects(mInstanceGetBoundingBox2DCallbackFunction(node->instancIds.at(j)))) {
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

std::set<std::pair<int, int>> QuadTree::findIntersectionsInDescendants(std::shared_ptr<QuadTreeNode> node, int instanceId) {
  std::set<std::pair<int, int>> values;

  for (const auto& other : node->instancIds) {
    if (mInstanceGetBoundingBox2DCallbackFunction(instanceId).intersects(mInstanceGetBoundingBox2DCallbackFunction(other))) {
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

std::vector<BoundingBox2D> QuadTree::getTreeBoxes() {
  std::vector<BoundingBox2D> values;
  values = getTreeBoxes(mRootNode, mRootBoundingBox);
  return values;
}

std::vector<BoundingBox2D> QuadTree::getTreeBoxes(std::shared_ptr<QuadTreeNode> node, BoundingBox2D box) {
  std::vector<BoundingBox2D> values;

  if (isLeaf(node)) {
    values.emplace_back(box);
  } else {
    for (int i = 0; i < node->childs.size(); ++i) {
      BoundingBox2D childBox = getChildQuadrant(box, i);
      std::vector<BoundingBox2D> childValues = getTreeBoxes(node->childs.at(i), childBox);
      values.insert(values.end(), childValues.begin(), childValues.end());
    }
  }
  return values;
}
