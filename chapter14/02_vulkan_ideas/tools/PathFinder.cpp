#include "PathFinder.h"

#include <queue>

#include "Logger.h"

void PathFinder::generateGroundTriangles(VkRenderData& renderData, std::shared_ptr<TriangleOctree> octree, BoundingBox3D worldbox) {
  mNavTriangles.clear();

  mLevelGroundMesh = std::make_shared<VkLineMesh>();

  /* get all triangles frm octree */
  std::vector<MeshTriangle> levelTris = octree->query(worldbox);
  Logger::log(1, "%s: level has %i triangles \n", __FUNCTION__, levelTris.size());

  /* find all triangles that face upwards */
  std::vector<MeshTriangle> groundTris{};
  NavTriangle navTri;
  for (const auto& tri: levelTris) {
    if (glm::dot(tri.normal, glm::vec3(0.0f, 1.0f, 0.0f)) >= std::cos(glm::radians(renderData.rdMaxLevelGroundSlopeAngle))) {
      groundTris.emplace_back(tri);

      navTri.points = tri.points;
      navTri.normal = tri.normal;
      navTri.index = tri.index;
      navTri.center = (tri.points.at(0) + tri.points.at(1) + tri.points.at(2)) / 3.0f;

      mNavTriangles.insert(std::make_pair(tri.index, navTri));
    }
  }

  Logger::log(1, "%s: level has %i (%i) possible ground triangles\n", __FUNCTION__, groundTris.size(), mNavTriangles.size());

  VkLineVertex vert;
  vert.color = glm::vec3(0.0f, 0.2f, 0.8f);

  for (const auto& tri : groundTris) {
    BoundingBox3D triBox = tri.boundingBox;

    /* extend query box by stair height in position and size (look up and down) */
    glm::vec3 boxPos = glm::vec3(triBox.getFrontTopLeft().x, triBox.getFrontTopLeft().y - renderData.rdMaxStairstepHeight, triBox.getFrontTopLeft().z);
    glm::vec3 boxSize = glm::vec3(triBox.getSize().x, triBox.getSize().y + renderData.rdMaxStairstepHeight * 2, triBox.getSize().z);
    BoundingBox3D queryBox = BoundingBox3D(boxPos, boxSize);

    std::vector<MeshTriangle> nearbyTris = octree->query(queryBox);

    NavTriangle& navTri = mNavTriangles.at(tri.index);
    for (const auto& peer : nearbyTris) {
      /* ignore myself */
      if (tri.index == peer.index) {
        continue;
      }

      /* ignore if no ground triangle */
      if (glm::dot(peer.normal, glm::vec3(0.0f, 1.0f, 0.0f)) < std::cos(glm::radians(renderData.rdMaxLevelGroundSlopeAngle))) {
        continue;
      }

      if (mNavTriangles.count(peer.index) == 0) {
        Logger::log(1, "%s error: peer triangle %i for triangle %i not found\n", __FUNCTION__, peer.index, tri.index);
        continue;
      }

      NavTriangle& peerNavTri = mNavTriangles.at(peer.index);
      for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
          /* get distance of triangle points from peer sides, and of peer points from triangle sides */
          glm::vec3 pointToPeerLine = glm::cross(tri.points.at(j) - peer. points.at(i), tri.points.at(j) - peer.points.at((i + 1) % 3));
          float pointDistance = glm::length(pointToPeerLine) / peer.edgeLengths.at(i);
          glm::vec3 peerPointToTriLine = glm::cross(peer.points.at(j) - tri. points.at(i), peer.points.at(j) - tri.points.at((i + 1) % 3));
          float peerPointDistance = glm::length(peerPointToTriLine) / tri.edgeLengths.at(i);

          if ((pointDistance < 0.01f || peerPointDistance < 0.01f)) {
            navTri.neighborTris.insert(peerNavTri.index);
          }

          /* also add ground triangles which have less than step width a differences in Y direction */
          if (std::fabs(tri.points.at(j).y - peer.points.at(i).y) < renderData.rdMaxStairstepHeight &&
              std::fabs(peer.points.at(j).y - tri.points.at(i).y) < renderData.rdMaxStairstepHeight) {
            navTri.neighborTris.insert(peerNavTri.index);
          }
        }
      }
    }

    vert.position = tri.points.at(0) + tri.normal * 0.1f;
    mLevelGroundMesh->vertices.emplace_back(vert);
    vert.position = tri.points.at(1) + tri.normal * 0.1f;
    mLevelGroundMesh->vertices.emplace_back(vert);
    vert.position = tri.points.at(2) + tri.normal * 0.1f;
    mLevelGroundMesh->vertices.emplace_back(vert);
  }
}

