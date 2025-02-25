#include "IKSolver.h"
#include "Logger.h"
#include "Tools.h"

#include <glm/gtx/quaternion.hpp>

IKSolver::IKSolver() : mIterations(10) {}
IKSolver::IKSolver(int iterations) : mIterations(iterations) {}

void IKSolver::setNumIterations(int iterations) {
  mIterations = iterations;
}

void IKSolver::calculateOrigBoneLengths() {
  mBoneLengths.resize(mNodePositions.size() - 1);
  for (int i = 0; i < mNodePositions.size() - 1; ++i) {
    glm::vec3 startPos = mNodePositions.at(i);
    glm::vec3 endPos = mNodePositions.at(i + 1);

    mBoneLengths.at(i) = glm::length(endPos - startPos);
  }
}

void IKSolver::solveFABRIKForward(glm::vec3 targetPos) {
  /* set effector to target */
  mNodePositions.at(0) = targetPos;

  for (size_t i = 1; i < mNodePositions.size(); ++i) {
    glm::vec3 boneDirection = glm::normalize(mNodePositions.at(i) - mNodePositions.at(i - 1));
    glm::vec3 offset = boneDirection * mBoneLengths.at(i - 1);

    mNodePositions.at(i) = mNodePositions.at(i - 1) + offset;
  }
}

void IKSolver::solveFABRIKBackwards(glm::vec3 rootPos) {
  /* set root node back to (saved) target */
  mNodePositions.at(mNodePositions.size() - 1) = rootPos;

  for (int i = mNodePositions.size() - 2; i >= 0; --i) {
    glm::vec3 boneDirection = glm::normalize(mNodePositions.at(i) - mNodePositions.at(i + 1));
    glm::vec3 offset = boneDirection * mBoneLengths.at(i);

    mNodePositions.at(i) = mNodePositions.at(i + 1) + offset;
  }
}

std::vector<glm::vec3> IKSolver::solveFARBIK(std::vector<glm::mat4>& nodeMatrices, glm::vec3 targetPos) {
  if (nodeMatrices.empty()) {
    return std::vector<glm::vec3>{};
  }

  /* extract node positions */
  mNodePositions.resize(nodeMatrices.size());
  for (int i = 0; i < nodeMatrices.size(); ++i) {
    mNodePositions.at(i) = Tools::extractGlobalPosition(nodeMatrices.at(i));
  }

  /* we need the original bone lengths for FABRIK */
  calculateOrigBoneLengths();

  /* save position of root node for backward part */
  glm::vec3 rootPos = mNodePositions.at(mNodePositions.size() - 1);

  /* run the iterations */
  for (unsigned int i = 0; i < mIterations; ++i) {
    /* we are really close to the target, stop iterations */
    glm::vec3 effector = mNodePositions.at(0);
    if (glm::length(targetPos - effector) < mCloseThreshold) {
      return mNodePositions;
    }

    /* the solving itself */
    solveFABRIKForward(targetPos);
    solveFABRIKBackwards(rootPos);
  }

  return mNodePositions;
}


