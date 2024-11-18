/* Dear ImGui*/
#pragma once
#include <vector>

#include "OGLRenderData.h"
#include "ModelAndInstanceData.h"
#include "InstanceSettings.h"

class UserInterface {
  public:
    void init(OGLRenderData &renderData);
    void hideMouse(bool hide);

    void createFrame(OGLRenderData &renderData, ModelAndInstanceData &modInstData);
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
};
