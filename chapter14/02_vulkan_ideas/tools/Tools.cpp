#include <iostream>
#include <fstream>
#include <cerrno>  // errno
#include <cstring> // strerror()
#include <limits>
#include <filesystem>
#include <glm/gtx/matrix_decompose.hpp>

#include "Tools.h"
#include "Logger.h"

std::string Tools::getFilenameExt(std::string filename) {
  size_t pos = filename.find_last_of('.');
  if (pos != std::string::npos) {
    return filename.substr(pos + 1);
  }
  return std::string();
}

std::string Tools::loadFileToString(std::string fileName) {
  std::ifstream inFile(fileName, std::ios::binary);
  std::string str;

  if (inFile.is_open()) {
    str.clear();
    // allocate string data (no slower realloc)
    inFile.seekg(0, std::ios::end);
    str.reserve(inFile.tellg());
    inFile.seekg(0, std::ios::beg);

    str.assign((std::istreambuf_iterator<char>(inFile)),
                std::istreambuf_iterator<char>());
    inFile.close();
  } else {
    Logger::log(1, "%s error: could not open file %s\n", __FUNCTION__, fileName.c_str());
    Logger::log(1, "%s error: system says '%s'\n", __FUNCTION__, strerror(errno));
    return std::string();
  }

  if (inFile.bad() || inFile.fail()) {
    Logger::log(1, "%s error: error while reading file %s\n", __FUNCTION__, fileName.c_str());
    inFile.close();
    return std::string();
  }

  inFile.close();
  Logger::log(1, "%s: file %s successfully read to string\n", __FUNCTION__, fileName.c_str());
  return str;
}


/* transposes the matrix from Assimp to GL format */
glm::mat4 Tools::convertAiToGLM(aiMatrix4x4 inMat) {
  return {
    inMat.a1, inMat.b1, inMat.c1, inMat.d1,
    inMat.a2, inMat.b2, inMat.c2, inMat.d2,
    inMat.a3, inMat.b3, inMat.c3, inMat.d3,
    inMat.a4, inMat.b4, inMat.c4, inMat.d4
  };
}

/* Möller-Trumbore ray-triangle intersection */
std::optional<glm::vec3> Tools::rayTriangleIntersection(glm::vec3 rayOrigin, glm::vec3 rayDirection, MeshTriangle triangle) {
  constexpr float epsilon = std::numeric_limits<float>::epsilon();

  glm::vec3 edge1 = triangle.points.at(1) - triangle.points.at(0);
  glm::vec3 edge2 = triangle.points.at(2) - triangle.points.at(0);

  glm::vec3 rayCrossEdge2 = glm::cross(rayDirection, edge2);

  float inPlaneDeterminant = glm::dot(edge1, rayCrossEdge2);

  /* ray is (almost) parallel to triangle */
  if (std::fabs(inPlaneDeterminant) < epsilon) {
    return {};
  }

  float inverseInPlaneDeterminant = 1.0f / inPlaneDeterminant;

  glm::vec3 rayOriginDistFromPoint0 = rayOrigin - triangle.points.at(0);

  /* calculate U and check bounds */
  float barycentricU = inverseInPlaneDeterminant * glm::dot(rayOriginDistFromPoint0, rayCrossEdge2);
  if (barycentricU < 0.0f || barycentricU > 1.0f) {
    return {};
  }

  glm::vec3 rayOriginDistCrossEdge1 = glm::cross(rayOriginDistFromPoint0, edge1);

  /* calculate V and check bounds */
  float barycentricV = inverseInPlaneDeterminant * glm::dot(rayDirection, rayOriginDistCrossEdge1);
  if (barycentricV < 0.0f || barycentricU + barycentricV > 1.0f) {
    return {};
  }

  /* calculate t */
  float intersectionPointScale = inverseInPlaneDeterminant * glm::dot(edge2, rayOriginDistCrossEdge1);

  if (intersectionPointScale <= epsilon) {
    return {};
  }

  return glm::vec3(rayOrigin + rayDirection * intersectionPointScale);
}

glm::vec4 Tools::extractGlobalPosition(glm::mat4 nodeMatrix) {
  glm::quat orientation;
  glm::vec3 scale;
  glm::vec3 translation;
  glm::vec3 skew;
  glm::vec4 perspective;

  if (!glm::decompose(nodeMatrix, scale, orientation, translation, skew, perspective)) {
    Logger::log(1, "%s error: could not decompose matrix\n", __FUNCTION__);
    return glm::vec4(0.0f);
  }

  return glm::vec4(translation, 0.0f);
}

glm::quat Tools::extractGlobalRotation(glm::mat4 nodeMatrix) {
  glm::quat orientation;
  glm::vec3 scale;
  glm::vec3 translation;
  glm::vec3 skew;
  glm::vec4 perspective;

  if (!glm::decompose(nodeMatrix, scale, orientation, translation, skew, perspective)) {
    Logger::log(1, "%s error: could not decompose matrix\n", __FUNCTION__);
    return glm::identity<glm::quat>();
  }

  return glm::inverse(orientation);
}

std::vector<std::string> Tools::getDirectoryContent(std::string path, std::string extension) {
  std::vector<std::string> files;

  for (const auto &entry : std::filesystem::directory_iterator(path)) {
    std::string filename = entry.path().string();
    if (filename != "." && filename != ".." && (filename.find(extension) == (filename.size() - extension.size()) )) {
      Logger::log(1, "%s: found music title '%s''\n", __FUNCTION__, filename.c_str());
      files.push_back(filename);
    }
  }

  Logger::log(1, "%s: successfully read content of dir '%s', found %i title%s\n", __FUNCTION__, path.c_str(), files.size(), files.size() == 1 ? "" : "s");

  return files;
}
