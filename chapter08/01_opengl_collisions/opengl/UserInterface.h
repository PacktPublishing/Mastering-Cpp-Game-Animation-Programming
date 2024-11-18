/* Dear ImGui */
#pragma once
#include <string>
#include <vector>

#include <imgui.h>

#include "OGLRenderData.h"
#include "ModelInstanceCamData.h"
#include "InstanceSettings.h"

class UserInterface {
  public:
    void init(OGLRenderData &renderData);
    void hideMouse(bool hide);

    void createFrame(OGLRenderData &renderData);
    void createSettingsWindow(OGLRenderData &renderData, ModelInstanceCamData &modInstCamData);
    void createStatusBar(OGLRenderData &renderData, ModelInstanceCamData &modInstCamData);
    void createPositionsWindow(OGLRenderData &renderData, ModelInstanceCamData &modInstCamData);

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

    static int cameraNameInputFilter(ImGuiInputTextCallbackData* data);
};
