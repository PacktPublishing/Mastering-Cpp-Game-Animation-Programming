#include "SphereModel.h"

#include <vector>
#include <cmath>

#include "Logger.h"

VkLineMesh SphereModel::getVertexData() {
  if (mVertexData.vertices.empty()) {
    init();
  }
  return mVertexData;
}

void SphereModel::init() {
  std::vector<std::vector<glm::vec3>> constructionVertexData;

  /* construct rings with position data */
  for (unsigned int i = 1; i < mVertDiv; ++i) {
    /* divide the elevation value into radians - we start at 90 degree! */
    float elevation = glm::radians(90.0f) - glm::radians(180.0f / mVertDiv) * i;
    std::vector<glm::vec3> ringData {};

    for (unsigned int j = 0; j < mHorDiv; ++j) {
      float amzimuth = glm::radians(360.0f / mHorDiv) * j;

      ringData.emplace_back(glm::vec3(
        std::sin(amzimuth) * std::cos(elevation) * mRadius,
        std::sin(elevation) * mRadius,
        std::cos(amzimuth) * std::cos(elevation) * mRadius
      ));
    }
    constructionVertexData.push_back(ringData);
  }

  /* construct top triangle ring */
  glm::vec3 topPos = glm::vec3(0.0f, mRadius, 0.0f);
  for (unsigned int j = 0; j < mHorDiv; ++j) {
    mVertexData.vertices.emplace_back(topPos, mColor);
    mVertexData.vertices.emplace_back(constructionVertexData.at(0).at(j), mColor);

    mVertexData.vertices.emplace_back(constructionVertexData.at(0).at(j), mColor);
    mVertexData.vertices.emplace_back(constructionVertexData.at(0).at((j + 1) % mHorDiv), mColor);

    mVertexData.vertices.emplace_back(constructionVertexData.at(0).at((j + 1) % mHorDiv), mColor);
    mVertexData.vertices.emplace_back(topPos, mColor);
  }

  /* 2x triangles as quad, over all rings */
  for (unsigned int i = 0; i < mVertDiv - 2; ++i) {
    for (unsigned int j = 0; j < mHorDiv; ++j) {
      mVertexData.vertices.emplace_back(constructionVertexData.at(i).at(j), mColor);
      mVertexData.vertices.emplace_back(constructionVertexData.at(i + 1).at(j), mColor);

      mVertexData.vertices.emplace_back(constructionVertexData.at(i + 1).at(j), mColor);
      mVertexData.vertices.emplace_back(constructionVertexData.at(i).at((j + 1) % mHorDiv), mColor);

      mVertexData.vertices.emplace_back(constructionVertexData.at(i).at((j + 1) % mHorDiv), mColor);
      mVertexData.vertices.emplace_back(constructionVertexData.at(i).at(j), mColor);


      mVertexData.vertices.emplace_back(constructionVertexData.at(i).at((j + 1) % mHorDiv), mColor);
      mVertexData.vertices.emplace_back(constructionVertexData.at(i + 1).at(j), mColor);

      mVertexData.vertices.emplace_back(constructionVertexData.at(i + 1).at(j), mColor);
      mVertexData.vertices.emplace_back(constructionVertexData.at(i + 1).at((j + 1) % mHorDiv), mColor);

      mVertexData.vertices.emplace_back(constructionVertexData.at(i + 1).at((j + 1) % mHorDiv), mColor);
      mVertexData.vertices.emplace_back(constructionVertexData.at(i).at((j + 1) % mHorDiv), mColor);
    }
  }

  /* construct bottom */
  glm::vec3 bottomPos = glm::vec3(0.0f, -mRadius, 0.0f);
  for (unsigned int j = 0; j < mHorDiv; ++j) {
    mVertexData.vertices.emplace_back(constructionVertexData.at(mVertDiv - 2).at((j + 1) % mHorDiv), mColor);
    mVertexData.vertices.emplace_back(constructionVertexData.at(mVertDiv - 2).at(j), mColor);

    mVertexData.vertices.emplace_back(constructionVertexData.at(mVertDiv - 2).at(j), mColor);
    mVertexData.vertices.emplace_back(bottomPos, mColor);

    mVertexData.vertices.emplace_back(bottomPos, mColor);
    mVertexData.vertices.emplace_back(constructionVertexData.at(mVertDiv - 2).at((j + 1) % mHorDiv), mColor);
  }

  Logger::log(1, "%s: SphereModel - loaded %d vertices\n", __FUNCTION__, mVertexData.vertices.size());
}