std::vector<int> PathFinder::getGroundTriangleNeighbors(int groundTriIndex) {
  if (mNavTriangles.count(groundTriIndex) ==  0) {
    return std::vector<int>{};
  }

  std::unordered_set neighborIndices = mNavTriangles.at(groundTriIndex).neighborTris;

  std::vector<int> neighbors{};
  for (const auto index : neighborIndices) {
    neighbors.push_back(index);
  }

  return neighbors;
}

std::vector<int> PathFinder::findPath(int startTriIndex, int targetTriIndex) {
  if (mNavTriangles.count(targetTriIndex) == 0) {
    Logger::log(1, "%s error: target triangle id %i not found\n", __FUNCTION__, targetTriIndex);
    return std::vector<int>{};
  }

  NavTriangle targetTri = mNavTriangles.at(targetTriIndex);
  glm::vec3 targetPoint = targetTri.center;

  if (mNavTriangles.count(startTriIndex) == 0) {
    Logger::log(1, "%s error: source triangle id %i not found\n", __FUNCTION__, startTriIndex);
    return std::vector<int>{};
  }

  NavTriangle startTri = mNavTriangles.at(startTriIndex);
  glm::vec3 startPoint = startTri.center;

  std::unordered_set<int> navOpenList{};
  std::unordered_set<int> navClosedList{};
  std::unordered_map<int, NavData> navPoints{};

  int currentIndex = startTriIndex;

  /* insert start data */
  NavData navStartPoint{};
  navStartPoint.triIndex = startTriIndex;
  navStartPoint.prevTriIndex = -1;
  navStartPoint.distanceFromSource = 0;
  navStartPoint.heuristicToDest = glm::distance(startPoint, targetPoint);
  navStartPoint.distanceToDest = navStartPoint.distanceFromSource + navStartPoint.heuristicToDest;
  navPoints.emplace(std::make_pair(startTriIndex, navStartPoint));
  navOpenList.insert(startTriIndex);

  while (currentIndex != targetTriIndex) {
    NavTriangle currentTri = mNavTriangles.at(currentIndex);
    glm::vec3 currentTriPoint = currentTri.center;
    std::unordered_set<int> neighborTris = currentTri.neighborTris;

    for (const auto& navTriIndex : neighborTris) {
      NavTriangle navTri = mNavTriangles.at(navTriIndex);
      glm::vec3 navTriPoint = navTri.center;

      if (navClosedList.count(navTriIndex) == 0) {
        if (navOpenList.count(navTriIndex) == 0) {
          /* insert new node */
          navOpenList.insert(navTriIndex);

          NavData navPoint{};

          navPoint.triIndex = navTriIndex;
          navPoint.prevTriIndex = currentIndex;

          NavData prevNavPoint = navPoints.at(navPoint.prevTriIndex);
          navPoint.distanceFromSource = prevNavPoint.distanceFromSource + glm::distance(currentTriPoint, navTriPoint);
          navPoint.heuristicToDest = glm::distance(navTriPoint, targetPoint);
          navPoint.distanceToDest = navPoint.distanceFromSource + navPoint.heuristicToDest;

          navPoints.emplace(std::make_pair(navTriIndex, navPoint));
        } else {
          /* update triangle if in open list and already visited - and if path with new prev triangle is shorter */
          NavData& navPoint = navPoints.at(navTriIndex);

          NavData possibleNewPrevNavPoint = navPoints.at(currentIndex);
          float newDistanceFromSource = possibleNewPrevNavPoint.distanceFromSource + glm::distance(currentTriPoint, navTriPoint);
          float newDistanceToDest = newDistanceFromSource + navPoint.heuristicToDest;
          if (newDistanceToDest < navPoint.distanceToDest) {
            navPoint.prevTriIndex = currentIndex;
            navPoint.distanceFromSource = newDistanceFromSource;
            navPoint.distanceToDest = newDistanceToDest;
          }
        }
      }
    }
    navClosedList.insert(currentIndex);

    if (navOpenList.empty()) {
      Logger::log(1, "%s error: nav open list empty while searching for neighbor to %i\n", __FUNCTION__, currentIndex);
      return std::vector<int>{};
    }

    /* create comparator for min prio queue  */
    auto cmp = [](NavData left, NavData right) { return left.distanceToDest > right.distanceToDest; };

    /* create prio queue to find lowest distance to destination  */
    std::priority_queue<NavData, std::vector<NavData>, decltype(cmp)> naviDataQueue(cmp);
    for (const auto& navTriIndex : navOpenList) {
      NavData navPoint = navPoints.at(navTriIndex);
      naviDataQueue.push(navPoint);
    }

    NavData nextPointToDest{};
    nextPointToDest = naviDataQueue.top();
    currentIndex = nextPointToDest.triIndex;

    /* remove from open list */
    navOpenList.erase(currentIndex);
  }

  std::vector<int> foundPath{};
  /* target point is currentIndex (end condition of while() loop)  */
  foundPath.emplace_back(currentIndex);

  /* walk backwards */
  NavData navPoint = navPoints.at(currentIndex);
  while (navPoint.prevTriIndex != -1) {
    foundPath.emplace_back(navPoint.prevTriIndex);
    navPoint = navPoints.at(navPoint.prevTriIndex);
  }
  /* start point is added in while() loop as last parent */
  //foundPath.emplace_back(startTriIndex);

  /* turn vector around, start to end */
  std::reverse(foundPath.begin(), foundPath.end());

  return foundPath;
}

