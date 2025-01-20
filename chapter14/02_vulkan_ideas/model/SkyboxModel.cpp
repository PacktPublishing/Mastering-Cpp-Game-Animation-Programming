#include "SkyboxModel.h"
#include "Logger.h"

void SkyboxModel::init() {
  mVertexData.vertices.resize(36);

  /* front */
  mVertexData.vertices[0].position = glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f);
  mVertexData.vertices[1].position = glm::vec4( 1.0f,  1.0f, 1.0f, 1.0f);
  mVertexData.vertices[2].position = glm::vec4(-1.0f,  1.0f, 1.0f, 1.0f);
  mVertexData.vertices[3].position = glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f);
  mVertexData.vertices[4].position = glm::vec4( 1.0f, -1.0f, 1.0f, 1.0f);
  mVertexData.vertices[5].position = glm::vec4( 1.0f,  1.0f, 1.0f, 1.0f);

  /* back */
  mVertexData.vertices[6].position =  glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f);
  mVertexData.vertices[7].position =  glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f);
  mVertexData.vertices[8].position =  glm::vec4( 1.0f,  1.0f, -1.0f, 1.0f);
  mVertexData.vertices[9].position =  glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f);
  mVertexData.vertices[10].position = glm::vec4( 1.0f,  1.0f, -1.0f, 1.0f);
  mVertexData.vertices[11].position = glm::vec4( 1.0f, -1.0f, -1.0f, 1.0f);

  /* left */
  mVertexData.vertices[12].position = glm::vec4(-1.0f, -1.0f,  1.0f, 1.0f);
  mVertexData.vertices[13].position = glm::vec4(-1.0f,  1.0f,  1.0f, 1.0f);
  mVertexData.vertices[14].position = glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f);
  mVertexData.vertices[15].position = glm::vec4(-1.0f, -1.0f,  1.0f, 1.0f);
  mVertexData.vertices[16].position = glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f);
  mVertexData.vertices[17].position = glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f);

  /* right */
  mVertexData.vertices[18].position = glm::vec4(1.0f, -1.0f,  1.0f, 1.0f);
  mVertexData.vertices[19].position = glm::vec4(1.0f,  1.0f, -1.0f, 1.0f);
  mVertexData.vertices[20].position = glm::vec4(1.0f,  1.0f,  1.0f, 1.0f);
  mVertexData.vertices[21].position = glm::vec4(1.0f, -1.0f,  1.0f, 1.0f);
  mVertexData.vertices[22].position = glm::vec4(1.0f, -1.0f, -1.0f, 1.0f);
  mVertexData.vertices[23].position = glm::vec4(1.0f,  1.0f, -1.0f, 1.0f);

  /* top */
  mVertexData.vertices[24].position = glm::vec4( 1.0f,  1.0f,  1.0f, 1.0f);
  mVertexData.vertices[25].position = glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f);
  mVertexData.vertices[26].position = glm::vec4(-1.0f,  1.0f,  1.0f, 1.0f);
  mVertexData.vertices[27].position = glm::vec4( 1.0f,  1.0f,  1.0f, 1.0f);
  mVertexData.vertices[28].position = glm::vec4( 1.0f,  1.0f, -1.0f, 1.0f);
  mVertexData.vertices[29].position = glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f);

  /* bottom */
  mVertexData.vertices[30].position = glm::vec4( 1.0f,  -1.0f, 1.0f, 1.0f);
  mVertexData.vertices[31].position = glm::vec4(-1.0f,  -1.0f, 1.0f, 1.0f);
  mVertexData.vertices[32].position = glm::vec4(-1.0f,  -1.0f,-1.0f, 1.0f);
  mVertexData.vertices[33].position = glm::vec4( 1.0f,  -1.0f, 1.0f, 1.0f);
  mVertexData.vertices[34].position = glm::vec4(-1.0f,  -1.0f,-1.0f, 1.0f);
  mVertexData.vertices[35].position = glm::vec4( 1.0f,  -1.0f,-1.0f, 1.0f);

  Logger::log(1, "%s: loaded %d vertices\n", __FUNCTION__, mVertexData.vertices.size());
}

VkSkyboxMesh SkyboxModel::getVertexData() {
  return mVertexData;
}
