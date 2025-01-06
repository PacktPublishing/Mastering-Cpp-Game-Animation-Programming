/* Dear ImGui */
#pragma once
#include <string>
#include <vector>

#include <imgui.h>

#include "VkRenderData.h"
#include "ModelInstanceCamData.h"

class UserInterface {
  public:
    bool init(VkRenderData& renderData);
    void hideMouse(bool hide);

    void createFrame(VkRenderData& renderData);
    void createSettingsWindow(VkRenderData &renderData, ModelInstanceCamData &modInstCamData);
    void createStatusBar(VkRenderData &renderData, ModelInstanceCamData &modInstCamData);
    void createPositionsWindow(VkRenderData &renderData, ModelInstanceCamData &modInstCamData);

    void render(VkRenderData& renderData);
    void cleanup(VkRenderData& renderData);

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

    std::vector<float> mBehaviorValues{};
    int mNumBehaviorValues = 90;

    std::vector<float> mInteractionValues{};
    int mNumInteractionValues = 90;

    static int nameInputFilter(ImGuiInputTextCallbackData* data);
};