std::shared_ptr<VkLineMesh> PathFinder::getGroundLevelMesh() {
  return mLevelGroundMesh;
}

glm::vec3 PathFinder::getTriangleCenter(int index) {
  if (mNavTriangles.count(index) == 0) {
    return glm::vec3(0.0f);
  }

  return mNavTriangles.at(index).center;
}


std::shared_ptr<VkLineMesh> PathFinder::getAsLineMesh(std::vector<int> indices, glm::vec3 color, glm::vec3 offset) {
  std::shared_ptr<VkLineMesh> pointMesh = std::make_shared<VkLineMesh>();

  /* we need at least two vertices to draw a line */
  if (indices.size() < 2) {
    return pointMesh;
  }

  VkLineVertex vert{};
  vert.color = color;

  for (int i = 0; i < indices.size() - 1; ++i) {
    if (mNavTriangles.count(indices.at(i)) == 0 || mNavTriangles.count(indices.at(i + 1)) == 0) {
      continue;
    }

    NavTriangle tri = mNavTriangles.at(indices.at(i));
    vert.position = tri.center + tri.normal * offset;
    pointMesh->vertices.emplace_back(vert);

    tri = mNavTriangles.at(indices.at(i + 1));
    vert.position = tri.center + tri.normal * offset;
    pointMesh->vertices.emplace_back(vert);
  }

  return pointMesh;
}

std::shared_ptr<VkLineMesh> PathFinder::getAsTriangleMesh(std::vector<int> indices, glm::vec3 color, glm::vec3 normalColor, glm::vec3 offset) {
  std::shared_ptr<VkLineMesh> pointMesh = std::make_shared<VkLineMesh>();

  VkLineVertex vert;
  VkLineVertex normalVert;

  for (const auto index : indices) {
    NavTriangle tri = mNavTriangles.at(index);

    vert.color = color;
    /* move wireframe overdraw a bit above the planes */
    vert.position = tri.points.at(0) + tri.normal * offset;
    pointMesh->vertices.push_back(vert);
    vert.position = tri.points.at(1) + tri.normal * offset;
    pointMesh->vertices.push_back(vert);

    vert.position = tri.points.at(1) + tri.normal * offset;
    pointMesh->vertices.push_back(vert);
    vert.position = tri.points.at(2) + tri.normal * offset;
    pointMesh->vertices.push_back(vert);

    vert.position = tri.points.at(2) + tri.normal * offset;
    pointMesh->vertices.push_back(vert);
    vert.position = tri.points.at(0) + tri.normal * offset;
    pointMesh->vertices.push_back(vert);

    /* draw normal vector in the middle of the triangle */
    normalVert.color = normalColor;
    glm::vec3 normalPos = tri.center;
    normalVert.position = normalPos;
    pointMesh->vertices.push_back(normalVert);
    normalVert.position = normalPos + tri.normal;
    pointMesh->vertices.push_back(normalVert);
  }

  return pointMesh;
}
