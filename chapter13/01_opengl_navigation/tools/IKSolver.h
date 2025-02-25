#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

class IKSolver {
  public:
    IKSolver();
    IKSolver(int iterations);

    void setNumIterations(int iterations);

    std::vector<glm::vec3> solveFARBIK(std::vector<glm::mat4>& nodeMatrices, glm::vec3 targetPos);

  private:
    std::vector<glm::vec3> mNodePositions{};
    std::vector<float> mBoneLengths{};

    void solveFABRIKForward(glm::vec3 targetPos);
    void solveFABRIKBackwards(glm::vec3 rootPos);

    void calculateOrigBoneLengths();

    int mIterations = 0;
    float mCloseThreshold = 0.00001f;
};
