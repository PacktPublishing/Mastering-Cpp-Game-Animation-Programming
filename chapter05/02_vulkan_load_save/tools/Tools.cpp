#include <iostream>
#include <fstream>
#include <cerrno>  // errno
#include <cstring> // strerror()

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
