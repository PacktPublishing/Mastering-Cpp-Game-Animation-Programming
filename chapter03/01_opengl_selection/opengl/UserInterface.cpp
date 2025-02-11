#include <string>
#include <limits>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuiFileDialog.h>

#include <filesystem>

#include "UserInterface.h"
#include "AssimpModel.h"
#include "AssimpAnimClip.h"
#include "AssimpInstance.h"
#include "InstanceSettings.h"
#include "Logger.h"

void UserInterface::init(OGLRenderData &renderData) {
  IMGUI_CHECKVERSION();

  ImGui::CreateContext();

  ImGui_ImplGlfw_InitForOpenGL(renderData.rdWindow, true);

  const char *glslVersion = "#version 460 core";
  ImGui_ImplOpenGL3_Init(glslVersion);

  ImGui::StyleColorsDark();

  /* init plot vectors */
  mFPSValues.resize(mNumFPSValues);
  mFrameTimeValues.resize(mNumFrameTimeValues);
  mModelUploadValues.resize(mNumModelUploadValues);
  mMatrixGenerationValues.resize(mNumMatrixGenerationValues);
  mMatrixUploadValues.resize(mNumMatrixUploadValues);
  mUiGenValues.resize(mNumUiGenValues);
  mUiDrawValues.resize(mNumUiDrawValues);
}

void UserInterface::hideMouse(bool hide) {
  /* v1.89.8 removed the check for disabled mouse cursor in GLFW
   * we need to ignore the mouse postion if the mouse lock is active */
  ImGuiIO& io = ImGui::GetIO();

  if (hide) {
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
  } else {
    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
  }
}

