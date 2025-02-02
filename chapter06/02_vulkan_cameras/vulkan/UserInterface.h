/* Dear ImGui */
#pragma once
#include <string>
#include <vector>

#include <imgui.h>

#include "VkRenderData.h"
#include "AssimpInstance.h"
#include "ModelInstanceCamData.h"
#include "InstanceSettings.h"
#include "CameraSettings.h"
#include "Camera.h"

class UserInterface {
  public:
    bool init(VkRenderData& renderData);
    void hideMouse(bool hide);

    void createFrame(VkRenderData& renderData);
    void createSettingsWindow(VkRenderData &renderData, ModelInstanceCamData &modInstCamData);
    void createStatusBar(VkRenderData &renderData, ModelInstanceCamData &modInstCamData);

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

    float mNewFps = 0.0f;
    double mUpdateTime = 0.0;

    int mFpsOffset = 0;
    int mFrameTimeOffset = 0;
    int mModelUploadOffset = 0;
    int mMatrixGenOffset = 0;
    int mMatrixUploadOffset = 0;
    int mUiGenOffset = 0;
    int mUiDrawOffset = 0;

    int mManyInstanceCreateNum = 1;
    int mManyInstanceCloneNum = 1;

    InstanceSettings mSavedInstanceSettings{};
    std::shared_ptr<AssimpInstance> mCurrentInstance = nullptr;

    CameraSettings mSsavedCameraSettings{};
    std::shared_ptr<Camera> mCurrentCamera = nullptr;
    bool mShowDuplicateCamNameDialog = false;

    std::vector<std::string> mBboneNames{};

    static int cameraNameInputFilter(ImGuiInputTextCallbackData* data);
};
