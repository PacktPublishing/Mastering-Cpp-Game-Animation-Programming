#include "Tools.h"

std::string Tools::getFilenameExt(std::string filename) {
  size_t pos = filename.find_last_of('.');
  if (pos != std::string::npos) {
    return filename.substr(pos + 1);
  }
  return std::string();
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
