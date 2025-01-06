#pragma once
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Shader {
  public:
    bool loadShaders(std::string vertexShaderFileName, std::string fragmentShaderFileName);
    bool loadComputeShader(std::string computeShaderFileName);

    void use();
    bool getUniformLocation(std::string uniformName);
    void setUniformValue(int value);

    void cleanup();

  private:
    GLuint mShaderProgram = 0;
    GLint mUniformLocation = -1;

    bool createShaderProgram(std::string vertexShaderFileName, std::string fragmentShaderFileName);
    bool createComputeShaderProgram(std::string computeShaderName);

    GLuint loadShader(std::string shaderFileName, GLuint shaderType);
    std::string loadFileToString(std::string filename);

    bool checkCompileStats(std::string shaderFileName, GLuint shader);
    bool checkLinkStats(std::string vertexShaderFileName, std::string fragmentShaderFileName, GLuint shaderProgram);
    bool checkLinkStats(std::string computeShaderFileName, GLuint shaderProgram);
};
