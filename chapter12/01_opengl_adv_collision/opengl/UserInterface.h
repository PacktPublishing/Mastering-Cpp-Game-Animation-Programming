/* Dear ImGui */
#pragma once
#include <string>
#include <vector>

#include <imgui.h>

#include "OGLRenderData.h"
#include "ModelInstanceCamData.h"

class UserInterface {
  public:
    void init(OGLRenderData &renderData);
    void hideMouse(bool hide);

    void createFrame(OGLRenderData &renderData);
    void createSettingsWindow(OGLRenderData &renderData, ModelInstanceCamData &modInstCamData);
    void createStatusBar(OGLRenderData &renderData, ModelInstanceCamData &modInstCamData);
    void createPositionsWindow(OGLRenderData &renderData, ModelInstanceCamData &modInstCamData);
    void resetPositionWindowOctreeView();

    void render();
    void cleanup();

  private:
    float mFramesPerSecond = 0.0f;
    /* averaging speed */
    float mAveragingAlpha = 0.96f;

    std::vector<float> mFPSValues{};
    int mNumFPSValues = 90;

    std::vector<float> mFrameTimeValues{};
    int mNumFrameTimeValues = 90;

    std::vector<float> mModelUploadValues{};
    int mNumModelUploadValues = 90;

    std::vector<float> mMatrixGenerationValues{};
    int mNumMatrixGenerationValues = 90;

    std::vector<float> mMatrixUploadValues{};
    int mNumMatrixUploadValues = 90;

    std::vector<float> mMatrixDownloadValues{};
    int mNumMatrixDownloadValues = 90;

    std::vector<float> mUiGenValues{};
    int mNumUiGenValues = 90;

    std::vector<float> mUiDrawValues{};
    int mNumUiDrawValues = 90;

    std::vector<float> mCollisionDebugDrawValues{};
    int mNumCollisionDebugDrawValues = 90;

    std::vector<float> mCollisionCheckValues{};
    int mNumCollisionCheckValues = 90;

    std::vector<float> mNumCollisionsValues{};
    int mNumNumCollisionValues = 90;

    std::vector<float> mBehaviorValues{};
    int mNumBehaviorValues = 90;

    std::vector<float> mInteractionValues{};
    int mNumInteractionValues = 90;

    std::vector<float> mFaceAnimValues{};
    int mNumFaceAnimValues = 90;

    std::vector<float> mLevelCollisionCheckValues{};
    int mNumLevelCollisionCheckValues = 90;

    std::vector<float> mIKValues{};
    int mNumIKValues = 90;

    static int nameInputFilter(ImGuiInputTextCallbackData* data);

    OGLLineMesh mOctreeLines{};
    float mOctreeZoomFactor = 1.0f;
    glm::vec3 mOctreeRotation = glm::vec3(0.0f);
    glm::vec3 mOctreeTranslation = glm::vec3(0.0f);

    glm::mat4 mScaleMat = glm::mat4(1.0f);
    glm::mat4 mRotationMat = glm::mat4(1.0f);
    glm::mat3 mOctreeViewMat = glm::mat3(1.0f);
};