void UserInterface::createFrame(OGLRenderData &renderData, ModelAndInstanceData &modInstData) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGuiIO& io = ImGui::GetIO();
  ImGuiWindowFlags imguiWindowFlags = 0;

  ImGui::SetNextWindowBgAlpha(0.8f);

  /* dim background for modal dialogs */
  ImGuiStyle& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.75f);

  /* avoid inf values (division by zero) */
  if (renderData.rdFrameTime > 0.0) {
    mNewFps = 1.0f / renderData.rdFrameTime * 1000.f;
  }
  /* make an averge value to avoid jumps */
  mFramesPerSecond = (mAveragingAlpha * mFramesPerSecond) + (1.0f - mAveragingAlpha) * mNewFps;

  /* clamp manual input on all sliders to min/max */
  ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;

  /* avoid literal double compares */
  if (mUpdateTime < 0.000001) {
    mUpdateTime = ImGui::GetTime();
  }

  while (mUpdateTime < ImGui::GetTime()) {
    mFPSValues.at(mFpsOffset) = mFramesPerSecond;
    mFpsOffset = ++mFpsOffset % mNumFPSValues;

    mFrameTimeValues.at(mFrameTimeOffset) = renderData.rdFrameTime;
    mFrameTimeOffset = ++mFrameTimeOffset % mNumFrameTimeValues;

    mModelUploadValues.at(mModelUploadOffset) = renderData.rdUploadToVBOTime;
    mModelUploadOffset = ++mModelUploadOffset % mNumModelUploadValues;

    mMatrixGenerationValues.at(mMatrixGenOffset) = renderData.rdMatrixGenerateTime;
    mMatrixGenOffset = ++mMatrixGenOffset % mNumMatrixGenerationValues;

    mMatrixUploadValues.at(mMatrixUploadOffset) = renderData.rdUploadToUBOTime;
    mMatrixUploadOffset = ++mMatrixUploadOffset % mNumMatrixUploadValues;

    mUiGenValues.at(mUiGenOffset) = renderData.rdUIGenerateTime;
    mUiGenOffset = ++mUiGenOffset % mNumUiGenValues;

    mUiDrawValues.at(mUiDrawOffset) = renderData.rdUIDrawTime;
    mUiDrawOffset = ++mUiDrawOffset % mNumUiDrawValues;

    mUpdateTime += 1.0 / 30.0;
  }

  if (!ImGui::Begin("Control", nullptr, imguiWindowFlags)) {
    /* window collapsed */
    ImGui::End();
    return;
  }

  ImGui::Text("FPS: %10.4f", mFramesPerSecond);

  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    float averageFPS = 0.0f;
    for (const auto value : mFPSValues) {
      averageFPS += value;
    }
    averageFPS /= static_cast<float>(mNumFPSValues);
    std::string fpsOverlay = "now:     " + std::to_string(mFramesPerSecond) + "\n30s avg: " + std::to_string(averageFPS);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("FPS");
    ImGui::SameLine();
    ImGui::PlotLines("##FrameTimes", mFPSValues.data(), mFPSValues.size(), mFpsOffset, fpsOverlay.c_str(), 0.0f,
      std::numeric_limits<float>::max(), ImVec2(0, 80));
    ImGui::EndTooltip();
  }

  if (ImGui::CollapsingHeader("Info")) {
    ImGui::Text("Triangles:              %10i", renderData.rdTriangleCount);

    std::string unit = "B";
    float memoryUsage = renderData.rdMatricesSize;

    if (memoryUsage > 1024.0f * 1024.0f) {
      memoryUsage /= 1024.0f * 1024.0f;
      unit = "MB";
    } else  if (memoryUsage > 1024.0f) {
      memoryUsage /= 1024.0f;
      unit = "KB";
    }

    ImGui::Text("Instance Matrix Size:  %8.2f %2s", memoryUsage, unit.c_str());

    std::string windowDims = std::to_string(renderData.rdWidth) + "x" + std::to_string(renderData.rdHeight);
    ImGui::Text("Window Dimensions:      %10s", windowDims.c_str());

    std::string imgWindowPos = std::to_string(static_cast<int>(ImGui::GetWindowPos().x)) + "/" + std::to_string(static_cast<int>(ImGui::GetWindowPos().y));
    ImGui::Text("ImGui Window Position:  %10s", imgWindowPos.c_str());
  }

  if (ImGui::CollapsingHeader("Timers")) {
    ImGui::Text("Frame Time:             %10.4f ms", renderData.rdFrameTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageFrameTime = 0.0f;
      for (const auto value : mFrameTimeValues) {
        averageFrameTime += value;
      }
      averageFrameTime /= static_cast<float>(mNumMatrixGenerationValues);
      std::string frameTimeOverlay = "now:     " + std::to_string(renderData.rdFrameTime) +
        " ms\n30s avg: " + std::to_string(averageFrameTime) + " ms";
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Frame Time       ");
      ImGui::SameLine();
      ImGui::PlotLines("##FrameTime", mFrameTimeValues.data(), mFrameTimeValues.size(), mFrameTimeOffset,
        frameTimeOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Model Upload Time:      %10.4f ms", renderData.rdUploadToVBOTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageModelUpload = 0.0f;
      for (const auto value : mModelUploadValues) {
        averageModelUpload += value;
      }
      averageModelUpload /= static_cast<float>(mNumModelUploadValues);
      std::string modelUploadOverlay = "now:     " + std::to_string(renderData.rdUploadToVBOTime) +
        " ms\n30s avg: " + std::to_string(averageModelUpload) + " ms";
      ImGui::AlignTextToFramePadding();
      ImGui::Text("VBO Upload");
      ImGui::SameLine();
      ImGui::PlotLines("##ModelUploadTimes", mModelUploadValues.data(), mModelUploadValues.size(), mModelUploadOffset,
        modelUploadOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Matrix Generation Time: %10.4f ms", renderData.rdMatrixGenerateTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageMatGen = 0.0f;
      for (const auto value : mMatrixGenerationValues) {
        averageMatGen += value;
      }
      averageMatGen /= static_cast<float>(mNumMatrixGenerationValues);
      std::string matrixGenOverlay = "now:     " + std::to_string(renderData.rdMatrixGenerateTime) +
        " ms\n30s avg: " + std::to_string(averageMatGen) + " ms";
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Matrix Generation");
      ImGui::SameLine();
      ImGui::PlotLines("##MatrixGenTimes", mMatrixGenerationValues.data(), mMatrixGenerationValues.size(), mMatrixGenOffset,
        matrixGenOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Matrix Upload Time:     %10.4f ms", renderData.rdUploadToUBOTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageMatrixUpload = 0.0f;
      for (const auto value : mMatrixUploadValues) {
        averageMatrixUpload += value;
      }
      averageMatrixUpload /= static_cast<float>(mNumMatrixUploadValues);
      std::string matrixUploadOverlay = "now:     " + std::to_string(renderData.rdUploadToUBOTime) +
        " ms\n30s avg: " + std::to_string(averageMatrixUpload) + " ms";
      ImGui::AlignTextToFramePadding();
      ImGui::Text("UBO Upload");
      ImGui::SameLine();
      ImGui::PlotLines("##MatrixUploadTimes", mMatrixUploadValues.data(), mMatrixUploadValues.size(), mMatrixUploadOffset,
        matrixUploadOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("UI Generation Time:     %10.4f ms", renderData.rdUIGenerateTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageUiGen = 0.0f;
      for (const auto value : mUiGenValues) {
        averageUiGen += value;
      }
      averageUiGen /= static_cast<float>(mNumUiGenValues);
      std::string uiGenOverlay = "now:     " + std::to_string(renderData.rdUIGenerateTime) +
        " ms\n30s avg: " + std::to_string(averageUiGen) + " ms";
      ImGui::AlignTextToFramePadding();
      ImGui::Text("UI Generation");
      ImGui::SameLine();
      ImGui::PlotLines("##UIGenTimes", mUiGenValues.data(), mUiGenValues.size(), mUiGenOffset,
        uiGenOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("UI Draw Time:           %10.4f ms", renderData.rdUIDrawTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageUiDraw = 0.0f;
      for (const auto value : mUiDrawValues) {
        averageUiDraw += value;
      }
      averageUiDraw /= static_cast<float>(mNumUiDrawValues);
      std::string uiDrawOverlay = "now:     " + std::to_string(renderData.rdUIDrawTime) +
        " ms\n30s avg: " + std::to_string(averageUiDraw) + " ms";
      ImGui::AlignTextToFramePadding();
      ImGui::Text("UI Draw");
      ImGui::SameLine();
      ImGui::PlotLines("##UIDrawTimes", mUiDrawValues.data(), mUiDrawValues.size(), mUiDrawOffset,
        uiDrawOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }
  }

  if (ImGui::CollapsingHeader("Camera")) {
    ImGui::Text("Camera Position: %s", glm::to_string(renderData.rdCameraWorldPosition).c_str());
    ImGui::Text("View Azimuth:    %6.1f", renderData.rdViewAzimuth);
    ImGui::Text("View Elevation:  %6.1f", renderData.rdViewElevation);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Field of View");
    ImGui::SameLine();
    ImGui::SliderInt("##FOV", &renderData.rdFieldOfView, 40, 100, "%d", flags);
  }

  if (ImGui::CollapsingHeader("Models")) {
    /* state is changed during model deletion, so save it first */
    bool modelListEmtpy = modInstData.miModelList.size() == 1;
    std::string selectedModelName = "None";

    if (!modelListEmtpy) {
      selectedModelName = modInstData.miModelList.at(modInstData.miSelectedModel)->getModelFileName().c_str();
    }

    if (modelListEmtpy) {
      ImGui::BeginDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Models :");
    ImGui::SameLine();
    ImGui::PushItemWidth(300);
    if (ImGui::BeginCombo("##ModelCombo",
      /* avoid access the empty model vector or the null model name */
      selectedModelName.c_str())) {
      for (int i = 1; i < modInstData.miModelList.size(); ++i) {
        const bool isSelected = (modInstData.miSelectedModel == i);
        if (ImGui::Selectable(modInstData.miModelList.at(i)->getModelFileName().c_str(), isSelected)) {
          modInstData.miSelectedModel = i;
          selectedModelName = modInstData.miModelList.at(modInstData.miSelectedModel)->getModelFileName().c_str();
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    if (modelListEmtpy) {
      ImGui::EndDisabled();
    }

    if (ImGui::Button("Import Model")) {
      IGFD::FileDialogConfig config;
      config.path = ".";
      config.countSelectionMax = 1;
      config.flags = ImGuiFileDialogFlags_Modal;
      ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
      ImGuiFileDialog::Instance()->OpenDialog("ChooseModelFile", "Choose Model File",
        "Supported Model Files{.gltf,.glb,.obj,.fbx,.dae,.mdl,.md3,.pk3}", config);
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseModelFile")) {
      if (ImGuiFileDialog::Instance()->IsOk()) {
        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();

        /* try to construct a relative path */
        std::filesystem::path currentPath = std::filesystem::current_path();
        std::string relativePath =  std::filesystem::relative(filePathName, currentPath).generic_string();

        if (!relativePath.empty()) {
          filePathName = relativePath;
        }
        /* Windows does understand forward slashes, but std::filesystem preferres backslashes... */
        std::replace(filePathName.begin(), filePathName.end(), '\\', '/');

        if (!modInstData.miModelAddCallbackFunction(filePathName)) {
          Logger::log(1, "%s error: unable to load model file '%s', unknown error \n", __FUNCTION__, filePathName.c_str());
        } else {
          /* select new model and new instance */
          modInstData.miSelectedModel = modInstData.miModelList.size() - 1;
          modInstData.miSelectedInstance = modInstData.miAssimpInstances.size() - 1;
        }
      }
      ImGuiFileDialog::Instance()->Close();
    }

    if (modelListEmtpy) {
     ImGui::BeginDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete Model")) {
      ImGui::OpenPopup("Delete Model?");
    }

    if (ImGui::BeginPopupModal("Delete Model?", nullptr, ImGuiChildFlags_AlwaysAutoResize)) {
      ImGui::Text("Delete Model '%s'?", modInstData.miModelList.at(modInstData.miSelectedModel)->getModelFileName().c_str());

      /* cheating a bit to get buttons more to the center */
      ImGui::Indent();
      ImGui::Indent();
      if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
        modInstData.miModelDeleteCallbackFunction(modInstData.miModelList.at(modInstData.miSelectedModel)->getModelFileName().c_str());

        /* decrement selected model index to point to model that is in list before the deleted one */
        if (modInstData.miSelectedModel > 1) {
          modInstData.miSelectedModel -= 1;
        }

        /* reset model instance to first instance */
        if (modInstData.miAssimpInstances.size() > 1) {
          modInstData.miSelectedInstance = 1;
        }

        /* if we have only the null instance left, disable selection */
        if (modInstData.miAssimpInstances.size() == 1) {
          modInstData.miSelectedInstance = 0;
          renderData.rdHighlightSelectedInstance = false;
        }

        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (ImGui::Button("Create New Instance")) {
      std::shared_ptr<AssimpModel> currentModel = modInstData.miModelList[modInstData.miSelectedModel];
      modInstData.miInstanceAddCallbackFunction(currentModel);
      /* select new instance */
      modInstData.miSelectedInstance = modInstData.miAssimpInstances.size() - 1;
    }

    if (ImGui::Button("Create Multiple Instances")) {
      std::shared_ptr<AssimpModel> currentModel = modInstData.miModelList[modInstData.miSelectedModel];
      modInstData.miInstanceAddManyCallbackFunction(currentModel, mManyInstanceCreateNum);
      modInstData.miSelectedInstance = modInstData.miAssimpInstances.size() - 1;
    }
    ImGui::SameLine();
    ImGui::SliderInt("##MassInstanceCreation", &mManyInstanceCreateNum, 1, 100, "%d", flags);

    if (modelListEmtpy) {
     ImGui::EndDisabled();
    }
  }

  if (ImGui::CollapsingHeader("Instances")) {
    bool modelListEmtpy = modInstData.miModelList.size() == 1;
    bool nullInstanceSelected = modInstData.miSelectedInstance == 0;
    size_t numberOfInstances = modInstData.miAssimpInstances.size() - 1;

    ImGui::Text("Number of Instances: %ld", numberOfInstances);

    if (modelListEmtpy) {
     ImGui::BeginDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Hightlight Instance:");
    ImGui::SameLine();
    ImGui::Checkbox("##HighlightInstance", &renderData.rdHighlightSelectedInstance);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Selected Instance  :");
    ImGui::SameLine();
    ImGui::PushButtonRepeat(true);
    if (ImGui::ArrowButton("##Left", ImGuiDir_Left) &&
      modInstData.miSelectedInstance > 1) {
      modInstData.miSelectedInstance--;
    }

    if (modelListEmtpy || nullInstanceSelected) {
     ImGui::BeginDisabled();
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(30);
    ImGui::DragInt("##SelInst", &modInstData.miSelectedInstance, 1, 1,
      modInstData.miAssimpInstances.size() - 1, "%3d", flags);
    ImGui::PopItemWidth();

    if (modelListEmtpy || nullInstanceSelected) {
     ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::ArrowButton("##Right", ImGuiDir_Right) &&
      modInstData.miSelectedInstance < (modInstData.miAssimpInstances.size() - 1)) {
      modInstData.miSelectedInstance++;
    }
    ImGui::PopButtonRepeat();

    if (modelListEmtpy) {
     ImGui::EndDisabled();
    }

    if (modelListEmtpy || nullInstanceSelected) {
     ImGui::BeginDisabled();
    }

    /* DragInt does not like clamp flag */
    modInstData.miSelectedInstance = std::clamp(modInstData.miSelectedInstance, 0,
      static_cast<int>(modInstData.miAssimpInstances.size() - 1));

    InstanceSettings settings;
    if (numberOfInstances > 0) {
      settings = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getInstanceSettings();
    }

    if (ImGui::Button("Center This Instance")) {
      std::shared_ptr<AssimpInstance> currentInstance = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance);
      modInstData.miInstanceCenterCallbackFunction(currentInstance);
    }

    ImGui::SameLine();

    /* we MUST retain the last model */
    unsigned int numberOfInstancesPerModel = 0;
    if (modInstData.miAssimpInstances.size() > 1) {
      std::shared_ptr<AssimpInstance> currentInstance = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance);
      std::string currentModelName = currentInstance->getModel()->getModelFileName();
      numberOfInstancesPerModel = modInstData.miAssimpInstancesPerModel[currentModelName].size();
    }

    if (numberOfInstancesPerModel < 2) {
      ImGui::BeginDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete Instance")) {
      std::shared_ptr<AssimpInstance> currentInstance = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance);
      modInstData.miInstanceDeleteCallbackFunction(currentInstance);

      /* hard reset for now */
      if (modInstData.miSelectedInstance > 1) {
        modInstData.miSelectedInstance -= 1;
      }
      settings = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getInstanceSettings();
    }

    if (numberOfInstancesPerModel < 2) {
      ImGui::EndDisabled();
    }

    if (ImGui::Button("Clone Instance")) {
      std::shared_ptr<AssimpInstance> currentInstance = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance);
      modInstData.miInstanceCloneCallbackFunction(currentInstance);

      /* reset to last position for now */
      modInstData.miSelectedInstance = modInstData.miAssimpInstances.size() - 1;

      /* read back settings for UI */
      settings = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getInstanceSettings();
    }

    if (ImGui::Button("Create Multiple Clones")) {
      std::shared_ptr<AssimpInstance> currentInstance = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance);
      modInstData.miInstanceCloneManyCallbackFunction(currentInstance, mManyInstanceCloneNum);

      /* reset to last position for now */
      modInstData.miSelectedInstance = modInstData.miAssimpInstances.size() - 1;

      /* read back settings for UI */
      settings = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getInstanceSettings();
    }
    ImGui::SameLine();
    ImGui::SliderInt("##MassInstanceCloning", &mManyInstanceCloneNum, 1, 100, "%d", flags);

    if (modelListEmtpy || nullInstanceSelected) {
      ImGui::EndDisabled();
    }

    /* get the new size, in case of a deletion */
    numberOfInstances = modInstData.miAssimpInstances.size() - 1;

    std::string baseModelName = "None";
    if (numberOfInstances > 0 && !nullInstanceSelected) {
      baseModelName = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getModel()->getModelFileName();
    }
    ImGui::Text("Base Model: %s", baseModelName.c_str());

    if (numberOfInstances == 0 || nullInstanceSelected) {
      ImGui::BeginDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Swap Y and Z axes:     ");
    ImGui::SameLine();
    ImGui::Checkbox("##ModelAxisSwap", &settings.isSwapYZAxis);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Model Pos (X/Y/Z):     ");
    ImGui::SameLine();
    ImGui::SliderFloat3("##ModelPos", glm::value_ptr(settings.isWorldPosition),
      -25.0f, 25.0f, "%.3f", flags);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Model Rotation (X/Y/Z):");
    ImGui::SameLine();
    ImGui::SliderFloat3("##ModelRot", glm::value_ptr(settings.isWorldRotation),
      -180.0f, 180.0f, "%.3f", flags);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Model Scale:           ");
    ImGui::SameLine();
    ImGui::SliderFloat("##ModelScale", &settings.isScale,
      0.001f, 10.0f, "%.4f", flags);

    if (ImGui::Button("Reset Instance Values")) {
      InstanceSettings defaultSettings{};
      settings = defaultSettings;
    }

    if (numberOfInstances == 0 || nullInstanceSelected) {
      ImGui::EndDisabled();
    }

    if (numberOfInstances > 0) {
      modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->setInstanceSettings(settings);
    }
  }

  if (ImGui::CollapsingHeader("Animations")) {
    size_t numberOfInstances = modInstData.miAssimpInstances.size() - 1;

    InstanceSettings settings;
    size_t numberOfClips = 0;
    if (numberOfInstances > 0) {
      settings = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getInstanceSettings();
      numberOfClips = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getModel()->getAnimClips().size();
    }

    if (numberOfInstances > 0 && numberOfClips > 0) {
      std::vector<std::shared_ptr<AssimpAnimClip>> animClips = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getModel()->getAnimClips();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Animation Clip:");
      ImGui::SameLine();
      if (ImGui::BeginCombo("##ClipCombo",
        animClips.at(settings.isAnimClipNr)->getClipName().c_str())) {
        for (int i = 0; i < animClips.size(); ++i) {
          const bool isSelected = (settings.isAnimClipNr == i);
          if (ImGui::Selectable(animClips.at(i)->getClipName().c_str(), isSelected)) {
            settings.isAnimClipNr = i;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Replay Speed:  ");
      ImGui::SameLine();
      ImGui::SliderFloat("##ClipSpeed", &settings.isAnimSpeedFactor, 0.0f, 2.0f, "%.3f", flags);
    } else {
      /* TODO: better solution if no instances or no clips are found */
      ImGui::BeginDisabled();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Animation Clip:");
      ImGui::SameLine();
      ImGui::BeginCombo("##ClipComboDisabled", "None");

      float playSpeed = 1.0f;
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Replay Speed:  ");
      ImGui::SameLine();
      ImGui::SliderFloat("##ClipSpeedDisabled", &playSpeed, 0.0f, 2.0f, "%.3f", flags);

      ImGui::EndDisabled();
    }

    if (numberOfInstances > 0) {
      modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->setInstanceSettings(settings);
    }
  }

  ImGui::End();
}

void UserInterface::render() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UserInterface::cleanup() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}
