#include <string>

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
#include "AssimpSettingsContainer.h"
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

void UserInterface::createFrame(OGLRenderData &renderData, ModelAndInstanceData &modInstData, const bool ignoreMouse) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGuiWindowFlags imguiWindowFlags = 0;
  //imguiWindowFlags |= ImGuiWindowFlags_NoCollapse;
  //imguiWindowFlags |= ImGuiWindowFlags_NoResize;
  //imguiWindowFlags |= ImGuiWindowFlags_NoMove;

  /* v1.89.8 removed the check for disabled mouse cursor in GLFW
   * we need to ignore the mouse postion if the mouse lock is active */
  if (ignoreMouse) {
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
  } else {
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
  }

  ImGui::SetNextWindowBgAlpha(0.8f);

  /* remove dimming of background for modal dialogs */
  ImGuiStyle& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

  ImGui::Begin("Control", nullptr, imguiWindowFlags);

  bool loadModelRequest = false;

  bool loadConfigRequest = false;
  bool saveConfigRequest = false;

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      ImGui::MenuItem("Load Config", nullptr, &loadConfigRequest);
      if (modInstData.miModelList.size() == 1) {
        ImGui::BeginDisabled();
      }
      ImGui::MenuItem("Save Config", nullptr, &saveConfigRequest);
      if (modInstData.miModelList.size() == 1) {
        ImGui::EndDisabled();
      }
      ImGui::MenuItem("Exit", nullptr, &renderData.rdRequestApplicationExit);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (modInstData.miSettingsContainer->getUndoSize() == 0) {
        ImGui::BeginDisabled();
      }
      if (ImGui::MenuItem("Undo", "CTRL+Z")) {
        modInstData.miUndoCallbackFunction();
      }
      if (modInstData.miSettingsContainer->getUndoSize() == 0) {
        ImGui::EndDisabled();
      }

      if (modInstData.miSettingsContainer->getRedoSize() == 0) {
        ImGui::BeginDisabled();
      }
      if (ImGui::MenuItem("Redo", "CTRL+Y")) {
        modInstData.miRedoCallbackFunction();
      }
      if (modInstData.miSettingsContainer->getRedoSize() == 0) {
        ImGui::EndDisabled();
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Models")) {
      ImGui::MenuItem("Load Model...", nullptr, &loadModelRequest);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  /* application exit */
  if (renderData.rdRequestApplicationExit) {
    ImGuiFileDialog::Instance()->Close();
    ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
    ImGui::OpenPopup("Do you want to quit?");
  }

  if (ImGui::BeginPopupModal("Do you want to quit?", nullptr, ImGuiChildFlags_AlwaysAutoResize)) {
    ImGui::Text("  Exit Application?  ");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    if (ImGui::Button("OK")) {
      renderData.rdAppExitCallback();
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      renderData.rdRequestApplicationExit = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* load config */
  if (loadConfigRequest) {
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal;
    const std::string defaultFileName = "config/conf.acfg";
    config.filePathName = defaultFileName.c_str();
    ImGuiFileDialog::Instance()->OpenDialog("LoadConfigFile", "Load Configuration File",
      ".acfg", config);
  }

  bool loadSuccessful = true;
  if (ImGuiFileDialog::Instance()->Display("LoadConfigFile")) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      loadSuccessful = modInstData.miLoadConfigCallbackFunction(filePathName);
    }
    ImGuiFileDialog::Instance()->Close();
  }

  /* show error message if load was not successful */
  if (!loadSuccessful) {
    ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
    ImGui::OpenPopup("Load Error!");
  }

  if (ImGui::BeginPopupModal("Load Error!", nullptr, ImGuiChildFlags_AlwaysAutoResize)) {
    ImGui::Text("Error loading config!");
    ImGui::Text("Check console output!");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    ImGui::Indent();
    ImGui::Indent();
    if (ImGui::Button("OK")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* save config*/
  if (saveConfigRequest) {
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_ConfirmOverwrite;
    const std::string defaultFileName = "config/conf.acfg";
    config.filePathName = defaultFileName.c_str();
    ImGuiFileDialog::Instance()->OpenDialog("SaveConfigFile", "Save Configuration File",
      ".acfg", config);
  }

  bool saveSuccessful = true;
  if (ImGuiFileDialog::Instance()->Display("SaveConfigFile")) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      saveSuccessful = modInstData.miSaveConfigCallbackFunction(filePathName);
    }
    ImGuiFileDialog::Instance()->Close();
  }

  /* show error message if save was not successful */
  if (!saveSuccessful) {
    ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
    ImGui::OpenPopup("Save Error!");
  }

  if (ImGui::BeginPopupModal("Save Error!", nullptr, ImGuiChildFlags_AlwaysAutoResize)) {
    ImGui::Text("Error saving config!");
    ImGui::Text("Check console output!");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    ImGui::Indent();
    ImGui::Indent();
    if (ImGui::Button("OK")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* load model*/
  if (loadModelRequest) {
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal;
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

      if (!modInstData.miModelAddCallbackFunction(filePathName, true, true)) {
        Logger::log(1, "%s error: unable to load model file '%s', unnown error \n", __FUNCTION__, filePathName.c_str());
      }
    }
    ImGuiFileDialog::Instance()->Close();
  }

  static float newFps = 0.0f;
  /* avoid inf values (division by zero) */
  if (renderData.rdFrameTime > 0.0) {
    newFps = 1.0f / renderData.rdFrameTime * 1000.f;
  }
  /* make an averge value to avoid jumps */
  mFramesPerSecond = (mAveragingAlpha * mFramesPerSecond) + (1.0f - mAveragingAlpha) * newFps;

  /* clamp manual input on all sliders to min/max */
  ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;

  static double updateTime = 0.0;

  /* avoid literal double compares */
  if (updateTime < 0.000001) {
    updateTime = ImGui::GetTime();
  }

  static int fpsOffset = 0;
  static int frameTimeOffset = 0;
  static int modelUploadOffset = 0;
  static int matrixGenOffset = 0;
  static int matrixUploadOffset = 0;
  static int uiGenOffset = 0;
  static int uiDrawOffset = 0;

  while (updateTime < ImGui::GetTime()) {
    mFPSValues.at(fpsOffset) = mFramesPerSecond;
    fpsOffset = ++fpsOffset % mNumFPSValues;

    mFrameTimeValues.at(frameTimeOffset) = renderData.rdFrameTime;
    frameTimeOffset = ++frameTimeOffset % mNumFrameTimeValues;

    mModelUploadValues.at(modelUploadOffset) = renderData.rdUploadToVBOTime;
    modelUploadOffset = ++modelUploadOffset % mNumModelUploadValues;

    mMatrixGenerationValues.at(matrixGenOffset) = renderData.rdMatrixGenerateTime;
    matrixGenOffset = ++matrixGenOffset % mNumMatrixGenerationValues;

    mMatrixUploadValues.at(matrixUploadOffset) = renderData.rdUploadToUBOTime;
    matrixUploadOffset = ++matrixUploadOffset % mNumMatrixUploadValues;

    mUiGenValues.at(uiGenOffset) = renderData.rdUIGenerateTime;
    uiGenOffset = ++uiGenOffset % mNumUiGenValues;

    mUiDrawValues.at(uiDrawOffset) = renderData.rdUIDrawTime;
    uiDrawOffset = ++uiDrawOffset % mNumUiDrawValues;

    updateTime += 1.0 / 30.0;
  }

  ImGui::BeginGroup();
  ImGui::Text("FPS: %s", std::to_string(mFramesPerSecond).c_str());
  ImGui::EndGroup();

  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    float averageFPS = 0.0f;
    for (const auto value : mFPSValues) {
      averageFPS += value;
    }
    averageFPS /= static_cast<float>(mNumFPSValues);
    std::string fpsOverlay = "now:     " + std::to_string(mFramesPerSecond) + "\n30s avg: " + std::to_string(averageFPS);
    ImGui::Text("FPS");
    ImGui::SameLine();
    ImGui::PlotLines("##FrameTimes", mFPSValues.data(), mFPSValues.size(), fpsOffset, fpsOverlay.c_str(), 0.0f, FLT_MAX,
      ImVec2(0, 80));
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
    ImGui::BeginGroup();
    ImGui::Text("Frame Time:             %10.4f ms", renderData.rdFrameTime);
    ImGui::EndGroup();

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageFrameTime = 0.0f;
      for (const auto value : mFrameTimeValues) {
        averageFrameTime += value;
      }
      averageFrameTime /= static_cast<float>(mNumMatrixGenerationValues);
      std::string frameTimeOverlay = "now:     " + std::to_string(renderData.rdFrameTime)
        + " ms\n30s avg: " + std::to_string(averageFrameTime) + " ms";
      ImGui::Text("Frame Time       ");
      ImGui::SameLine();
      ImGui::PlotLines("##FrameTime", mFrameTimeValues.data(), mFrameTimeValues.size(), frameTimeOffset,
        frameTimeOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::BeginGroup();
    ImGui::Text("Model Upload Time:      %10.4f ms", renderData.rdUploadToVBOTime);
    ImGui::EndGroup();

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageModelUpload = 0.0f;
      for (const auto value : mModelUploadValues) {
        averageModelUpload += value;
      }
      averageModelUpload /= static_cast<float>(mNumModelUploadValues);
      std::string modelUploadOverlay = "now:     " + std::to_string(renderData.rdUploadToVBOTime)
        + " ms\n30s avg: " + std::to_string(averageModelUpload) + " ms";
      ImGui::Text("VBO Upload");
      ImGui::SameLine();
      ImGui::PlotLines("##ModelUploadTimes", mModelUploadValues.data(), mModelUploadValues.size(), modelUploadOffset,
        modelUploadOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::BeginGroup();
    ImGui::Text("Matrix Generation Time: %10.4f ms", renderData.rdMatrixGenerateTime);
    ImGui::EndGroup();

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageMatGen = 0.0f;
      for (const auto value : mMatrixGenerationValues) {
        averageMatGen += value;
      }
      averageMatGen /= static_cast<float>(mNumMatrixGenerationValues);
      std::string matrixGenOverlay = "now:     " + std::to_string(renderData.rdMatrixGenerateTime)
        + " ms\n30s avg: " + std::to_string(averageMatGen) + " ms";
      ImGui::Text("Matrix Generation");
      ImGui::SameLine();
      ImGui::PlotLines("##MatrixGenTimes", mMatrixGenerationValues.data(), mMatrixGenerationValues.size(), matrixGenOffset,
        matrixGenOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
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
      std::string matrixUploadOverlay = "now:     " + std::to_string(renderData.rdUploadToVBOTime)
        + " ms\n30s avg: " + std::to_string(averageMatrixUpload) + " ms";
      ImGui::Text("UBO Upload");
      ImGui::SameLine();
      ImGui::PlotLines("##MatrixUploadTimes", mMatrixUploadValues.data(), mMatrixUploadValues.size(), matrixUploadOffset,
        matrixUploadOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::BeginGroup();
    ImGui::Text("UI Generation Time:     %10.4f ms", renderData.rdUIGenerateTime);
    ImGui::EndGroup();

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageUiGen = 0.0f;
      for (const auto value : mUiGenValues) {
        averageUiGen += value;
      }
      averageUiGen /= static_cast<float>(mNumUiGenValues);
      std::string uiGenOverlay = "now:     " + std::to_string(renderData.rdUIGenerateTime)
        + " ms\n30s avg: " + std::to_string(averageUiGen) + " ms";
      ImGui::Text("UI Generation");
      ImGui::SameLine();
      ImGui::PlotLines("##ModelUpload", mUiGenValues.data(), mUiGenValues.size(), uiGenOffset,
        uiGenOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::BeginGroup();
    ImGui::Text("UI Draw Time:           %10.4f ms", renderData.rdUIDrawTime);
    ImGui::EndGroup();

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageUiDraw = 0.0f;
      for (const auto value : mUiDrawValues) {
        averageUiDraw += value;
      }
      averageUiDraw /= static_cast<float>(mNumUiDrawValues);
      std::string uiDrawOverlay = "now:     " + std::to_string(renderData.rdUIDrawTime)
        + " ms\n30s avg: " + std::to_string(averageUiDraw) + " ms";
      ImGui::Text("UI Draw");
      ImGui::SameLine();
      ImGui::PlotLines("##UIDrawTimes", mUiDrawValues.data(), mUiDrawValues.size(), uiGenOffset,
        uiDrawOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
      ImGui::EndTooltip();
    }
  }

  if (ImGui::CollapsingHeader("Camera")) {
    ImGui::Text("Camera Position: %s", glm::to_string(renderData.rdCameraWorldPosition).c_str());
    ImGui::Text("View Azimuth:    %6.1f", renderData.rdViewAzimuth);
    ImGui::Text("View Elevation:  %6.1f", renderData.rdViewElevation);

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

    ImGui::SameLine();
    if (ImGui::Button("Delete Model")) {
      ImGui::SetNextWindowPos(ImVec2(renderData.rdWidth / 2.0f, renderData.rdHeight / 2.0f), ImGuiCond_Always);
      ImGui::OpenPopup("Delete Model?");
    }

    if (ImGui::BeginPopupModal("Delete Model?", nullptr, ImGuiChildFlags_AlwaysAutoResize)) {
      ImGui::Text("Delete Model '%s'?", modInstData.miModelList.at(modInstData.miSelectedModel)->getModelFileName().c_str());

      /* cheating a bit to get buttons more to the center */
      ImGui::Indent();
      ImGui::Indent();
      if (ImGui::Button("OK")) {
        modInstData.miModelDeleteCallbackFunction(modInstData.miModelList.at(modInstData.miSelectedModel)->getModelFileName().c_str(), true);

        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
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

    static int manyInstanceCreateNum = 1;
    if (ImGui::Button("Create Multiple Instances")) {
      std::shared_ptr<AssimpModel> currentModel = modInstData.miModelList[modInstData.miSelectedModel];
      modInstData.miInstanceAddManyCallbackFunction(currentModel, manyInstanceCreateNum);
    }
    ImGui::SameLine();
    ImGui::SliderInt("##MassInstanceCreation", &manyInstanceCreateNum, 1, 100, "%d", flags);


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

    ImGui::Text("Hightlight Instance:");
    ImGui::SameLine();
    ImGui::Checkbox("##HighlightInstance", &renderData.rdHighlightSelectedInstance);

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

    static InstanceSettings savedInstanceSettings{};
    static std::shared_ptr<AssimpInstance> currentInstance = nullptr;

    InstanceSettings settings;
    if (numberOfInstances > 0) {
      settings = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getInstanceSettings();
      if (currentInstance != modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)) {
        currentInstance = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance);
        savedInstanceSettings = settings;
      }
    }

    if (ImGui::Button("Center This Instance")) {
      modInstData.miInstanceCenterCallbackFunction(currentInstance);
    }

    ImGui::SameLine();

    /* we MUST retain the last model */
    unsigned int numberOfInstancesPerModel = 0;
    if (modInstData.miAssimpInstances.size() > 1) {
      std::string currentModelName = currentInstance->getModel()->getModelFileName();
      numberOfInstancesPerModel = modInstData.miAssimpInstancesPerModel[currentModelName].size();
    }

    if (numberOfInstancesPerModel < 2) {
      ImGui::BeginDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete Instance")) {
      modInstData.miInstanceDeleteCallbackFunction(currentInstance, true);

      settings = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getInstanceSettings();
    }

    if (numberOfInstancesPerModel < 2) {
      ImGui::EndDisabled();
    }

    if (ImGui::Button("Clone Instance")) {
      modInstData.miInstanceCloneCallbackFunction(currentInstance);

      /* reset to last position for now */
      modInstData.miSelectedInstance = modInstData.miAssimpInstances.size() - 1;

      /* read back settings for UI */
      settings = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getInstanceSettings();
    }

    static int manyInstanceCloneNum = 1;
    if (ImGui::Button("Create Multiple Clones")) {
      modInstData.miInstanceCloneManyCallbackFunction(currentInstance, manyInstanceCloneNum);

      /* read back settings for UI */
      settings = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getInstanceSettings();
    }
    ImGui::SameLine();
    ImGui::SliderInt("##MassInstanceCloning", &manyInstanceCloneNum, 1, 100, "%d", flags);

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

    ImGui::Text("Swap Y and Z axes:     ");
    ImGui::SameLine();
    ImGui::Checkbox("##ModelAxisSwap", &settings.isSwapYZAxis);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstData.miSettingsContainer->applyEditInstanceSettings(modInstData.miAssimpInstances.at(modInstData.miSelectedInstance),
        settings, savedInstanceSettings);
      savedInstanceSettings = settings;
    }

    ImGui::Text("Model Pos (X/Y/Z):     ");
    ImGui::SameLine();
    ImGui::SliderFloat3("##ModelPos", glm::value_ptr(settings.isWorldPosition),
      -75.0f, 75.0f, "%.3f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstData.miSettingsContainer->applyEditInstanceSettings(modInstData.miAssimpInstances.at(modInstData.miSelectedInstance),
        settings, savedInstanceSettings);
      savedInstanceSettings = settings;
    }

    ImGui::Text("Model Rotation (X/Y/Z):");
    ImGui::SameLine();
    ImGui::SliderFloat3("##ModelRot", glm::value_ptr(settings.isWorldRotation),
      -180.0f, 180.0f, "%.3f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstData.miSettingsContainer->applyEditInstanceSettings(modInstData.miAssimpInstances.at(modInstData.miSelectedInstance),
        settings, savedInstanceSettings);
      savedInstanceSettings = settings;
    }

    ImGui::Text("Model Scale:           ");
    ImGui::SameLine();
    ImGui::SliderFloat("##ModelScale", &settings.isScale,
      0.001f, 10.0f, "%.4f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstData.miSettingsContainer->applyEditInstanceSettings(modInstData.miAssimpInstances.at(modInstData.miSelectedInstance),
        settings, savedInstanceSettings);
      savedInstanceSettings = settings;
    }

    if (ImGui::Button("Reset Values to Zero")) {
      modInstData.miSettingsContainer->applyEditInstanceSettings(modInstData.miAssimpInstances.at(modInstData.miSelectedInstance),
        settings, savedInstanceSettings);
      InstanceSettings defaultSettings{};
      settings = defaultSettings;
      savedInstanceSettings = settings;
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

    static InstanceSettings savedInstanceSettings{};
    static std::shared_ptr<AssimpInstance> currentInstance = nullptr;

    InstanceSettings settings;
    size_t numberOfClips = 0;

    if (numberOfInstances > 0) {
      settings = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getInstanceSettings();
      if (currentInstance != modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)) {
        currentInstance = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance);
        savedInstanceSettings = settings;
      }
      numberOfClips = modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getModel()->getAnimClips().size();
    }

    if (numberOfInstances > 0 && numberOfClips > 0) {
      std::vector<std::shared_ptr<AssimpAnimClip>> animClips =
        modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)->getModel()->getAnimClips();

      ImGui::Text("Animation Clip:");
      ImGui::SameLine();
      if (ImGui::BeginCombo("##ClipCombo",
        animClips.at(settings.isAnimClipNr)->getClipName().c_str())) {
        for (int i = 0; i < animClips.size(); ++i) {
          const bool isSelected = (settings.isAnimClipNr == i);
          if (ImGui::Selectable(animClips.at(i)->getClipName().c_str(), isSelected)) {
            settings.isAnimClipNr = i;
            /* save for undo */
            modInstData.miSettingsContainer->applyEditInstanceSettings(modInstData.miAssimpInstances.at(modInstData.miSelectedInstance),
              settings, savedInstanceSettings);
            savedInstanceSettings = settings;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }

      ImGui::Text("Replay Speed:  ");
      ImGui::SameLine();
      ImGui::SliderFloat("##ClipSpeed", &settings.isAnimSpeedFactor, 0.0f, 2.0f, "%.3f", flags);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        modInstData.miSettingsContainer->applyEditInstanceSettings(modInstData.miAssimpInstances.at(modInstData.miSelectedInstance),
          settings, savedInstanceSettings);
        savedInstanceSettings = settings;
      }
    } else {
      /* TODO: besster solution if no instances or no clips are found */
      ImGui::BeginDisabled();

      ImGui::Text("Animation Clip:");
      ImGui::SameLine();
      ImGui::BeginCombo("##ClipComboDisabled", "None");

      float playSpeed = 1.0f;
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
