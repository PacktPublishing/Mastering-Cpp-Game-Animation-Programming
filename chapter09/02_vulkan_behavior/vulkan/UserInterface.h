/* Dear ImGui */
#pragma once
#include <string>
#include <vector>

#include <imgui.h>

#include "VkRenderData.h"
#include "AssimpModel.h"
#include "AssimpInstance.h"
#include "ModelInstanceCamData.h"
#include "InstanceSettings.h"
#include "CameraSettings.h"
#include "Camera.h"
#include "SingleInstanceBehavior.h"

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

    std::vector<float> mBehaviorManagerValues{};
    int mNumBehaviorManagerValues = 90;

    std::vector<float> mInteractionValues{};
    int mNumInteractionValues = 90;

    float mNewFps = 0.0f;
    double mUpdateTime = 0.0;

    int mFpsOffset = 0;
    int mFrameTimeOffset = 0;
    int mModelUploadOffset = 0;
    int mMatrixGenOffset = 0;
    int mMatrixUploadOffset = 0;
    int mUiGenOffset = 0;
    int mUiDrawOffset = 0;
    int mCollisionDebugDrawOffset = 0;
    int mCollisionCheckOffset = 0;
    int mNumCollisionOffset = 0;
    int mBehaviorManagerOffset = 0;
    int mInteractionOffset = 0;

    int mManyInstanceCreateNum = 1;
    int mManyInstanceCloneNum = 1;

    InstanceSettings mSavedInstanceSettings{};
    std::shared_ptr<AssimpInstance> mCurrentInstance = nullptr;
    std::shared_ptr<AssimpModel> mCurrentModel = nullptr;

    CameraSettings mSavedCameraSettings{};
    std::shared_ptr<Camera> mCurrentCamera = nullptr;
    bool mShowDuplicateCamNameDialog = false;

    std::vector<std::string> mBoneNames{};

    std::string mNewTreeName = "Tree1";
    std::string mSelectedTreeName;
    std::string mTreeToDelete;
    std::shared_ptr<SingleInstanceBehavior> mBehaviorManager = nullptr;

    static int nameInputFilter(ImGuiInputTextCallbackData* data);
};
