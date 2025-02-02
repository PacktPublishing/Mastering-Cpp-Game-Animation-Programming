#include "RotationArrowsModel.h"
#include "Logger.h"

VkLineMesh RotationArrowsModel::getVertexData() {
  if (mVertexData.vertices.empty()) {
    init();
  }
  return mVertexData;
}

void RotationArrowsModel::init() {
  std::vector<glm::vec2> circleValues;
  for (float i = -165.0f; i <= 166.0f; i += 15.0f) {
    float xCoord = std::sin(glm::radians(i));
    float yCoord = std::cos(glm::radians(i));
    circleValues.emplace_back(glm::vec2(xCoord, yCoord));
  }

  int endIndex = circleValues.size() - 2;
  VkLineVertex startVert;
  VkLineVertex endVert;

  /* X axis - red */
  for (int i = 0; i <= endIndex; ++i) {
    startVert.position = glm::vec3(0.0f, circleValues.at(i).x, circleValues.at(i).y);
    startVert.color = glm::vec3(0.8f, 0.0f, 0.0f);
    endVert.position = glm::vec3(0.0f, circleValues.at(i + 1).x, circleValues.at(i+ 1).y);
    endVert.color = glm::vec3(0.8f, 0.0f, 0.0f);

    mVertexData.vertices.emplace_back(startVert);
    mVertexData.vertices.emplace_back(endVert);
  }

  /* arrow lines */
  startVert.position = glm::vec3(0.0f, circleValues.at(endIndex + 1).x, circleValues.at(endIndex + 1).y);
  startVert.color = glm::vec3(0.8f, 0.0f, 0.0f);
  endVert.position = glm::vec3(0.0f, circleValues.at(endIndex).x * 1.1f, circleValues.at(endIndex).y * 1.1f);
  endVert.color = glm::vec3(0.8f, 0.0f, 0.0f);

  mVertexData.vertices.emplace_back(startVert);
  mVertexData.vertices.emplace_back(endVert);

  endVert.position = glm::vec3(0.0f, circleValues.at(endIndex).x / 1.1f, circleValues.at(endIndex).y / 1.1f);
  endVert.color = glm::vec3(0.8f, 0.0f, 0.0f);

  mVertexData.vertices.emplace_back(startVert);
  mVertexData.vertices.emplace_back(endVert);

  /*  Y axis - green */
  for (int i = 0; i <= endIndex; ++i) {
    startVert.position = glm::vec3(circleValues.at(i).x, 0.0f, circleValues.at(i).y);
    startVert.color = glm::vec3(0.0f, 0.8f, 0.0f);
    endVert.position = glm::vec3(circleValues.at(i + 1).x, 0.0f, circleValues.at(i+ 1).y);
    endVert.color = glm::vec3(0.0f, 0.8f, 0.0f);

    mVertexData.vertices.emplace_back(startVert);
    mVertexData.vertices.emplace_back(endVert);
  }

  /* arrow lines */
  startVert.position = glm::vec3(circleValues.at(endIndex + 1).x, 0.0f, circleValues.at(endIndex + 1).y);
  startVert.color = glm::vec3(0.0f, 0.8f, 0.0f);
  endVert.position = glm::vec3(circleValues.at(endIndex).x * 1.1f, 0.0f, circleValues.at(endIndex).y * 1.1f);
  endVert.color = glm::vec3(0.0f, 0.8f, 0.0f);

  mVertexData.vertices.emplace_back(startVert);
  mVertexData.vertices.emplace_back(endVert);

  endVert.position = glm::vec3(circleValues.at(endIndex).x / 1.1f, 0.0f, circleValues.at(endIndex).y / 1.1f);
  endVert.color = glm::vec3(0.0f, 0.8f, 0.0f);

  mVertexData.vertices.emplace_back(startVert);
  mVertexData.vertices.emplace_back(endVert);

  /*  Z axis - blue */
  for (int i = 0; i <= endIndex; ++i) {
    startVert.position = glm::vec3(circleValues.at(i).x, circleValues.at(i).y, 0.0f);
    startVert.color = glm::vec3(0.0f, 0.0f, 0.8f);
    endVert.position = glm::vec3(circleValues.at(i + 1).x, circleValues.at(i+ 1).y, 0.0f);
    endVert.color = glm::vec3(0.0f, 0.0f, 0.8f);

    mVertexData.vertices.emplace_back(startVert);
    mVertexData.vertices.emplace_back(endVert);
  }

  /* arrow lines */
  startVert.position = glm::vec3(circleValues.at(endIndex + 1).x, circleValues.at(endIndex + 1).y, 0.0f);
  startVert.color = glm::vec3(0.0f, 0.0f, 0.8f);
  endVert.position = glm::vec3(circleValues.at(endIndex).x * 1.1f, circleValues.at(endIndex).y * 1.1f, 0.0f);
  endVert.color = glm::vec3(0.0f, 0.0f, 0.8f);

  mVertexData.vertices.emplace_back(startVert);
  mVertexData.vertices.emplace_back(endVert);

  endVert.position = glm::vec3(circleValues.at(endIndex).x / 1.1f, circleValues.at(endIndex).y / 1.1f, 0.0f);
  endVert.color = glm::vec3(0.0f, 0.0f, 0.8f);

  mVertexData.vertices.emplace_back(startVert);
  mVertexData.vertices.emplace_back(endVert);

  Logger::log(1, "%s: RotationArrowsModel - loaded %d vertices\n", __FUNCTION__, mVertexData.vertices.size());
}

