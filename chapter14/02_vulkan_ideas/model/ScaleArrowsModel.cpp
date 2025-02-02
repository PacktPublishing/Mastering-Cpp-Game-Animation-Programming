#include "ScaleArrowsModel.h"
#include "Logger.h"

VkLineMesh ScaleArrowsModel::getVertexData() {
  if (mVertexData.vertices.empty()) {
    init();
  }
  return mVertexData;
}

void ScaleArrowsModel::init() {
  mVertexData.vertices.resize(30);

  /*  scale axis - yellow */
  /* X */
  mVertexData.vertices[0].position = glm::vec3(-1.0f, 0.0f,  0.0f);
  mVertexData.vertices[1].position = glm::vec3( 1.0f, 0.0f,  0.0f);

  mVertexData.vertices[2].position = glm::vec3(-1.0f, 0.0f,  0.0f);
  mVertexData.vertices[3].position = glm::vec3(-0.8f, 0.0f,  0.075f);
  mVertexData.vertices[4].position = glm::vec3(-1.0f, 0.0f,  0.0f);
  mVertexData.vertices[5].position = glm::vec3(-0.8f, 0.0f, -0.075f);

  mVertexData.vertices[6].position = glm::vec3( 1.0f, 0.0f,  0.0f);
  mVertexData.vertices[7].position = glm::vec3( 0.8f, 0.0f,  0.075f);
  mVertexData.vertices[8].position = glm::vec3( 1.0f, 0.0f,  0.0f);
  mVertexData.vertices[9].position = glm::vec3( 0.8f, 0.0f, -0.075f);

  mVertexData.vertices[0].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[1].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[2].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[3].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[4].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[5].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[6].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[7].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[8].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[9].color = glm::vec3(0.8f, 0.8f, 0.0f);

  /* Y */
  mVertexData.vertices[10].position = glm::vec3( 0.0f, -1.0f,  0.0f);
  mVertexData.vertices[11].position = glm::vec3( 0.0f,  1.0f,  0.0f);

  mVertexData.vertices[12].position = glm::vec3( 0.0f, -1.0f,  0.0f);
  mVertexData.vertices[13].position = glm::vec3( 0.0f, -0.8f,  0.075f);
  mVertexData.vertices[14].position = glm::vec3( 0.0f, -1.0f,  0.0f);
  mVertexData.vertices[15].position = glm::vec3( 0.0f, -0.8f, -0.075f);

  mVertexData.vertices[16].position = glm::vec3( 0.0f,  1.0f,  0.0f);
  mVertexData.vertices[17].position = glm::vec3( 0.0f,  0.8f,  0.075f);
  mVertexData.vertices[18].position = glm::vec3( 0.0f,  1.0f,  0.0f);
  mVertexData.vertices[19].position = glm::vec3( 0.0f,  0.8f, -0.075f);

  mVertexData.vertices[10].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[11].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[12].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[13].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[14].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[15].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[16].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[17].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[18].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[19].color = glm::vec3(0.8f, 0.8f, 0.0f);

  /* Z */
  mVertexData.vertices[20].position = glm::vec3( 0.0f, 0.0f, -1.0f);
  mVertexData.vertices[21].position = glm::vec3( 0.0f, 0.0f,  1.0f);

  mVertexData.vertices[22].position = glm::vec3( 0.0f,   0.0f, -1.0f);
  mVertexData.vertices[23].position = glm::vec3( 0.075f, 0.0f, -0.8f);
  mVertexData.vertices[24].position = glm::vec3( 0.0f,   0.0f, -1.0f);
  mVertexData.vertices[25].position = glm::vec3(-0.075f, 0.0f, -0.8f);

  mVertexData.vertices[26].position = glm::vec3( 0.0f,   0.0f,  1.0f);
  mVertexData.vertices[27].position = glm::vec3( 0.075f, 0.0f,  0.8f);
  mVertexData.vertices[28].position = glm::vec3( 0.0f,   0.0f,  1.0f);
  mVertexData.vertices[29].position = glm::vec3(-0.075f, 0.0f,  0.8f);

  mVertexData.vertices[20].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[21].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[22].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[23].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[24].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[25].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[26].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[27].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[28].color = glm::vec3(0.8f, 0.8f, 0.0f);
  mVertexData.vertices[29].color = glm::vec3(0.8f, 0.8f, 0.0f);

  Logger::log(1, "%s: ScaleArrowsModel - loaded %d vertices\n", __FUNCTION__, mVertexData.vertices.size());
}

