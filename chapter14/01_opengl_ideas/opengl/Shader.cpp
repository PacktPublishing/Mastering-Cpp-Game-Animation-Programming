#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Tools.h"
#include "Logger.h"

bool Shader::loadShaders(std::string vertexShaderFileName, std::string fragmentShaderFileName) {
  Logger::log(1, "%s: loading vertex shader '%s' and fragment shader '%s'\n", __FUNCTION__, vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());

  if (!createShaderProgram(vertexShaderFileName, fragmentShaderFileName)) {
    Logger::log(1, "%s error: shader program creation failed\n", __FUNCTION__);
    return false;
  }

  return true;
}

bool Shader::loadComputeShader(std::string computeShaderFileName) {
  Logger::log(1, "%s: loading compute shader '%s'\n", __FUNCTION__, computeShaderFileName.c_str());

  if (!createComputeShaderProgram(computeShaderFileName)) {
    Logger::log(1, "%s error: shader program creation failed\n", __FUNCTION__);
    return false;
  }

  return true;
}


void Shader::use() {
  glUseProgram(mShaderProgram);
}

bool Shader::getUniformLocation(std::string uniformName) {
  if (mShaderProgram > 0) {
    mUniformLocation = glGetUniformLocation(mShaderProgram, uniformName.c_str());
    return mUniformLocation > -1;
  }
  return false;
}

void Shader::setUniformValue(int value) {
  if (mShaderProgram > 0) {
    /* 0 is a valid location */
    if (mUniformLocation > -1) {
      glUniform1i(mUniformLocation, value);
    }
  }
}

void Shader::cleanup() {
  glDeleteProgram(mShaderProgram);
}

GLuint Shader::loadShader(std::string shaderFileName, GLuint shaderType) {
  std::string shaderAsText;
  shaderAsText = Tools::loadFileToString(shaderFileName);
  Logger::log(4, "%s: loaded shader file '%s', size %i\n", __FUNCTION__, shaderFileName.c_str(),shaderAsText.size());

  const char* shaderSource = shaderAsText.c_str();
  GLuint shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, (const GLchar**) &shaderSource, 0);
  glCompileShader(shader);

  if (!checkCompileStats(shaderFileName, shader)) {
    Logger::log(1, "%s error: compiling shader '%s' failed\n", __FUNCTION__, shaderFileName.c_str());
    return 0;
  }

  Logger::log(1, "%s: shader %#x loaded and compiled\n", __FUNCTION__, shader);
  return shader;
}

bool Shader::createShaderProgram(std::string vertexShaderFileName, std::string fragmentShaderFileName) {
  GLuint vertexShader = loadShader(vertexShaderFileName, GL_VERTEX_SHADER);
  if (!vertexShader) {
    Logger::log(1, "%s: loading of vertex shader '%s' failed\n", __FUNCTION__, vertexShaderFileName.c_str());
    return false;
  }

  GLuint fragmentShader = loadShader(fragmentShaderFileName, GL_FRAGMENT_SHADER);
  if (!fragmentShader) {
    Logger::log(1, "%s: loading of fragment shader '%s' failed\n", __FUNCTION__, fragmentShaderFileName.c_str());
    return false;
  }

  mShaderProgram = glCreateProgram();

  glAttachShader(mShaderProgram, vertexShader);
  glAttachShader(mShaderProgram, fragmentShader);

  glLinkProgram(mShaderProgram);

  if (!checkLinkStats(vertexShaderFileName, fragmentShaderFileName, mShaderProgram)) {
    Logger::log(1, "%s error: program linking from vertex shader '%s' / fragment shader '%s' failed\n", __FUNCTION__, vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return false;
  }

  /* bind UBO in shader */
  GLint uboIndex = glGetUniformBlockIndex(mShaderProgram, "Matrices");
  glUniformBlockBinding(mShaderProgram, uboIndex, 0);

  /* it is safe to delete the original shaders here */
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  Logger::log(1, "%s: shader program %#x successfully compiled from vertex shader '%s' and fragment shader '%s'\n", __FUNCTION__, mShaderProgram, vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());
  return true;
}

bool Shader::createComputeShaderProgram(std::string computeShaderName) {
  GLuint computeShader = loadShader(computeShaderName, GL_COMPUTE_SHADER);
  if (!computeShader) {
    Logger::log(1, "%s: loading of compute shader '%s' failed\n", __FUNCTION__, computeShaderName.c_str());
    return false;
  }

  mShaderProgram = glCreateProgram();

  glAttachShader(mShaderProgram, computeShader);

  glLinkProgram(mShaderProgram);

  if (!checkLinkStats(computeShaderName, mShaderProgram)) {
    Logger::log(1, "%s error: program linking from compute shader '%s' failed\n", __FUNCTION__, computeShaderName.c_str());

    glDeleteShader(computeShader);

    return false;
  }

  /* it is safe to delete the original shaders here */
  glDeleteShader(computeShader);

  Logger::log(1, "%s: shader program %#x successfully compiled from compute shader '%s'\n", __FUNCTION__, mShaderProgram, computeShaderName.c_str());
  return true;
}


bool Shader::checkCompileStats(std::string shaderFileName, GLuint shader) {
  GLint isShaderCompiled;
  int logMessageLength;
  std::vector<char> shaderLog;

  glGetShaderiv(shader, GL_COMPILE_STATUS, &isShaderCompiled);
  if (!isShaderCompiled) {
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logMessageLength);
    shaderLog = std::vector<char>(logMessageLength + 1);
    glGetShaderInfoLog(shader, logMessageLength, &logMessageLength, shaderLog.data());
    shaderLog.at(logMessageLength) = '\0';
    Logger::log(1, "%s error: shader compile of shader '%s' failed\n", __FUNCTION__, shaderFileName.c_str());
    Logger::log(1, "%s compile log:\n%s\n", __FUNCTION__, shaderLog.data());
    return false;
  }

  return true;
}

bool Shader::checkLinkStats(std::string vertexShaderFileName, std::string fragmentShaderFileName, GLuint shaderProgram) {
  GLint isProgramLinked;
  int logMessageLength;
  std::vector<char> programLog;

  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isProgramLinked);
  if (!isProgramLinked) {
    glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logMessageLength);
    programLog = std::vector<char>(logMessageLength + 1);
    glGetProgramInfoLog(shaderProgram, logMessageLength, &logMessageLength, programLog.data());
    programLog.at(logMessageLength) = '\0';
    Logger::log(1, "%s error: program linking of shaders '%s' and '%s' failed\n", __FUNCTION__, vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());
    Logger::log(1, "%s compile log:\n%s\n", __FUNCTION__, programLog.data());
    return false;
  }

  return true;
}

bool Shader::checkLinkStats(std::string computeShaderFileName, GLuint shaderProgram) {
  GLint isProgramLinked;
  int logMessageLength;
  std::vector<char> programLog;

  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isProgramLinked);
  if (!isProgramLinked) {
    glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logMessageLength);
    programLog = std::vector<char>(logMessageLength + 1);
    glGetProgramInfoLog(shaderProgram, logMessageLength, &logMessageLength, programLog.data());
    programLog.at(logMessageLength) = '\0';
    Logger::log(1, "%s error: program linking of shader '%s' failed\n", __FUNCTION__, computeShaderFileName.c_str());
    Logger::log(1, "%s compile log:\n%s\n", __FUNCTION__, programLog.data());
    return false;
  }

  return true;
}
