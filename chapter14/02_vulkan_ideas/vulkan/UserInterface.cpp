#include <string>
#include <limits>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <misc/cpp/imgui_stdlib.h> // ImGui::InputText with std::string

#include <ImGuiFileDialog.h>

#include <imnodes.h>

#include <filesystem>

#include "UserInterface.h"
#include "CommandBuffer.h"
#include "AssimpAnimClip.h"
#include "AssimpSettingsContainer.h"
#include "ModelSettings.h"
#include "LevelSettings.h"
#include "Logger.h"
#include "BoundingBox3D.h"
#include "Tools.h"

bool UserInterface::init(VkRenderData& renderData) {
  IMGUI_CHECKVERSION();

  ImGui::CreateContext();
  ImNodes::CreateContext();

  std::vector<VkDescriptorPoolSize> imguiPoolSizes =
  {
    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
  };

  VkDescriptorPoolCreateInfo imguiPoolInfo{};
  imguiPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  imguiPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  imguiPoolInfo.maxSets = 1000;
  imguiPoolInfo.poolSizeCount = static_cast<uint32_t>(imguiPoolSizes.size());
  imguiPoolInfo.pPoolSizes = imguiPoolSizes.data();

  VkResult result = vkCreateDescriptorPool(renderData.rdVkbDevice.device, &imguiPoolInfo, nullptr, &renderData.rdImguiDescriptorPool);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not init ImGui descriptor pool \n", __FUNCTION__);
    return false;
  }

  if (!ImGui_ImplGlfw_InitForVulkan(renderData.rdWindow, true)) {
    Logger::log(1, "%s error: could not init ImGui GLFW for Vulkan \n", __FUNCTION__);
    return false;
  }

  ImGui_ImplVulkan_InitInfo imguiIinitInfo{};
  imguiIinitInfo.Instance = renderData.rdVkbInstance.instance;
  imguiIinitInfo.PhysicalDevice = renderData.rdVkbPhysicalDevice.physical_device;
  imguiIinitInfo.Device = renderData.rdVkbDevice.device;
  imguiIinitInfo.Queue = renderData.rdGraphicsQueue;
  imguiIinitInfo.DescriptorPool = renderData.rdImguiDescriptorPool;
  imguiIinitInfo.MinImageCount = 2;
  imguiIinitInfo.ImageCount = static_cast<uint32_t>(renderData.rdSwapchainImages.size());
  imguiIinitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  imguiIinitInfo.RenderPass = renderData.rdImGuiRenderpass;

  if (!ImGui_ImplVulkan_Init(&imguiIinitInfo)) {
    Logger::log(1, "%s error: could not init ImGui for Vulkan \n", __FUNCTION__);
    return false;
  }

  ImGui::StyleColorsDark();
  ImNodes::StyleColorsDark();

  /* init plot vectors */
  mFPSValues.resize(mNumFPSValues);
  mFrameTimeValues.resize(mNumFrameTimeValues);
  mModelUploadValues.resize(mNumModelUploadValues);
  mMatrixGenerationValues.resize(mNumMatrixGenerationValues);
  mMatrixUploadValues.resize(mNumMatrixUploadValues);
  mMatrixDownloadValues.resize(mNumMatrixDownloadValues);
  mUiGenValues.resize(mNumUiGenValues);
  mUiDrawValues.resize(mNumUiDrawValues);
  mCollisionDebugDrawValues.resize(mNumCollisionDebugDrawValues);
  mCollisionCheckValues.resize(mNumCollisionCheckValues);
  mNumCollisionsValues.resize(mNumNumCollisionValues);
  mBehaviorManagerValues.resize(mNumBehaviorManagerValues);
  mInteractionValues.resize(mNumInteractionValues);
  mFaceAnimValues.resize(mNumFaceAnimValues);
  mLevelCollisionCheckValues.resize(mNumLevelCollisionCheckValues);
  mIKValues.resize(mNumIKValues);
  mLevelGroundNeighborUpdateValues.resize(mNumLevelGroundNeighborUpdateValues);
  mPathFindingValues.resize(mNumPathFindingValues);

  /* Use CTRL to detach links */
  ImNodesIO& io = ImNodes::GetIO();
  io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;

  const std::string controlHelpTextFileName = "controls.txt";
  mControlsHelpText = Tools::loadFileToString(controlHelpTextFileName);
  if (mControlsHelpText.empty()) {
    Logger::log(1, "%s error: could not load controls text file '%s'\n", __FUNCTION__, controlHelpTextFileName.c_str());
  }

  return true;
}

void UserInterface::createFrame(VkRenderData &renderData) {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  /* avoid inf values (division by zero) */
  if (renderData.rdFrameTime > 0.0) {
    mNewFps = 1.0f / renderData.rdFrameTime * 1000.f;
  }

  /* make an averge value to avoid jumps */
  mFramesPerSecond = (mAveragingAlpha * mFramesPerSecond) + (1.0f - mAveragingAlpha) * mNewFps;
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

void UserInterface::createSettingsWindow(VkRenderData& renderData, ModelInstanceCamData& modInstCamData) {
  ImGuiIO& io = ImGui::GetIO();
  ImGuiWindowFlags imguiWindowFlags = 0;

  ImGui::SetNextWindowBgAlpha(0.8f);

  /* dim background for modal dialogs */
  ImGuiStyle& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.75f);

  bool loadModelRequest = false;
  bool loadLevelRequest = false;

  bool openUnsavedChangesNewDialog = false;
  bool openUnsavedChangesLoadDialog = false;
  bool openUnsavedChangesExitDialog = false;

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      ImGui::MenuItem("New Config", "CTRL+N", &renderData.rdNewConfigRequest);
      ImGui::MenuItem("Load Config", "CTRL+L", &renderData.rdLoadConfigRequest);
      if (modInstCamData.micModelList.size() == 1) {
        ImGui::BeginDisabled();
      }
      ImGui::MenuItem("Save Config", "CTRL+S", &renderData.rdSaveConfigRequest);
      if (modInstCamData.micModelList.size() == 1) {
        ImGui::EndDisabled();
      }
      ImGui::MenuItem("Exit", "CTRL+Q", &renderData.rdRequestApplicationExit);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (modInstCamData.micSettingsContainer->getUndoSize() == 0) {
        ImGui::BeginDisabled();
      }
      if (ImGui::MenuItem("Undo", "CTRL+Z")) {
        modInstCamData.micUndoCallbackFunction();
      }
      if (modInstCamData.micSettingsContainer->getUndoSize() == 0) {
        ImGui::EndDisabled();
      }

      if (modInstCamData.micSettingsContainer->getRedoSize() == 0) {
        ImGui::BeginDisabled();
      }
      if (ImGui::MenuItem("Redo", "CTRL+Y")) {
        modInstCamData.micRedoCallbackFunction();
      }
      if (modInstCamData.micSettingsContainer->getRedoSize() == 0) {
        ImGui::EndDisabled();
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Models")) {
      ImGui::MenuItem("Load Model...", nullptr, &loadModelRequest);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Levels")) {
      ImGui::MenuItem("Load Level...", nullptr, &loadLevelRequest);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Control", nullptr, &mControlWindowOpen);
      ImGui::MenuItem("Instance Positions", nullptr, &mInstancePosWindowOpen);
      ImGui::MenuItem("Status Bar", nullptr, &mStatusBarVisible);
      ImGui::EndMenu();
    }

    if (!mControlsHelpText.empty()) {
      if (ImGui::BeginMenu("Help")) {
        ImGui::MenuItem("Show Controls", "F1", &renderData.rdShowControlsHelpRequest);
        ImGui::EndMenu();
      }
    }
    ImGui::EndMainMenuBar();
  }

  /* application exit */
  if (renderData.rdRequestApplicationExit) {
    ImGuiFileDialog::Instance()->Close();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::OpenPopup("Do you want to quit?");
  }

  if (ImGui::BeginPopupModal("Do you want to quit?", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
    ImGui::Text("  Exit Application?  ");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
      if (modInstCamData.micGetConfigDirtyCallbackFunction()) {
        openUnsavedChangesExitDialog = true;
        renderData.rdRequestApplicationExit = false;
      } else {
        renderData.rdAppExitCallbackFunction();
      }
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
      renderData.rdRequestApplicationExit = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* unsaved changes, ask */
  if (openUnsavedChangesExitDialog) {
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::OpenPopup("Exit - Unsaved Changes");
  }

  if (ImGui::BeginPopupModal("Exit - Unsaved Changes", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
    ImGui::Text("You have unsaved Changes!");
    ImGui::Text("Still exit?");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
      renderData.rdAppExitCallbackFunction();
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
      renderData.rdRequestApplicationExit = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* new config */
  if (renderData.rdNewConfigRequest) {
    if (modInstCamData.micGetConfigDirtyCallbackFunction()) {
      openUnsavedChangesNewDialog = true;
    } else {
      modInstCamData.micNewConfigCallbackFunction();
    }
  }

  /* unsaved changes, ask */
  if (openUnsavedChangesNewDialog) {
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::OpenPopup("New - Unsaved Changes");
  }

  if (ImGui::BeginPopupModal("New - Unsaved Changes", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
    ImGui::Text("You have unsaved Changes!");
    ImGui::Text("Continue?");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
      modInstCamData.micNewConfigCallbackFunction();
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  const std::string defaultFileName = "config/conf.acfg";

  /* load config */
  if (renderData.rdLoadConfigRequest) {
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal;
    config.filePathName = defaultFileName;
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGuiFileDialog::Instance()->OpenDialog("LoadConfigFile", "Load Configuration File",
      ".acfg", config);
  }

  bool loadSuccessful = true;
  if (ImGuiFileDialog::Instance()->Display("LoadConfigFile")) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      if (modInstCamData.micGetConfigDirtyCallbackFunction()) {
        openUnsavedChangesLoadDialog = true;
      } else {
        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
        loadSuccessful = modInstCamData.micLoadConfigCallbackFunction(filePathName);
      }
    }
    ImGuiFileDialog::Instance()->Close();
  }

  /* ask for replacement  */
  if (openUnsavedChangesLoadDialog) {
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::OpenPopup("Load - Unsaved Changes");
  }

  if (ImGui::BeginPopupModal("Load - Unsaved Changes", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
    ImGui::Text("You have unsaved Changes!");
    ImGui::Text("Continue?");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      loadSuccessful = modInstCamData.micLoadConfigCallbackFunction(filePathName);
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* show error message if load was not successful */
  if (!loadSuccessful) {
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::OpenPopup("Load Error!");
  }

  if (ImGui::BeginPopupModal("Load Error!", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
    ImGui::Text("Error loading config!");
    ImGui::Text("Check console output!");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    ImGui::Indent();
    ImGui::Indent();
    if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* save config */
  if (renderData.rdSaveConfigRequest) {
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_ConfirmOverwrite;
    config.filePathName = defaultFileName;
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGuiFileDialog::Instance()->OpenDialog("SaveConfigFile", "Save Configuration File",
      ".acfg", config);
  }

  bool saveSuccessful = true;
  if (ImGuiFileDialog::Instance()->Display("SaveConfigFile")) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      saveSuccessful = modInstCamData.micSaveConfigCallbackFunction(filePathName);

      if (saveSuccessful) {
        modInstCamData.micSetConfigDirtyCallbackFunction(false);
      }
    }
    ImGuiFileDialog::Instance()->Close();
  }

  /* show error message if save was not successful */
  if (!saveSuccessful) {
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::OpenPopup("Save Error!");
  }

  if (ImGui::BeginPopupModal("Save Error!", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
    ImGui::Text("Error saving config!");
    ImGui::Text("Check console output!");

    /* cheating a bit to get buttons more to the center */
    ImGui::Indent();
    ImGui::Indent();
    ImGui::Indent();
    if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* load model */
  if (loadModelRequest) {
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

      if (!modInstCamData.micModelAddCallbackFunction(filePathName, true, true)) {
        Logger::log(1, "%s error: unable to load model file '%s', unnown error \n", __FUNCTION__, filePathName.c_str());
      }
    }
    ImGuiFileDialog::Instance()->Close();
  }

  /* load level */
  if (loadLevelRequest) {
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal;
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGuiFileDialog::Instance()->OpenDialog("ChooseLevelFile", "Choose Level File",
      "Supported Level Files{.gltf,.glb,.obj,.fbx,.dae,.pk3}", config);
  }

  if (ImGuiFileDialog::Instance()->Display("ChooseLevelFile")) {
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

      if (!modInstCamData.micLevelAddCallbackFunction(filePathName)) {
        Logger::log(1, "%s error: unable to load level file '%s', unnown error \n", __FUNCTION__, filePathName.c_str());
      }
    }
    ImGuiFileDialog::Instance()->Close();
  }

  /* show help text */
  if (renderData.rdShowControlsHelpRequest && !mControlsHelpText.empty()) {
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::OpenPopup("Controls");
  }

  bool helpWindowOpen = true;
  if (ImGui::BeginPopupModal("Controls", &helpWindowOpen, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
    ImGui::TextWrapped("%s", mControlsHelpText.c_str());
    ImGui::EndPopup();
  }

  /* reset values to false to avoid side-effects */
  renderData.rdNewConfigRequest = false;
  renderData.rdLoadConfigRequest = false;
  renderData.rdSaveConfigRequest = false;
  renderData.rdShowControlsHelpRequest = false;

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

    mMatrixDownloadValues.at(mMatrixDownloadOffset) = renderData.rdDownloadFromUBOTime;
    mMatrixDownloadOffset = ++mMatrixDownloadOffset % mNumMatrixDownloadValues;

    mUiGenValues.at(mUiGenOffset) = renderData.rdUIGenerateTime;
    mUiGenOffset = ++mUiGenOffset % mNumUiGenValues;

    mUiDrawValues.at(mUiDrawOffset) = renderData.rdUIDrawTime;
    mUiDrawOffset = ++mUiDrawOffset % mNumUiDrawValues;

    mCollisionDebugDrawValues.at(mCollisionDebugDrawOffset) = renderData.rdCollisionDebugDrawTime;
    mCollisionDebugDrawOffset = ++mCollisionDebugDrawOffset % mNumCollisionDebugDrawValues;

    mCollisionCheckValues.at(mCollisionCheckOffset) = renderData.rdCollisionCheckTime;
    mCollisionCheckOffset = ++mCollisionCheckOffset % mNumCollisionCheckValues;

    mNumCollisionsValues.at(mNumCollisionOffset) = renderData.rdNumberOfCollisions;
    mNumCollisionOffset = ++mNumCollisionOffset % mNumNumCollisionValues;

    mBehaviorManagerValues.at(mBehaviorManagerOffset) = renderData.rdBehaviorTime;
    mBehaviorManagerOffset = ++mBehaviorManagerOffset % mNumBehaviorManagerValues;

    mInteractionValues.at(mInteractionOffset) = renderData.rdInteractionTime;
    mInteractionOffset = ++mInteractionOffset % mNumInteractionValues;

    mFaceAnimValues.at(mFaceAnimOffset) = renderData.rdFaceAnimTime;
    mFaceAnimOffset = ++mFaceAnimOffset % mNumFaceAnimValues;

    mLevelCollisionCheckValues.at(mLevelCollisionOffset) = renderData.rdLevelCollisionTime;
    mLevelCollisionOffset = ++mLevelCollisionOffset % mNumLevelCollisionCheckValues;

    mIKValues.at(mIkOffset) = renderData.rdIKTime;
    mIkOffset = ++mIkOffset % mNumIKValues;

    mLevelGroundNeighborUpdateValues.at(mLevelGroundNeighborOffset) = renderData.rdLevelGroundNeighborUpdateTime;
    mLevelGroundNeighborOffset = ++mLevelGroundNeighborOffset % mNumLevelGroundNeighborUpdateValues;

    mPathFindingValues.at(mPathFindingOffset) = renderData.rdPathFindingTime;
    mPathFindingOffset = ++mPathFindingOffset % mNumPathFindingValues;

    mUpdateTime += 1.0 / 30.0;
  }

  /* window closed */
  if (!mControlWindowOpen) {
    return;
  }

  if (!ImGui::Begin("Control", &mControlWindowOpen, imguiWindowFlags)) {
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
    ImGui::PlotLines("##FrameTimes", mFPSValues.data(), mFPSValues.size(), mFpsOffset, fpsOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(),
      ImVec2(0, 80));
    ImGui::EndTooltip();
  }

  if (ImGui::CollapsingHeader("Info")) {
    ImGui::Text("Triangles:              %10i", renderData.rdTriangleCount);
    ImGui::Text("Level Triangles:        %10i", renderData.rdLevelTriangleCount);

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
    ImGui::Text("Frame Time:              %10.4f ms", renderData.rdFrameTime);

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

    ImGui::Text("Model Upload Time:       %10.4f ms", renderData.rdUploadToVBOTime);

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

    ImGui::Text("Matrix Generation Time:  %10.4f ms", renderData.rdMatrixGenerateTime);

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

    ImGui::Text("Matrix Upload Time:      %10.4f ms", renderData.rdUploadToUBOTime);

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

    ImGui::Text("Matrix Download Time:    %10.4f ms", renderData.rdDownloadFromUBOTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageMatrixDownload = 0.0f;
      for (const auto value : mMatrixDownloadValues) {
        averageMatrixDownload += value;
      }
      averageMatrixDownload /= static_cast<float>(mNumMatrixDownloadValues);
      std::string matrixDownloadOverlay = "now:     " + std::to_string(renderData.rdDownloadFromUBOTime) +
        " ms\n30s avg: " + std::to_string(averageMatrixDownload) + " ms";
      ImGui::Text("UBO Download");
      ImGui::SameLine();
      ImGui::PlotLines("##MatrixDownloadTimes", mMatrixDownloadValues.data(), mMatrixDownloadValues.size(), mMatrixDownloadOffset,
        matrixDownloadOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("UI Generation Time:      %10.4f ms", renderData.rdUIGenerateTime);

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

    ImGui::Text("UI Draw Time:            %10.4f ms", renderData.rdUIDrawTime);

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

    ImGui::Text("Collision Debug Draw:    %10.4f ms", renderData.rdCollisionDebugDrawTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageCollisionDebugDraw = 0.0f;
      for (const auto value : mCollisionDebugDrawValues) {
        averageCollisionDebugDraw += value;
      }
      averageCollisionDebugDraw /= static_cast<float>(mNumCollisionDebugDrawValues);
      std::string collisionDebugOverlay = "now:     " + std::to_string(renderData.rdCollisionDebugDrawTime) +
        " ms\n30s avg: " + std::to_string(averageCollisionDebugDraw) + " ms";
      ImGui::Text("Collision Debug Draw");
      ImGui::SameLine();
      ImGui::PlotLines("##CollisionDebugDrawTimes", mCollisionDebugDrawValues.data(),
        mCollisionDebugDrawValues.size(), mCollisionDebugDrawOffset,
        collisionDebugOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Collision Check Time:    %10.4f ms", renderData.rdCollisionCheckTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageCollisionCheck = 0.0f;
      for (const auto value : mCollisionCheckValues) {
        averageCollisionCheck += value;
      }
      averageCollisionCheck /= static_cast<float>(mNumCollisionCheckValues);
      std::string collisionCheckOverlay = "now:     " + std::to_string(renderData.rdCollisionCheckTime) +
        " ms\n30s avg: " + std::to_string(averageCollisionCheck) + " ms";
      ImGui::Text("Collision Check");
      ImGui::SameLine();
      ImGui::PlotLines("##CollisionCheckTimes", mCollisionCheckValues.data(),
        mCollisionCheckValues.size(), mCollisionCheckOffset,
        collisionCheckOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Behavior Update Time:    %10.4f ms", renderData.rdBehaviorTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageBehavior = 0.0f;
      for (const auto value : mBehaviorManagerValues) {
        averageBehavior += value;
      }
      averageBehavior /= static_cast<float>(mNumBehaviorManagerValues);
      std::string behaviorOverlay = "now:     " + std::to_string(renderData.rdBehaviorTime) +
        " ms\n30s avg: " + std::to_string(averageBehavior) + " ms";
      ImGui::Text("Behavior Update");
      ImGui::SameLine();
      ImGui::PlotLines("##BehaviorUpdateTimes", mBehaviorManagerValues.data(), mBehaviorManagerValues.size(), mBehaviorManagerOffset,
        behaviorOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Interaction Update Time: %10.4f ms", renderData.rdInteractionTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageInteraction = 0.0f;
      for (const auto value : mInteractionValues) {
        averageInteraction += value;
      }
      averageInteraction /= static_cast<float>(mNumInteractionValues);
      std::string interactionOverlay = "now:     " + std::to_string(renderData.rdInteractionTime) +
        " ms\n30s avg: " + std::to_string(averageInteraction) + " ms";
      ImGui::Text("Interaction Update");
      ImGui::SameLine();
      ImGui::PlotLines("##InteractionUpdateTimes", mInteractionValues.data(), mInteractionValues.size(), mInteractionOffset,
        interactionOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Face Animation Time:     %10.4f ms", renderData.rdFaceAnimTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageFaceAnim = 0.0f;
      for (const auto value : mFaceAnimValues) {
        averageFaceAnim += value;
      }
      averageFaceAnim /= static_cast<float>(mNumFaceAnimValues);
      std::string faceAnimOverlay = "now:     " + std::to_string(renderData.rdFaceAnimTime) +
        " ms\n30s avg: " + std::to_string(averageFaceAnim) + " ms";
      ImGui::Text("Face Anim Time");
      ImGui::SameLine();
      ImGui::PlotLines("##FaceAnimTimes", mFaceAnimValues.data(), mFaceAnimValues.size(), mFaceAnimOffset,
        faceAnimOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Level Collision Check:   %10.4f ms", renderData.rdLevelCollisionTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageLevelCollisionCheck = 0.0f;
      for (const auto value : mLevelCollisionCheckValues) {
        averageLevelCollisionCheck += value;
      }
      averageLevelCollisionCheck /= static_cast<float>(mNumLevelCollisionCheckValues);
      std::string levelCollisionCheckOverlay = "now:     " + std::to_string(renderData.rdLevelCollisionTime) +
        " ms\n30s avg: " + std::to_string(averageLevelCollisionCheck) + " ms";
      ImGui::Text("Level Collision Check");
      ImGui::SameLine();
      ImGui::PlotLines("##LevelCollisionCheck", mLevelCollisionCheckValues.data(),
        mLevelCollisionCheckValues.size(), mLevelCollisionOffset,
        levelCollisionCheckOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Inverse Kinematics:      %10.4f ms", renderData.rdIKTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageIk = 0.0f;
      for (const auto value : mIKValues) {
        averageIk += value;
      }
      averageIk /= static_cast<float>(mNumIKValues);
      std::string ikOverlay = "now:     " + std::to_string(renderData.rdIKTime) +
        " ms\n30s avg: " + std::to_string(averageIk) + " ms";
      ImGui::Text("Inverse Kinematics");
      ImGui::SameLine();
      ImGui::PlotLines("##InverseKinematice", mIKValues.data(), mIKValues.size(), mIkOffset,
        ikOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();

    }

    ImGui::Text("Ground Neighbor Update:  %10.4f ms", renderData.rdLevelGroundNeighborUpdateTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageNeighborUpdate = 0.0f;
      for (const auto value : mLevelGroundNeighborUpdateValues) {
        averageNeighborUpdate += value;
      }
      averageNeighborUpdate /= static_cast<float>(mNumLevelCollisionCheckValues);
      std::string neighborUpdateOverlay = "now:     " + std::to_string(renderData.rdLevelGroundNeighborUpdateTime) +
        " ms\n30s avg: " + std::to_string(averageNeighborUpdate) + " ms";
      ImGui::Text("Ground Neighbor Update");
      ImGui::SameLine();
      ImGui::PlotLines("##GroundNeighborUpdate", mLevelGroundNeighborUpdateValues.data(),
        mLevelGroundNeighborUpdateValues.size(), mLevelGroundNeighborOffset,
        neighborUpdateOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Path Finding:            %10.4f ms", renderData.rdPathFindingTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averagePathFinding = 0.0f;
      for (const auto value : mPathFindingValues) {
        averagePathFinding += value;
      }
      averagePathFinding /= static_cast<float>(mNumPathFindingValues);
      std::string pathFindingOverlay = "now:     " + std::to_string(renderData.rdPathFindingTime) +
        " ms\n30s avg: " + std::to_string(averagePathFinding) + " ms";
      ImGui::Text("Path Finding");
      ImGui::SameLine();
      ImGui::PlotLines("##PathFinding", mPathFindingValues.data(), mPathFindingValues.size(), mPathFindingOffset,
        pathFindingOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }
  }

  if (ImGui::CollapsingHeader("Music & Sound")) {
    std::vector<std::string> playlist = modInstCamData.micGetMusicPlayListCallbackFunction();
    bool audioInitialized = modInstCamData.micIsAudioManagerInitializedCallbackFunction();

    bool playlistHasEntries = !playlist.empty();
    if (!playlistHasEntries || !audioInitialized) {
      ImGui::BeginDisabled();
    }

    bool musicPlaying = modInstCamData.micIsMusicPlayingCallbackFunction();
    bool musicPaused = modInstCamData.micIsMusicPausedCallbackFunction();

    std::string currentTrack = modInstCamData.micGetMusicCurrentTrackCallbackFunction();
    const auto& iter = std::find(playlist.begin(), playlist.end(), currentTrack);
    if (iter != playlist.end()) {
      mCurrentPlaylistPos = std::distance(playlist.begin(), iter);
    }

    ImGui::Text("Tracks:       ");
    ImGui::SameLine();
    if (playlistHasEntries) {
      ImGui::PushItemWidth(300.0f);
      if (ImGui::BeginCombo("##MusicCombo",
        playlist.at(mCurrentPlaylistPos).c_str())) {
        for (int i = 0; i < playlist.size(); ++i) {
          const bool isSelected = (mCurrentPlaylistPos == i);
          if (ImGui::Selectable(playlist.at(i).c_str(), isSelected)) {
            mCurrentPlaylistPos = i;
            modInstCamData.micPlayMusicTitleCallbackFunction(playlist.at(i));
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();
    } else {
      ImGui::Text("No Music avialable");
    }

    ImGui::Text("Control:      ");
    ImGui::SameLine();

    if (!musicPlaying) {
      ImGui::BeginDisabled();
    }
    ImGui::SameLine();
    if (ImGui::Button("Prev")) {
      modInstCamData.micPlayPrevMusicTrackCallbackFunction();
    }
    if (!musicPlaying) {
      ImGui::EndDisabled();
    }

    if (musicPlaying) {
      ImGui::BeginDisabled();
    }
    ImGui::SameLine();
    if (ImGui::Button("Play")) {
      modInstCamData.micPlayMusicTitleCallbackFunction(playlist.at(mCurrentPlaylistPos));
    }

    ImGui::SameLine();
    if (ImGui::Button("Play Random")) {
      modInstCamData.micPlayRandomMusicCallbackFunction();
    }
    if (musicPlaying) {
      ImGui::EndDisabled();
    }

    if (!musicPlaying) {
      ImGui::BeginDisabled();
    }
    ImGui::SameLine();
    if (!musicPaused) {
      if (ImGui::Button("Pause")) {
        modInstCamData.micPauseResumeMusicCallbackFunction(true);
      }
    } else {
      if (ImGui::Button("Resume")) {
        modInstCamData.micPauseResumeMusicCallbackFunction(false);
      }
    }
    if (!musicPlaying) {
      ImGui::EndDisabled();
    }

    if (!musicPlaying) {
      ImGui::BeginDisabled();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
      modInstCamData.micStopMusicCallbackFunction();
    }
    if (!musicPlaying) {
      ImGui::EndDisabled();
    }

    if (!musicPlaying) {
      ImGui::BeginDisabled();
    }
    ImGui::SameLine();
    if (ImGui::Button("Next")) {
      modInstCamData.micPlayNextMusicTrackCallbackFunction();
    }
    if (!musicPlaying) {
      ImGui::EndDisabled();
    }

    int musicVolume = modInstCamData.micGetMusicVolumeCallbackFunction();
    ImGui::Text("Music Volume: ");
    ImGui::SameLine();

    ImGui::SliderInt("##MusicVolume", &musicVolume, 0, 128, "%d", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      modInstCamData.micSetMusicVolumeCallbackFunction(musicVolume);
    }

    int soundVolume = modInstCamData.micGetSoundEffectsVolumeCallbackFunction();
    ImGui::Text("Sound Volume: ");
    ImGui::SameLine();

    ImGui::SliderInt("##SoundVolume", &soundVolume, 0, 128, "%d", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      modInstCamData.micSetSoundEffectsVolumeCallbackFunction(soundVolume);
    }

    if (!playlistHasEntries) {
      ImGui::EndDisabled();
    }
  }

  if (ImGui::CollapsingHeader("Camera")) {
    std::shared_ptr<Camera> cam = modInstCamData.micCameras.at(modInstCamData.micSelectedCamera);
    CameraSettings settings = cam->getCameraSettings();

    /* overwrite saved settings on camera change */
    if (mCurrentCamera != modInstCamData.micCameras.at(modInstCamData.micSelectedCamera)) {
      mCurrentCamera = modInstCamData.micCameras.at(modInstCamData.micSelectedCamera);
      mSavedCameraSettings = settings;
      mBoneNames = cam->getBoneNames();
    }

    /* same hack as for instances */
    int numCameras = modInstCamData.micCameras.size() - 1;
    if (numCameras == 0) {
      ImGui::BeginDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Cameras:         ");
    ImGui::SameLine();

    std::string selectedCamName = "None";

    if (ImGui::ArrowButton("##CamLeft", ImGuiDir_Left) &&
      modInstCamData.micSelectedCamera > 0) {
      modInstCamData.micSelectedCamera--;
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(180.0f);
    if (ImGui::BeginCombo("##CamCombo",
      settings.csCamName.c_str())) {
      for (int i = 0; i < modInstCamData.micCameras.size(); ++i) {
        const bool isSelected = (modInstCamData.micSelectedCamera == i);
        if (ImGui::Selectable(modInstCamData.micCameras.at(i)->getName().c_str(), isSelected)) {
          modInstCamData.micSelectedCamera = i;
          selectedCamName = modInstCamData.micCameras.at(modInstCamData.micSelectedCamera)->getName();
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::ArrowButton("##CamRight", ImGuiDir_Right) &&
      modInstCamData.micSelectedCamera < modInstCamData.micCameras.size() - 1) {
      modInstCamData.micSelectedCamera++;
    }

    if (numCameras == 0) {
      ImGui::EndDisabled();
    }

    ImGui::Text("                 ");
    ImGui::SameLine();
    if (ImGui::Button("Clone Current Camera")) {
      modInstCamData.micCameraCloneCallbackFunction();
      numCameras = modInstCamData.micCameras.size() - 1;
    }

    if (numCameras == 0 || modInstCamData.micSelectedCamera == 0) {
      ImGui::BeginDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete Camera")) {
      ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
      ImGui::OpenPopup("Delete Camera?");
    }

    if (ImGui::BeginPopupModal("Delete Camera?", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
      ImGui::Text("Delete Camera '%s'?", modInstCamData.micCameras.at(modInstCamData.micSelectedCamera)->getName().c_str());

      /* cheating a bit to get buttons more to the center */
      ImGui::Indent();
      ImGui::Indent();
      ImGui::Indent();
      if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
        modInstCamData.micCameraDeleteCallbackFunction();
        numCameras = modInstCamData.micCameras.size() - 1;
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (numCameras == 0 || modInstCamData.micSelectedCamera == 0) {
      ImGui::EndDisabled();
    }

    /* Disallow changing default 'FreeCam' name or type */
    if (modInstCamData.micSelectedCamera == 0) {
      ImGui::BeginDisabled();
    }

    ImGuiInputTextFlags textinputFlags = ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter;
    std::string camName = settings.csCamName;
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Camera Name:     ");
    ImGui::SameLine();
    if (ImGui::InputText("##CamName", &camName, textinputFlags, nameInputFilter)) {
      if (modInstCamData.micCameraNameCheckCallbackFunction(camName)) {
        mShowDuplicateCamNameDialog = true;
      } else {
        settings.csCamName = camName;
        modInstCamData.micSettingsContainer->applyEditCameraSettings(
          modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
          settings, mSavedCameraSettings);
        mSavedCameraSettings = settings;
        modInstCamData.micSetConfigDirtyCallbackFunction(true);
      }
    }

    if (mShowDuplicateCamNameDialog) {
      ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
      ImGui::OpenPopup("Duplicate Camera Name");
      mShowDuplicateCamNameDialog = false;
    }

    if (ImGui::BeginPopupModal("Duplicate Camera Name", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
      ImGui::Text("Camera Name '%s' alread exists!", camName.c_str());

      /* cheating a bit to get buttons more to the center */
      ImGui::Indent();
      ImGui::Indent();
      ImGui::Indent();
      ImGui::Indent();
      ImGui::Indent();
      if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Camera Type:     ");
    ImGui::SameLine();
    ImGui::PushItemWidth(250.0f);

    if (ImGui::BeginCombo("##CamTypeCombo",
      modInstCamData.micCameraTypeMap.at(settings.csCamType).c_str())) {
      for (int i = 0; i < modInstCamData.micCameraTypeMap.size(); ++i) {
        const bool isSelected = (static_cast<int>(settings.csCamType) == i);
        if (ImGui::Selectable(modInstCamData.micCameraTypeMap[static_cast<cameraType>(i)].c_str(), isSelected)) {
          settings.csCamType = static_cast<cameraType>(i);
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    int followInstanceIndex = 0;
    std::string followInstanceId = "-";
    std::shared_ptr<AssimpInstance> followInstance = cam->getInstanceToFollow();
    if (followInstance) {
      followInstanceIndex = followInstance->getInstanceIndexPosition();
      followInstanceId = std::to_string(followInstanceIndex);
    }

    if (settings.csCamType == cameraType::firstPerson || settings.csCamType == cameraType::thirdPerson || settings.csCamType == cameraType::stationaryFollowing) {
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Following:  %4s ", followInstanceId.c_str());
      ImGui::SameLine();

      if (modInstCamData.micSelectedInstance == 0) {
        ImGui::BeginDisabled();
      }

      if (ImGui::Button("Use Selected Instance")) {
        std::shared_ptr<AssimpInstance> selectedInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
        /* this call also fills in the bone list */
        cam->setInstanceToFollow(selectedInstance);
        mBoneNames = cam->getBoneNames();

        settings = cam->getCameraSettings();
      }
      if (modInstCamData.micSelectedInstance == 0) {
        ImGui::EndDisabled();
      }

      ImGui::SameLine();
      if (!followInstance) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Button("Clear Selection")) {
        cam->clearInstanceToFollow();
        mBoneNames = cam->getBoneNames();

        settings = cam->getCameraSettings();
      }

      ImGui::Text("                 ");
      ImGui::SameLine();
      if (ImGui::Button("Selected Following Instance")) {
        modInstCamData.micSelectedInstance = followInstanceIndex;
        std::shared_ptr<AssimpInstance> selectedInstance = modInstCamData.micAssimpInstances.at(followInstanceIndex);
        /* this call also fills in the bone list */
        cam->setInstanceToFollow(selectedInstance);
        mBoneNames = cam->getBoneNames();

        settings = cam->getCameraSettings();
      }

      if (settings.csCamType == cameraType::thirdPerson && followInstance) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Distance:        ");
        ImGui::SameLine();
        ImGui::SliderFloat("##3rdPersonDistance", &settings.csThirdPersonDistance, 3.0f, 10.0f, "%.3f", flags);

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Camera Height:   ");
        ImGui::SameLine();
        ImGui::SliderFloat("##3rdPersonOffset", &settings.csThirdPersonHeightOffset, 0.0f, 3.0f, "%.3f", flags);
      }

      if (settings.csCamType == cameraType::firstPerson && followInstance) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Lock View:       ");
        ImGui::SameLine();
        ImGui::Checkbox("##1stPersonLockView", &settings.csFirstPersonLockView);

        if (!cam->getBoneNames().empty()) {
          ImGui::AlignTextToFramePadding();
          ImGui::Text("Bone to Follow:  ");
          ImGui::SameLine();
          ImGui::PushItemWidth(250.0f);

          if (ImGui::BeginCombo("##1stPersonBoneNameCombo",
            mBoneNames.at(settings.csFirstPersonBoneToFollow).c_str())) {
            for (int i = 0; i < mBoneNames.size(); ++i) {
              const bool isSelected = (settings.csFirstPersonBoneToFollow == i);
              if (ImGui::Selectable(mBoneNames.at(i).c_str(), isSelected)) {
                settings.csFirstPersonBoneToFollow = i;
              }

              if (isSelected) {
                ImGui::SetItemDefaultFocus();
              }
            }
            ImGui::EndCombo();
          }
          ImGui::PopItemWidth();
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("View Offsets:    ");
        ImGui::SameLine();
        ImGui::SliderFloat3("##1stPersonOffset", glm::value_ptr(settings.csFirstPersonOffsets), -1.0f, 1.0f, "%.3f", flags);
      }

      if (settings.csCamType == cameraType::stationaryFollowing && followInstance) {
        ImGui::Text("Camera Height:   ");
        ImGui::SameLine();
        ImGui::SliderFloat("##3rdPersonOffset", &settings.csFollowCamHeightOffset, 0.0f, 5.0f, "%.3f", flags);
      }

      if (!followInstance) {
        ImGui::EndDisabled();
      }

    }

    if (modInstCamData.micSelectedCamera == 0) {
      ImGui::EndDisabled();
    }

    /* disable settings in locked 3rd person mode */
    if (!(followInstance || settings.csCamType == cameraType::stationary)) {
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Camera Position: ");
      ImGui::SameLine();
      ImGui::SliderFloat3("##CameraPos", glm::value_ptr(settings.csWorldPosition), -125.0f, 125.0f, "%.3f", flags);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        modInstCamData.micSettingsContainer->applyEditCameraSettings(
          modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
          settings, mSavedCameraSettings);
        mSavedCameraSettings = settings;
        modInstCamData.micSetConfigDirtyCallbackFunction(true);
      }

      ImGui::AlignTextToFramePadding();
      ImGui::Text("View Azimuth:    ");
      ImGui::SameLine();
      ImGui::SliderFloat("##CamAzimuth", &settings.csViewAzimuth, 0.0f, 360.0f, "%.3f", flags);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        modInstCamData.micSettingsContainer->applyEditCameraSettings(
          modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
          settings, mSavedCameraSettings);
        mSavedCameraSettings = settings;
        modInstCamData.micSetConfigDirtyCallbackFunction(true);
      }

      ImGui::AlignTextToFramePadding();
      ImGui::Text("View Elevation:  ");
      ImGui::SameLine();
      ImGui::SliderFloat("##CamElevation", &settings.csViewElevation, -89.0f, 89.0f, "%.3f", flags);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        modInstCamData.micSettingsContainer->applyEditCameraSettings(
          modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
          settings, mSavedCameraSettings);
        mSavedCameraSettings = settings;
        modInstCamData.micSetConfigDirtyCallbackFunction(true);
      }
    } // end of locked cam type third person

    /* force projection for first and  third person cam */
    if (settings.csCamType == cameraType::firstPerson || settings.csCamType == cameraType::thirdPerson) {
      settings.csCamProjection = cameraProjection::perspective;
    }

    /* remove perspective settings in third person mode */
    if (settings.csCamType != cameraType::firstPerson && settings.csCamType != cameraType::thirdPerson) {
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Projection:      ");
      ImGui::SameLine();
      if (ImGui::RadioButton("Perspective",
        settings.csCamProjection == cameraProjection::perspective)) {
        settings.csCamProjection = cameraProjection::perspective;

        modInstCamData.micSettingsContainer->applyEditCameraSettings(
          modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
          settings, mSavedCameraSettings);
        mSavedCameraSettings = settings;
        modInstCamData.micSetConfigDirtyCallbackFunction(true);
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("Orthogonal",
        settings.csCamProjection == cameraProjection::orthogonal)) {
        settings.csCamProjection = cameraProjection::orthogonal;

        modInstCamData.micSettingsContainer->applyEditCameraSettings(
          modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
          settings, mSavedCameraSettings);
        mSavedCameraSettings = settings;
        modInstCamData.micSetConfigDirtyCallbackFunction(true);
      }
    }

    if (settings.csCamProjection == cameraProjection::orthogonal) {
      ImGui::BeginDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Field of View:   ");
    ImGui::SameLine();
    ImGui::SliderInt("##CamFOV", &settings.csFieldOfView, 40, 100, "%d", flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      Logger::log(1, "%s: old FOV is %i\n", __FUNCTION__, mSavedCameraSettings.csFieldOfView);
      Logger::log(1, "%s: new FOV is %i\n", __FUNCTION__, settings.csFieldOfView);
      modInstCamData.micSettingsContainer->applyEditCameraSettings(
        modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
        settings, mSavedCameraSettings);
      mSavedCameraSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    if (settings.csCamProjection == cameraProjection::orthogonal) {
      ImGui::EndDisabled();
    }

    /* disable orthoginal scaling in 1st and 3rd person mode, only perspective is allowed  */
    if (settings.csCamType != cameraType::firstPerson && settings.csCamType != cameraType::thirdPerson) {
      if (settings.csCamProjection == cameraProjection::perspective) {
        ImGui::BeginDisabled();
      }

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Ortho Scaling:   ");
      ImGui::SameLine();
      ImGui::SliderFloat("##CamOrthoScale", &settings.csOrthoScale, 1.0f, 50.0f, "%.3f", flags);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        modInstCamData.micSettingsContainer->applyEditCameraSettings(
          modInstCamData.micCameras.at(modInstCamData.micSelectedCamera),
          settings, mSavedCameraSettings);
        mSavedCameraSettings = settings;
        modInstCamData.micSetConfigDirtyCallbackFunction(true);
      }

      if (settings.csCamProjection == cameraProjection::perspective) {
        ImGui::EndDisabled();
      }
    }

    cam->setCameraSettings(settings);
  }

  if (ImGui::CollapsingHeader("Environment")) {
    std::shared_ptr<Camera> cam = modInstCamData.micCameras.at(modInstCamData.micSelectedCamera);
    CameraSettings camSettings = cam->getCameraSettings();

    /* Skybox and fog do not work in orthographic projection, disable controls */
    if (camSettings.csCamProjection == cameraProjection::orthogonal) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("Draw Skybox:    ");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawSkybox", &renderData.rdDrawSkybox);

    ImGui::Text("Fog Density:    ");
    ImGui::SameLine();
    ImGui::SliderFloat("##LevelFogDensity", &renderData.rdFogDensity, 0.0f, 0.1f, "%.3f", flags);

    if (camSettings.csCamProjection == cameraProjection::orthogonal) {
      ImGui::EndDisabled();
    }

    ImGui::Text("Light Angle E/W:");
    ImGui::SameLine();
    ImGui::SliderFloat("##LevelLightAngleEastWest", &renderData.rdLightSourceAngleEastWest, 0.0f, 180.0f, "%.2f", flags);

    ImGui::Text("Light Angle N/S:");
    ImGui::SameLine();
    ImGui::SliderFloat("##LevelLightAngleNorthSouth", &renderData.rdLightSourceAngleNorthSouth, 0.0f, 180.0f, "%.2f", flags);

    ImGui::Text("Light Intensity:");
    ImGui::SameLine();
    ImGui::SliderFloat("##LevelLightIntensity", &renderData.rdLightSourceIntensity, 0.0f, 1.0f, "%.2f", flags);

    ImGui::Text("RGB Light Color:");
    ImGui::SameLine();
    ImGui::SliderFloat3("##LevelLightCol", glm::value_ptr(renderData.rdLightSourceColor), 0.0f, 1.0f, "%.3f", flags);
  }

  if (ImGui::CollapsingHeader("Time of Day")) {
    ImGui::Text("Enable Time:   ");
    ImGui::SameLine();
    ImGui::Checkbox("##EnableToD", &renderData.rdEnableTimeOfDay);

    bool enableTimeOfDay = renderData.rdEnableTimeOfDay;
    if (!enableTimeOfDay) {
      ImGui::BeginDisabled();
    }

    int currentHour = static_cast<int>(renderData.rdTimeOfDay / 60);
    int currentMinute = static_cast<int>(renderData.rdTimeOfDay) % 60;
    ImGui::Text("Current Time:   %02i:%02i", currentHour, currentMinute);

    ImGui::Text("Scale:         ");
    ImGui::SameLine();
    ImGui::PushItemWidth(300.0f);
    ImGui::SliderFloat("##TimeScale", &renderData.rdTimeScaleFactor,
      0.1f, 50.0f, "%.4f", flags);
    ImGui::PopItemWidth();

    if (!enableTimeOfDay) {
      ImGui::EndDisabled();
    }

    if (enableTimeOfDay) {
      ImGui::BeginDisabled();
    }

    // preset buttons
    bool presetChanged = false;
    ImGui::Text("Light Presets:  ");
    ImGui::SameLine();
    if (ImGui::Button("Default")) {
      renderData.rdTimeOfDayPreset = timeOfDay::fullLight;
      presetChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Morning")) {
      renderData.rdTimeOfDayPreset = timeOfDay::morning;
      presetChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Noon")) {
      renderData.rdTimeOfDayPreset = timeOfDay::noon;
      presetChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Evening")) {
      renderData.rdTimeOfDayPreset = timeOfDay::evening;
      presetChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Night")) {
      renderData.rdTimeOfDayPreset = timeOfDay::midnight;
      presetChanged = true;
    }

    if (presetChanged == true ) {
      renderData.rdLightSourceAngleEastWest = renderData.rdTimeOfDayLightSettings[renderData.rdTimeOfDayPreset].lightAngleEW;
      renderData.rdLightSourceAngleNorthSouth = renderData.rdTimeOfDayLightSettings[renderData.rdTimeOfDayPreset].lightAngleNS;
      renderData.rdLightSourceIntensity = renderData.rdTimeOfDayLightSettings[renderData.rdTimeOfDayPreset].lightIntensity;
      renderData.rdLightSourceColor = renderData.rdTimeOfDayLightSettings[renderData.rdTimeOfDayPreset].lightColor;
    }

    if (enableTimeOfDay) {
      ImGui::EndDisabled();
    }
  }

  if (ImGui::CollapsingHeader("Models")) {
    /* state is changed during model deletion, so save it first */
    bool modelListEmtpy = modInstCamData.micModelList.size() == 1;
    std::string selectedModelName = "None";
    std::shared_ptr<AssimpModel> selectedModel = nullptr;
    bool modelIsStatic = true;

    if (!modelListEmtpy) {
      selectedModel = modInstCamData.micModelList.at(modInstCamData.micSelectedModel);
      selectedModelName = selectedModel->getModelFileName().c_str();
      modelIsStatic = !selectedModel->hasAnimations();
    }

    if (modelListEmtpy) {
      ImGui::BeginDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Models:          ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    if (ImGui::BeginCombo("##ModelCombo",
      /* avoid access the empty model vector or the null model name */
      selectedModelName.c_str())) {
      for (int i = 1; i < modInstCamData.micModelList.size(); ++i) {
        const bool isSelected = (modInstCamData.micSelectedModel == i);
        if (ImGui::Selectable(modInstCamData.micModelList.at(i)->getModelFileName().c_str(), isSelected)) {
          modInstCamData.micSelectedModel = i;
          selectedModelName = modInstCamData.micModelList.at(modInstCamData.micSelectedModel)->getModelFileName();
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::Text("                 ");
    ImGui::SameLine();
    if (ImGui::Button("Create New Instance")) {
      std::shared_ptr<AssimpModel> currentModel = modInstCamData.micModelList[modInstCamData.micSelectedModel];
      modInstCamData.micInstanceAddCallbackFunction(currentModel);
      /* select new instance */
      modInstCamData.micSelectedInstance = modInstCamData.micAssimpInstances.size() - 1;
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete Model")) {
      ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
      ImGui::OpenPopup("Delete Model?");
    }

    if (ImGui::BeginPopupModal("Delete Model?", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
      ImGui::Text("Delete Model '%s'?", modInstCamData.micModelList.at(modInstCamData.micSelectedModel)->getModelFileName().c_str());

      /* cheating a bit to get buttons more to the center */
      ImGui::Indent();
      ImGui::Indent();
      if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
        modInstCamData.micModelDeleteCallbackFunction(modInstCamData.micModelList.at(modInstCamData.micSelectedModel)->getModelFileName(), true);

        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::Text("Create Instances:");
    ImGui::SameLine();
    ImGui::PushItemWidth(300.0f);
    ImGui::SliderInt("##MassInstanceCreation", &mManyInstanceCreateNum, 1, 100, "%d", flags);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Go!##Create")) {
      std::shared_ptr<AssimpModel> currentModel = modInstCamData.micModelList[modInstCamData.micSelectedModel];
      modInstCamData.micInstanceAddManyCallbackFunction(currentModel, mManyInstanceCreateNum);
    }

    if (modelListEmtpy) {
     ImGui::EndDisabled();
    }

    if (modelIsStatic) {
      ImGui::BeginDisabled();
    }

    size_t numTrees = modInstCamData.micBehaviorData.size();

    if (numTrees == 0)  {
      mSelectedTreeName = "None";
      mBehaviorManager.reset();
      ImGui::BeginDisabled();
    } else {
      if (mSelectedTreeName.empty() || mSelectedTreeName == "None") {
        mSelectedTreeName = modInstCamData.micBehaviorData.begin()->first;
      }
      if (!mBehaviorManager) {
        mBehaviorManager = modInstCamData.micBehaviorData.begin()->second;
      }
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Change Template: ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    if (ImGui::BeginCombo("##ModelTreeTemplateCombo", mSelectedTreeName.c_str())) {
      for (const auto& tree : modInstCamData.micBehaviorData) {
        const bool isSelected = (tree.first == mSelectedTreeName);
        if (ImGui::Selectable(tree.first.c_str(), isSelected)) {
          mSelectedTreeName = tree.first;
          mBehaviorManager = tree.second;
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Set Template##Model")) {
      modInstCamData.micModelAddBehaviorCallbackFunction(selectedModelName, mBehaviorManager);
    }
    ImGui::SameLine();

    if (numTrees == 0) {
      ImGui::EndDisabled();
    }

    if (ImGui::Button("Clear Template##Model")) {
      modInstCamData.micModelDelBehaviorCallbackFunction(selectedModelName);
    }

    if (modelIsStatic) {
      ImGui::EndDisabled();
    }

    bool isNavTarget = false;
    if (modelListEmtpy) {
      ImGui::BeginDisabled();
    } else{
      std::shared_ptr<AssimpModel> currentModel = modInstCamData.micModelList[modInstCamData.micSelectedModel];
      isNavTarget = currentModel->isNavigationTarget();
    }
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Use as NavTarget:");
    ImGui::SameLine();
    ImGui::Checkbox("##ModelIsNavTarget", &isNavTarget);
    if (modelListEmtpy) {
      ImGui::EndDisabled();
    } else {
      std::shared_ptr<AssimpModel> currentModel = modInstCamData.micModelList[modInstCamData.micSelectedModel];
      currentModel->setAsNavigationTarget(isNavTarget);
    }
  }

  if (ImGui::CollapsingHeader("Levels")) {
    /* state is changed during model deletion, so save it first */
    bool levelListEmtpy = modInstCamData.micLevels.size() == 1;
    bool nullLevelSelected = modInstCamData.micSelectedLevel == 0;
    std::string selectedLevelName = "None";

    LevelSettings settings;
    if (!nullLevelSelected) {
      if (mCurrentLevel != modInstCamData.micLevels.at(modInstCamData.micSelectedLevel)) {
        mCurrentLevel = modInstCamData.micLevels.at(modInstCamData.micSelectedLevel);
      }
      settings = mCurrentLevel->getLevelSettings();
      selectedLevelName = mCurrentLevel->getLevelFileName().c_str();
    }

    if (levelListEmtpy) {
      ImGui::BeginDisabled();
    }

    /* map with loaded levels */
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Levels:            ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    if (ImGui::BeginCombo("##LevelCombo",
      selectedLevelName.c_str())) {
      for (int i = 1; i < modInstCamData.micLevels.size(); ++i) {
        const bool isSelected = (modInstCamData.micSelectedLevel == i);
        if (ImGui::Selectable(modInstCamData.micLevels.at(i)->getLevelFileName().c_str(), isSelected)) {
          modInstCamData.micSelectedLevel = i;
          settings = modInstCamData.micLevels.at(modInstCamData.micSelectedLevel)->getLevelSettings();
          selectedLevelName = modInstCamData.micLevels.at(modInstCamData.micSelectedLevel)->getLevelFileName();
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button("Delete Level")) {
      ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
      ImGui::OpenPopup("Delete Level?");
    }

    if (ImGui::BeginPopupModal("Delete Level?", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
      ImGui::Text("Delete Level '%s'?", modInstCamData.micLevels.at(modInstCamData.micSelectedLevel)->getLevelFileName().c_str());

      /* cheating a bit to get buttons more to the center */
      ImGui::Indent();
      ImGui::Indent();
      if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
        modInstCamData.micLevelDeleteCallbackFunction(selectedLevelName);
        settings = modInstCamData.micLevels.at(modInstCamData.micSelectedLevel)->getLevelSettings();
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    /* level settings, like instance */
    bool recreateLevelData = false;
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Swap Y/Z axes:     ");
    ImGui::SameLine();
    if (ImGui::Checkbox("##LevelAxisSwap", &settings.lsSwapYZAxis)) {
      recreateLevelData = true;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Pos (X/Y/Z):       ");
    ImGui::SameLine();
    ImGui::SliderFloat3("##LevelPos", glm::value_ptr(settings.lsWorldPosition),
      -150.0f, 150.0f, "%.3f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      recreateLevelData = true;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Rotation (X/Y/Z):  ");
    ImGui::SameLine();
    ImGui::SliderFloat3("##LevelRot", glm::value_ptr(settings.lsWorldRotation),
      -180.0f, 180.0f, "%.3f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      recreateLevelData = true;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Scale:             ");
    ImGui::SameLine();
    ImGui::SliderFloat("##LevelScale", &settings.lsScale,
      0.001f, 10.0f, "%.4f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      recreateLevelData = true;
    }

    ImGui::Text("                   ");
    ImGui::SameLine();
    if (ImGui::Button("Reset Values to Zero##Level")) {
      LevelSettings defaultSettings{};
      std::string levelFileName = settings.lsLevelFilename;
      std::string levelFileNamePath = settings.lsLevelFilenamePath;

      settings = defaultSettings;
      settings.lsLevelFilename = levelFileName;
      settings.lsLevelFilenamePath = levelFileNamePath;

      recreateLevelData = true;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Colliding Tris:    %10i", renderData.rdNumberOfCollidingTriangles);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Ground Tris:       %10i", renderData.rdNumberOfCollidingGroundTriangles);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Max Ground Slope:  ");
    ImGui::SameLine();
    ImGui::SliderFloat("##MaxSlope", &renderData.rdMaxLevelGroundSlopeAngle,
      0.0f, 45.0f, "%.2f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      recreateLevelData = true;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Max Stair Height:  ");
    ImGui::SameLine();
    ImGui::SliderFloat("##MaxStairHeight", &renderData.rdMaxStairstepHeight,
      0.1f, 3.0f, "%.2f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      recreateLevelData = true;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Simple Gravity:    ");
    ImGui::SameLine();
    ImGui::Checkbox("##EnableGravity", &renderData.rdEnableSimpleGravity);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Draw AABB:         ");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawLevelAABB", &renderData.rdDrawLevelAABB);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Draw Wireframe:    ");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawLevelWireframe", &renderData.rdDrawLevelWireframe);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Draw Wire Map:     ");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawLevelWireframeMiniMap", &renderData.rdDrawLevelWireframeMiniMap);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Draw Octree:       ");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawLevelOctree", &renderData.rdDrawLevelOctree);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Octree Max Depth:  ");
    ImGui::SameLine();
    ImGui::SliderInt("##LevelOctreeMaxDepth", &renderData.rdLevelOctreeMaxDepth, 1, 10, "%d", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      recreateLevelData = true;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Octree Threshold:  ");
    ImGui::SameLine();
    ImGui::SliderInt("##LevelOctreeThreshold", &renderData.rdLevelOctreeThreshold, 1, 20, "%d", flags);
    if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsItemActive()) {
      recreateLevelData = true;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Draw Ground Tris:  ");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawGroundTriangles", &renderData.rdDrawGroundTriangles);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Draw Collisions:   ");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawLevelCollidingTriangles", &renderData.rdDrawLevelCollisionTriangles);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Draw Neighbor Tris:");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawGroundNeihgbors", &renderData.rdDrawNeighborTriangles);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Draw Instance Path:");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawInstancePaths", &renderData.rdDrawInstancePaths);

    if (!nullLevelSelected) {
      modInstCamData.micLevels.at(modInstCamData.micSelectedLevel)->setLevelSettings(settings);
      if (recreateLevelData) {
        modInstCamData.micLevelGenerateLevelDataCallbackFunction();
      }
    }

    if (levelListEmtpy) {
     ImGui::EndDisabled();
    }
  }

  if (ImGui::CollapsingHeader("Model Idle/Walk/Run Blendings")) {
    /* close the other animation header */
    ImGui::GetStateStorage()->SetInt(ImGui::GetID("Model Animation Mappings"), 0);
    ImGui::GetStateStorage()->SetInt(ImGui::GetID("Model Allowed Clip Orders"), 0);

    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    InstanceSettings settings;
    ModelSettings modSettings;
    size_t numberOfClips = 0;

    static int clipOne = 0;
    static int clipTwo = 0;
    static int clipThree = 0;

    static float clipOneSpeed = 1.0f;
    static float clipTwoSpeed = 1.0f;
    static float clipThreeSpeed = 1.0f;

    static moveDirection direction = moveDirection::any;

    static float blendFactor = 0.0f;

    if (numberOfInstances > 0 && modInstCamData.micSelectedInstance > 0) {
      mCurrentModel = mCurrentInstance->getModel();
      settings = mCurrentInstance->getInstanceSettings();

      numberOfClips = mCurrentModel->getAnimClips().size();
      modSettings = mCurrentModel->getModelSettings();

      if (mCurrentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        mCurrentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
        mCurrentModel = mCurrentInstance->getModel();
        settings = mCurrentInstance->getInstanceSettings();

        numberOfClips = mCurrentModel->getAnimClips().size();
        modSettings = mCurrentModel->getModelSettings();

        if (!modSettings.msIWRBlendings.empty()) {
          direction = modSettings.msIWRBlendings.begin()->first;
          IdleWalkRunBlending blend = modSettings.msIWRBlendings.begin()->second;
          clipOne = blend.iwrbIdleClipNr;
          clipOneSpeed = blend.iwrbIdleClipSpeed;
          clipTwo = blend.iwrbWalkClipNr;
          clipTwoSpeed = blend.iwrbWalkClipSpeed;
          clipThree = blend.iwrbRunClipNr;
          clipThreeSpeed = blend.iwrbRunClipSpeed;
        } else {
          clipOne = 0;
          clipTwo = 0;
          clipThree = 0;

          clipOneSpeed = 1.0f;
          clipTwoSpeed = 1.0f;
          clipThreeSpeed = 1.0f;

          direction = moveDirection::any;
        }

        blendFactor = 0.0f;
        mCurrentModel->setModelSettings(modSettings);
      }
    }

    if (numberOfInstances > 0 && numberOfClips > 0) {
      std::vector<std::shared_ptr<AssimpAnimClip>> animClips =
        mCurrentInstance->getModel()->getAnimClips();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Dir: ");
      ImGui::SameLine();
      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##DirCombo",
        modInstCamData.micMoveDirectionMap.at(direction).c_str())) {
        for (int i = 0; i < modInstCamData.micMoveDirectionMap.size(); ++i) {
          if (modInstCamData.micMoveDirectionMap[static_cast<moveDirection>(i)].empty()) {
            continue;
          }
          const bool isSelected = (static_cast<int>(direction) == i);
          if (ImGui::Selectable(modInstCamData.micMoveDirectionMap[static_cast<moveDirection>(i)].c_str(), isSelected)) {
            direction = static_cast<moveDirection>(i);
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("Movement direction to configure");
      }
      ImGui::PopItemWidth();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Idle:");
      ImGui::SameLine();
      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##FirstClipCombo",
        animClips.at(clipOne)->getClipName().c_str())) {
        for (int i = 0; i < animClips.size(); ++i) {
          const bool isSelected = (clipOne == i);
          if (ImGui::Selectable(animClips.at(i)->getClipName().c_str(), isSelected)) {
            clipOne = i;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("Clip name of selected 'Idle' animation clip");
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::PushItemWidth(200.0f);
      ImGui::SliderFloat("##ClipOneSpeed", &clipOneSpeed, 0.0f, 15.0f, "%.4f", flags);
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("Replay speed of selected 'Idle' animation clip");
      }
      ImGui::PopItemWidth();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Walk:");
      ImGui::SameLine();
      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##SecondClipCombo",
        animClips.at(clipTwo)->getClipName().c_str())) {
        for (int i = 0; i < animClips.size(); ++i) {
          const bool isSelected = (clipTwo == i);
          if (ImGui::Selectable(animClips.at(i)->getClipName().c_str(), isSelected)) {
            clipTwo = i;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("Clip name of selected 'Walk' animation clip");
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::PushItemWidth(200.0f);
      ImGui::SliderFloat("##ClipTwoSpeed", &clipTwoSpeed, 0.0f, 15.0f, "%.4f", flags);
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("Replay speed of selected 'Walk' animation clip");
      }
      ImGui::PopItemWidth();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Run: ");
      ImGui::SameLine();
      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##ThirdClipCombo",
        animClips.at(clipThree)->getClipName().c_str())) {
        for (int i = 0; i < animClips.size(); ++i) {
          const bool isSelected = (clipThree == i);
          if (ImGui::Selectable(animClips.at(i)->getClipName().c_str(), isSelected)) {
            clipThree = i;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("Clip name of selected 'Run' animation clip");
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::PushItemWidth(200.0f);
      ImGui::SliderFloat("##ClipThreeSpeed", &clipThreeSpeed, 0.0f, 15.0f, "%.4f", flags);
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("Replay speed of selected 'Run' animation clip");
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      if (ImGui::Button("Save##Blending")) {
        IdleWalkRunBlending blend;
        blend.iwrbIdleClipNr = clipOne;
        blend.iwrbIdleClipSpeed = clipOneSpeed;
        blend.iwrbWalkClipNr = clipTwo;
        blend.iwrbWalkClipSpeed = clipTwoSpeed;
        blend.iwrbRunClipNr = clipThree;
        blend.iwrbRunClipSpeed = clipThreeSpeed;

        modSettings.msIWRBlendings[direction] = blend;
      }
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("Save or update the current settings");
      }

      unsigned int buttonId = 0;
      for (auto iter = modSettings.msIWRBlendings.begin(); iter != modSettings.msIWRBlendings.end(); /* done while erasing */) {
        moveDirection dir = (*iter).first;
        IdleWalkRunBlending blend = (*iter).second;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%8s: %s(%2.2f)/%s(%2.2f)/%s(%2.2f)", modInstCamData.micMoveDirectionMap[dir].c_str(),
          animClips.at(blend.iwrbIdleClipNr)->getClipName().c_str(),
          blend.iwrbIdleClipSpeed,
          animClips.at(blend.iwrbWalkClipNr)->getClipName().c_str(),
          blend.iwrbWalkClipSpeed,
          animClips.at(blend.iwrbRunClipNr)->getClipName().c_str(),
          blend.iwrbRunClipSpeed
        );

        ImGui::SameLine();
        ImGui::PushID(buttonId++);
        if (ImGui::Button("Edit##Blending")) {
          direction = dir;
          clipOne = blend.iwrbIdleClipNr;
          clipOneSpeed = blend.iwrbIdleClipSpeed;
          clipTwo = blend.iwrbWalkClipNr;
          clipTwoSpeed = blend.iwrbWalkClipSpeed;
          clipThree = blend.iwrbRunClipNr;
          clipThreeSpeed = blend.iwrbRunClipSpeed;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
          ImGui::SetTooltip("Load the settings of this blending");
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(buttonId++);
        if (ImGui::Button("Remove##Blending")) {
          iter = modSettings.msIWRBlendings.erase(iter);
        } else {
          ++iter;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
          ImGui::SetTooltip("Remove this blending");
        }
        ImGui::PopID();
      }

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Enable Preview:");
      ImGui::SameLine();
      ImGui::Checkbox("##BlendPreviewTestMode", &modSettings.msPreviewMode);

      if (!modSettings.msPreviewMode) {
        ImGui::BeginDisabled();
      }


      ImGui::AlignTextToFramePadding();
      ImGui::Text("      %-12s %14s %22s", animClips.at(clipOne)->getClipName().c_str(), animClips.at(clipTwo)->getClipName().c_str(), animClips.at(clipThree)->getClipName().c_str());
      ImGui::Text("Test:");
      ImGui::SameLine();
      ImGui::PushItemWidth(350.0f);
      ImGui::SliderFloat("##ClipBlending", &blendFactor, 0.0f, 2.0f, "", flags);
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("Select blending level between the three animation clips");
      }
      ImGui::PopItemWidth();

      if (blendFactor <= 1.0f) {
        settings.isFirstAnimClipNr = clipOne;
        settings.isSecondAnimClipNr = clipTwo;
        settings.isAnimBlendFactor = blendFactor;
        settings.isAnimSpeedFactor = glm::mix(clipOneSpeed, clipTwoSpeed, settings.isAnimBlendFactor);
      } else {
        settings.isFirstAnimClipNr = clipTwo;
        settings.isSecondAnimClipNr = clipThree;
        settings.isAnimBlendFactor = blendFactor - 1.0f;
        settings.isAnimSpeedFactor = glm::mix(clipTwoSpeed, clipThreeSpeed, settings.isAnimBlendFactor);
      }

      if (!modSettings.msPreviewMode) {
        ImGui::EndDisabled();
      }

      mCurrentInstance->setInstanceSettings(settings);
      mCurrentModel->setModelSettings(modSettings);
    }
  }

  if (ImGui::CollapsingHeader("Model Animation Mappings")) {
    /* close the other animation header */
    ImGui::GetStateStorage()->SetInt(ImGui::GetID("Model Idle/Walk/Run Blendings"), 0);
    ImGui::GetStateStorage()->SetInt(ImGui::GetID("Model Allowed Clip Orders"), 0);

    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    InstanceSettings settings;
    ModelSettings modSettings;
    size_t numberOfClips = 0;

    static moveState state = static_cast<moveState>(0);
    static int clipNr = 0;
    static float clipSpeed = 1.0f;

    if (numberOfInstances > 0 && modInstCamData.micSelectedInstance > 0) {
      mCurrentModel = mCurrentInstance->getModel();
      settings = mCurrentInstance->getInstanceSettings();

      numberOfClips = mCurrentModel->getAnimClips().size();
      modSettings = mCurrentModel->getModelSettings();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Enable Preview:");
      ImGui::SameLine();
      ImGui::Checkbox("##MapPreviewTestMode", &modSettings.msPreviewMode);

      if (mCurrentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        mCurrentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
        mCurrentModel = mCurrentInstance->getModel();
        settings = mCurrentInstance->getInstanceSettings();

        numberOfClips = mCurrentModel->getAnimClips().size();
        modSettings = mCurrentModel->getModelSettings();

        if (!modSettings.msActionClipMappings.empty()) {
          state = modSettings.msActionClipMappings.begin()->first;
          ActionAnimation savedAnim = modSettings.msActionClipMappings.begin()->second;
          clipNr = savedAnim.aaClipNr;
          clipSpeed = savedAnim.aaClipSpeed;
        } else {
          state = static_cast<moveState>(0);
          clipNr = 0;
          clipSpeed = 1.0f;
        }

        mCurrentModel->setModelSettings(modSettings);
      }
    }

    if (numberOfInstances > 0 && numberOfClips > 0) {
      std::vector<std::shared_ptr<AssimpAnimClip>> animClips =
       mCurrentInstance->getModel()->getAnimClips();

      ImGui::Text("State           Clip           Speed");
      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##MapCombo",
        modInstCamData.micMoveStateMap.at(state).c_str())) {
        /* skip idle/walk/run */
        for (int i = 3; i < static_cast<int>(moveState::NUM); ++i) {
          const bool isSelected = (static_cast<int>(state) == i);
          if (ImGui::Selectable(modInstCamData.micMoveStateMap[static_cast<moveState>(i)].c_str(), isSelected)) {
            state = static_cast<moveState>(i);
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##ActionClipCombo",
        animClips.at(clipNr)->getClipName().c_str())) {
        for (int i = 0; i < animClips.size(); ++i) {
          const bool isSelected = (clipNr == i);
          if (ImGui::Selectable(animClips.at(i)->getClipName().c_str(), isSelected)) {
            clipNr = i;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::PushItemWidth(200.0f);
      ImGui::SliderFloat("##ActionClipSpeed", &clipSpeed, 0.0f, 15.0f, "%.4f", flags);
      ImGui::PopItemWidth();

      ImGui::SameLine();
      if (ImGui::Button("Save##Action")) {
        ActionAnimation anim;
        anim.aaClipNr = clipNr;
        anim.aaClipSpeed = clipSpeed;

        modSettings.msActionClipMappings[state] = anim;
      }
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("Save or update the current acion mapping");
      }

      unsigned int buttonId = 0;
      for (auto iter = modSettings.msActionClipMappings.begin(); iter != modSettings.msActionClipMappings.end(); /* done while erasing */) {
        moveState savedState = (*iter).first;
        ActionAnimation anim = (*iter).second;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%8s: %s(%2.2f)", modInstCamData.micMoveStateMap[savedState].c_str(),
          animClips.at(anim.aaClipNr)->getClipName().c_str(),
          anim.aaClipSpeed
        );

        ImGui::SameLine();
        ImGui::PushID(buttonId++);
        if (ImGui::Button("Edit##Action")) {
          state = savedState;
          clipNr = anim.aaClipNr;
          clipSpeed = anim.aaClipSpeed;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
          ImGui::SetTooltip("Load the settings of this action mapping");
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(buttonId++);
        if (ImGui::Button("Remove##Action")) {
          iter = modSettings.msActionClipMappings.erase(iter);
        } else {
          ++iter;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
          ImGui::SetTooltip("Remove this action mapping");
        }
        ImGui::PopID();
      }

      settings.isFirstAnimClipNr = clipNr;
      settings.isSecondAnimClipNr = clipNr;
      settings.isAnimSpeedFactor = clipSpeed;
      settings.isAnimBlendFactor = 0.0f;

      mCurrentInstance->setInstanceSettings(settings);
      mCurrentModel->setModelSettings(modSettings);
    }
  }

  if (ImGui::CollapsingHeader("Model Allowed Clip Orders")) {
    /* close the other animation header */
    ImGui::GetStateStorage()->SetInt(ImGui::GetID("Model Idle/Walk/Run Blendings"), 0);
    ImGui::GetStateStorage()->SetInt(ImGui::GetID("Model Animation Mappings"), 0);

    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    ModelSettings modSettings;
    size_t numberOfClips = 0;

    static moveState stateOne = moveState::idle;
    static moveState stateTwo = moveState::idle;

    if (numberOfInstances > 0 && modInstCamData.micSelectedInstance > 0) {
      mCurrentModel = mCurrentInstance->getModel();

      numberOfClips = mCurrentModel->getAnimClips().size();
      modSettings = mCurrentModel->getModelSettings();

      if (mCurrentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        mCurrentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
        mCurrentModel = mCurrentInstance->getModel();

        numberOfClips = mCurrentModel->getAnimClips().size();
        modSettings = mCurrentModel->getModelSettings();
      }
    }

    if (numberOfInstances > 0 && numberOfClips > 0) {
      std::vector<std::shared_ptr<AssimpAnimClip>> animClips =
       mCurrentInstance->getModel()->getAnimClips();

      ImGui::Text("Source          Destination");

      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##SourceStateCombo",
        modInstCamData.micMoveStateMap.at(stateOne).c_str())) {
        for (int i = 0; i < static_cast<int>(moveState::NUM); ++i) {
          const bool isSelected = (static_cast<int>(stateOne) == i);
          if (ImGui::Selectable(modInstCamData.micMoveStateMap[static_cast<moveState>(i)].c_str(), isSelected)) {
            stateOne = static_cast<moveState>(i);
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::PushItemWidth(100.0f);
      if (ImGui::BeginCombo("##DestStateCombo",
        modInstCamData.micMoveStateMap.at(stateTwo).c_str())) {
        for (int i = 0; i < static_cast<int>(moveState::NUM); ++i) {
          const bool isSelected = (static_cast<int>(stateTwo) == i);
          if (ImGui::Selectable(modInstCamData.micMoveStateMap[static_cast<moveState>(i)].c_str(), isSelected)) {
            stateTwo = static_cast<moveState>(i);
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      if (ImGui::Button("Save##Order")) {
        std::pair<moveState, moveState> order = std::make_pair(stateOne, stateTwo);
        modSettings.msAllowedStateOrder.insert(order);
      }
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("Save or update the current clip order");
      }

      unsigned int buttonId = 0;
      for (auto iter = modSettings.msAllowedStateOrder.begin(); iter != modSettings.msAllowedStateOrder.end(); /* done while erasing */) {
        std::pair<moveState, moveState> order = *iter;

        ImGui::AlignTextToFramePadding();
        ImGui::Text("From: %s to %s (and back)", modInstCamData.micMoveStateMap.at(order.first).c_str(), modInstCamData.micMoveStateMap.at(order.second).c_str());

        ImGui::SameLine();
        ImGui::PushID(buttonId++);
        if (ImGui::Button("Edit##Order")) {
          stateOne = order.first;
          stateTwo = order.second;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
          ImGui::SetTooltip("Load this clip order");
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID(buttonId++);
        if (ImGui::Button("Remove##Order")) {
          iter = modSettings.msAllowedStateOrder.erase(iter);
        } else {
          ++iter;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
          ImGui::SetTooltip("Remove this clip order");
        }
        ImGui::PopID();
      }

      mCurrentModel->setModelSettings(modSettings);
    }
  }

  if (ImGui::CollapsingHeader("Model Head Movement Animation Mappings")) {
    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    InstanceSettings settings;
    ModelSettings modSettings;
    static int clipNr = 0;

    if (numberOfInstances > 0 && modInstCamData.micSelectedInstance > 0) {
      mCurrentModel = mCurrentInstance->getModel();
      settings = mCurrentInstance->getInstanceSettings();
      modSettings = mCurrentModel->getModelSettings();

      clipNr = 0;

      if (mCurrentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        mCurrentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
        mCurrentModel = mCurrentInstance->getModel();
        settings = mCurrentInstance->getInstanceSettings();
        modSettings = mCurrentModel->getModelSettings();
      }

      if (mCurrentModel->hasAnimations()) {
        std::vector<std::shared_ptr<AssimpAnimClip>> animClips =
         mCurrentInstance->getModel()->getAnimClips();

        /* init mapping with default values if empty */
        if (modSettings.msHeadMoveClipMappings.empty()) {
          for (int i = 0; i < static_cast<int>(headMoveDirection::NUM); ++i) {
            modSettings.msHeadMoveClipMappings[static_cast<headMoveDirection>(i)] = -1;
          }
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("       Clip:");
        ImGui::SameLine();
        ImGui::PushItemWidth(160.0f);
        if (ImGui::BeginCombo("##HeadMoveClipCombo",
          animClips.at(clipNr)->getClipName().c_str())) {
          for (int i = 0; i < animClips.size(); ++i) {
            const bool isSelected = (clipNr == i);
            if (ImGui::Selectable(animClips.at(i)->getClipName().c_str(), isSelected)) {
              clipNr = i;
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        unsigned int buttonId = 0;
        for (int i = 0; i < static_cast<int>(headMoveDirection::NUM); ++i) {
          headMoveDirection headMoveDir = static_cast<headMoveDirection>(i);
          ImGui::Text("%10s:", modInstCamData.micHeadMoveAnimationNameMap[headMoveDir].c_str());
          ImGui::AlignTextToFramePadding();

          ImGui::SameLine();
          if (modSettings.msHeadMoveClipMappings[headMoveDir] >= 0) {
            ImGui::Text("%20s", animClips.at(modSettings.msHeadMoveClipMappings[headMoveDir])->getClipName().c_str());
          } else {
            ImGui::Text("%20s", "None");
          }

          ImGui::SameLine();
          ImGui::PushID(buttonId++);
          if (ImGui::Button("Set##HeadMove")) {
            modSettings.msHeadMoveClipMappings[headMoveDir] = clipNr;
          }
          ImGui::PopID();
          ImGui::SameLine();
          ImGui::PushID(buttonId++);
          if (ImGui::Button("Remove##HeadMove")) {
            modSettings.msHeadMoveClipMappings[headMoveDir] = -1;
          }
          ImGui::PopID();
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Test Left/Right: ");
        ImGui::SameLine();
        ImGui::PushItemWidth(150.0f);
        ImGui::SliderFloat("##HeadLeftRightTest", &settings.isHeadLeftRightMove,
          -1.0f, 1.0f, "%.2f", flags);
        ImGui::PopItemWidth();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Test Up/Down:    ");
        ImGui::SameLine();
        ImGui::PushItemWidth(150.0f);
        ImGui::SliderFloat("##HeadUpDownTest", &settings.isHeadUpDownMove,
          -1.0f, 1.0f, "%.2f", flags);
        ImGui::PopItemWidth();

        mCurrentInstance->setInstanceSettings(settings);
        mCurrentModel->setModelSettings(modSettings);
      }
    }
  }

  if (ImGui::CollapsingHeader("Model Forward Speed")) {
    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    ModelSettings modSettings;

    if (numberOfInstances > 0 && modInstCamData.micSelectedInstance > 0) {
      mCurrentModel = mCurrentInstance->getModel();
      modSettings = mCurrentModel->getModelSettings();

      if (mCurrentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        mCurrentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
        mCurrentModel = mCurrentInstance->getModel();
        modSettings = mCurrentModel->getModelSettings();
      }
    }

    if (numberOfInstances > 0 && modInstCamData.micSelectedInstance > 0) {
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Forward Speed Factor:");
      ImGui::SameLine();
      ImGui::PushItemWidth(250.0f);
      ImGui::SliderFloat("##ModelForwardSpeedFactor", &modSettings.msForwardSpeedFactor,
        0.0f, 10.0f, "%.2f", flags);
      ImGui::PopItemWidth();

      mCurrentModel->setModelSettings(modSettings);
    }
  }

  if (ImGui::CollapsingHeader("Model Bounding Sphere Adjustment")) {
    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    InstanceSettings settings;
    ModelSettings modSettings;

    static std::vector<std::string> nodeNames;
    static int selectedNode = 0;
    static float adjustmentValue = 1.0f;
    static glm::vec3 positionOffset = glm::vec3(0.0f);

    if (numberOfInstances > 0 && modInstCamData.micSelectedInstance > 0) {
      mCurrentModel = mCurrentInstance->getModel();
      settings = mCurrentInstance->getInstanceSettings();
      modSettings = mCurrentModel->getModelSettings();

      nodeNames = mCurrentModel->getBoneNameList();
      selectedNode = 0;

      if (mCurrentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        mCurrentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
        mCurrentModel = mCurrentInstance->getModel();
        settings = mCurrentInstance->getInstanceSettings();
        modSettings = mCurrentModel->getModelSettings();

        nodeNames = mCurrentModel->getBoneNameList();
      }

      glm::vec4 value = modSettings.msBoundingSphereAdjustments.at(selectedNode);
      adjustmentValue = value.w;
      positionOffset = glm::vec3(value.x, value.y, value.z);

      if (!modInstCamData.micModelList.at(modInstCamData.micSelectedModel)->getBoneNameList().empty()) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Node:    ");
        ImGui::SameLine();
        ImGui::PushItemWidth(150.0f);
        if (ImGui::BeginCombo("##NodeListCombo",
          nodeNames.at(selectedNode).c_str())) {
          for (int i = 0; i < nodeNames.size(); ++i) {
            const bool isSelected = (selectedNode == i);
            if (ImGui::Selectable(nodeNames.at(i).c_str(), isSelected)) {
              selectedNode = i;

              glm::vec4 value = modSettings.msBoundingSphereAdjustments.at(selectedNode);
              adjustmentValue = value.w;
              positionOffset = glm::vec3(value.x, value.y, value.z);
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Scaling: ");
        ImGui::SameLine();
        ImGui::SliderFloat("##SphereScale", &adjustmentValue,
          0.01f, 10.0f, "%.4f", flags);

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Position:");
        ImGui::SameLine();
        ImGui::SliderFloat3("##SphereOffset", glm::value_ptr(positionOffset), -1.0f, 1.0f, "%.3f", flags);

        modSettings.msBoundingSphereAdjustments.at(selectedNode) = glm::vec4(positionOffset, adjustmentValue);
      }

      mCurrentModel->setModelSettings(modSettings);
    }
  }

  if (ImGui::CollapsingHeader("Model Feet Inverse Kinematics")) {
    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    ModelSettings modSettings;

    static std::vector<std::string> nodeNames;

    if (numberOfInstances > 0 && modInstCamData.micSelectedInstance > 0) {
      mCurrentModel = mCurrentInstance->getModel();
      modSettings = mCurrentModel->getModelSettings();

      nodeNames = mCurrentModel->getBoneNameList();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Enable IK:      ");
      ImGui::SameLine();
      ImGui::Checkbox("##FeetIK", &renderData.rdEnableFeetIK);

      if (!renderData.rdEnableFeetIK) {
        ImGui::BeginDisabled();
      }

      ImGui::AlignTextToFramePadding();
      ImGui::Text("IK Iterations:  ");
      ImGui::SameLine();
      ImGui::PushItemWidth(300.0f);
      ImGui::SliderInt("##IKIterations", &renderData.rdNumberOfIkIteratons, 1, 15, "%d", flags);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        modInstCamData.micIkIterationsCallbackFunction(renderData.rdNumberOfIkIteratons);
      }

      modSettings = mCurrentModel->getModelSettings();

      /* read out values to use shorter lines */
      int leftEffector = modSettings.msFootIKChainPair.at(0).first;
      int leftRoot = modSettings.msFootIKChainPair.at(0).second;
      int rightEffector = modSettings.msFootIKChainPair.at(1).first;
      int rightRoot = modSettings.msFootIKChainPair.at(1).second;

      bool leftFootChainChanged = false;
      bool rightFootChainChanged = false;

      if (!mCurrentModel->getBoneNameList().empty()) {
        ImGui::Text("                  Effector Node         Root Node");
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Left Foot:      ");
        ImGui::SameLine();
        ImGui::PushItemWidth(150.0f);
        if (ImGui::BeginCombo("##LeftFootEffectorCombo",
          nodeNames.at(leftEffector).c_str())) {
          for (int i = 0; i < nodeNames.size(); ++i) {
            const bool isSelected = (leftEffector == i);
            if (ImGui::Selectable(nodeNames.at(i).c_str(), isSelected)) {
              leftEffector = i;
              leftFootChainChanged = true;
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();
        ImGui::PushItemWidth(150.0f);
        if (ImGui::BeginCombo("##LeftFootRootCombo",
          nodeNames.at(leftRoot).c_str())) {
          for (int i = 0; i < nodeNames.size(); ++i) {
            const bool isSelected = (leftRoot == i);
            if (ImGui::Selectable(nodeNames.at(i).c_str(), isSelected)) {
              leftRoot = i;
              leftFootChainChanged = true;
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        bool leftChainEmpty = modSettings.msFootIKChainNodes.at(0).empty();
        if (leftChainEmpty) {
          ImGui::BeginDisabled();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear##LeftFoot")) {
          leftEffector = 0;
          leftRoot = 0;
          modSettings.msFootIKChainNodes.at(0).clear();
        }
        if (leftChainEmpty) {
          ImGui::EndDisabled();
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Right Foot:     ");
        ImGui::SameLine();
        ImGui::PushItemWidth(150.0f);
        if (ImGui::BeginCombo("##RightFootEffectorCombo",
          nodeNames.at(rightEffector).c_str())) {
          for (int i = 0; i < nodeNames.size(); ++i) {
            const bool isSelected = (rightEffector == i);
            if (ImGui::Selectable(nodeNames.at(i).c_str(), isSelected)) {
              rightEffector = i;
              rightFootChainChanged = true;
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();
        ImGui::PushItemWidth(150.0f);
        if (ImGui::BeginCombo("##RightFootRootCombo",
          nodeNames.at(rightRoot).c_str())) {
          for (int i = 0; i < nodeNames.size(); ++i) {
            const bool isSelected = (rightRoot == i);
            if (ImGui::Selectable(nodeNames.at(i).c_str(), isSelected)) {
              rightRoot = i;
              rightFootChainChanged = true;
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
      }

      bool rightChainEmpty = modSettings.msFootIKChainNodes.at(1).empty();
      if (rightChainEmpty) {
        ImGui::BeginDisabled();
      }
      ImGui::SameLine();
      if (ImGui::Button("Clear##RightFoot")) {
        rightEffector = 0;
        rightRoot = 0;
        modSettings.msFootIKChainNodes.at(1).clear();
      }
      if (rightChainEmpty) {
        ImGui::EndDisabled();
      }

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Draw Debbug:    ");
      ImGui::SameLine();
      ImGui::Checkbox("##IKDebug", &renderData.rdDrawIKDebugLines);

      /* write (possibly updated) values back */
      modSettings.msFootIKChainPair.at(0).first = leftEffector;
      modSettings.msFootIKChainPair.at(0).second = leftRoot;
      modSettings.msFootIKChainPair.at(1).first = rightEffector;
      modSettings.msFootIKChainPair.at(1).second= rightRoot;

      mCurrentModel->setModelSettings(modSettings);

      if (leftFootChainChanged) {
        mCurrentModel->setIkNodeChain(0, leftEffector, leftRoot);
      }
      if (rightFootChainChanged) {
        mCurrentModel->setIkNodeChain(1, rightEffector, rightRoot);
      }

      if (!renderData.rdEnableFeetIK) {
        ImGui::EndDisabled();
      }
    }

  }

  if (ImGui::CollapsingHeader("Instances")) {
    bool modelListEmtpy = modInstCamData.micModelList.size() == 1;
    bool nullInstanceSelected = modInstCamData.micSelectedInstance == 0;
    size_t numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    ImGui::Text("Total Instances:   %ld", numberOfInstances);

    if (modelListEmtpy) {
     ImGui::BeginDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Select Instance:  ");
    ImGui::SameLine();
    ImGui::PushButtonRepeat(true);
    if (ImGui::ArrowButton("##Left", ImGuiDir_Left) &&
      modInstCamData.micSelectedInstance > 1) {
      modInstCamData.micSelectedInstance--;
    }

    if (modelListEmtpy || nullInstanceSelected) {
     ImGui::BeginDisabled();
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(30);
    ImGui::DragInt("##SelInst", &modInstCamData.micSelectedInstance, 1, 1,
      modInstCamData.micAssimpInstances.size() - 1, "%3d", flags);
    ImGui::PopItemWidth();

    /* DragInt does not like clamp flag */
    modInstCamData.micSelectedInstance = std::clamp(modInstCamData.micSelectedInstance, 0,
      static_cast<int>(modInstCamData.micAssimpInstances.size() - 1));

    if (modelListEmtpy || nullInstanceSelected) {
     ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::ArrowButton("##Right", ImGuiDir_Right) &&
      modInstCamData.micSelectedInstance < (modInstCamData.micAssimpInstances.size() - 1)) {
      modInstCamData.micSelectedInstance++;
    }
    ImGui::PopButtonRepeat();

    if (modelListEmtpy || nullInstanceSelected) {
     ImGui::BeginDisabled();
    }

    InstanceSettings settings;
    if (numberOfInstances > 0) {
      mCurrentModel = mCurrentInstance->getModel();
      mModelHasFaceAnims = mCurrentModel->hasAnimMeshes();

      settings = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)->getInstanceSettings();
      if (mCurrentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
        mCurrentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
        mCurrentModel = mCurrentInstance->getModel();
        mModelHasFaceAnims = mCurrentModel->hasAnimMeshes();

        /* overwrite saved settings on instance change */
        mSavedInstanceSettings = settings;
      }
    }

    if (modelListEmtpy || nullInstanceSelected) {
     ImGui::EndDisabled();
    }

    if (modelListEmtpy) {
     ImGui::EndDisabled();
    }

    std::string baseModelName = "None";
    if (numberOfInstances > 0 && !nullInstanceSelected) {
      baseModelName =mCurrentInstance->getModel()->getModelFileName();
    }
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Base Model:         %s", baseModelName.c_str());


    if (modelListEmtpy || nullInstanceSelected) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("                  ");
    ImGui::SameLine();
    if (ImGui::Button("Center This Instance##Instance")) {
      modInstCamData.micInstanceCenterCallbackFunction(mCurrentInstance);
    }

    ImGui::SameLine();

    /* we MUST retain the last model */
    unsigned int numberOfInstancesPerModel = 0;
    if (modInstCamData.micAssimpInstances.size() > 1) {
      std::string currentModelName = mCurrentInstance->getModel()->getModelFileName();
      numberOfInstancesPerModel = modInstCamData.micAssimpInstancesPerModel[currentModelName].size();
    }

    if (numberOfInstancesPerModel < 2) {
      ImGui::BeginDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete Instance")) {
      modInstCamData.micInstanceDeleteCallbackFunction(mCurrentInstance, true);

      /* read back settings for UI */
      settings = mCurrentInstance->getInstanceSettings();
    }

    if (numberOfInstancesPerModel < 2) {
      ImGui::EndDisabled();
    }

    ImGui::Text("                  ");
    ImGui::SameLine();
    if (ImGui::Button("Clone Instance")) {
      modInstCamData.micInstanceCloneCallbackFunction(mCurrentInstance);

      /* read back settings for UI */
      settings = mCurrentInstance->getInstanceSettings();
    }

    ImGui::Text("Create Clones:    ");
    ImGui::SameLine();
    ImGui::PushItemWidth(300.0f);
    ImGui::SliderInt("##MassInstanceCloning", &mManyInstanceCloneNum, 1, 100, "%d", flags);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Go!##Clone")) {
      modInstCamData.micInstanceCloneManyCallbackFunction(mCurrentInstance, mManyInstanceCloneNum);

      /* read back settings for UI */
      settings = mCurrentInstance->getInstanceSettings();
    }

    /* get the new size, in case of a deletion */
    numberOfInstances = modInstCamData.micAssimpInstances.size() - 1;

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Hightlight:       ");
    ImGui::SameLine();
    ImGui::Checkbox("##HighlightInstance", &renderData.rdHighlightSelectedInstance);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Stop Movement:    ");
    ImGui::SameLine();
    ImGui::Checkbox("##StopMovement", &settings.isNoMovement);


    ImGui::AlignTextToFramePadding();
    ImGui::Text("Swap Y/Z axes:    ");
    ImGui::SameLine();
    ImGui::Checkbox("##ModelAxisSwap", &settings.isSwapYZAxis);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(
        mCurrentInstance,
        settings, mSavedInstanceSettings);
      mSavedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Pos (X/Y/Z):      ");
    ImGui::SameLine();
    ImGui::SliderFloat3("##ModelPos", glm::value_ptr(settings.isWorldPosition),
      -125.0f, 125.0f, "%.3f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(
        mCurrentInstance,
        settings, mSavedInstanceSettings);
      mSavedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Rotation (X/Y/Z): ");
    ImGui::SameLine();
    ImGui::SliderFloat3("##ModelRot", glm::value_ptr(settings.isWorldRotation),
      -180.0f, 180.0f, "%.3f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(
        mCurrentInstance,
        settings, mSavedInstanceSettings);
      mSavedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Scale:            ");
    ImGui::SameLine();
    ImGui::SliderFloat("##ModelScale", &settings.isScale,
      0.001f, 10.0f, "%.4f", flags);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(
        mCurrentInstance,
        settings, mSavedInstanceSettings);
      mSavedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    ImGui::Text("                  ");
    ImGui::SameLine();
    if (ImGui::Button("Reset Values to Zero##Instance")) {
      modInstCamData.micSettingsContainer->applyEditInstanceSettings(
        mCurrentInstance,
        settings, mSavedInstanceSettings);
      InstanceSettings defaultSettings{};

      /* save and restore index positions */
      int instanceIndex = settings.isInstanceIndexPosition;
      int modelInstanceIndex = settings.isInstancePerModelIndexPosition;
      settings = defaultSettings;
      settings.isInstanceIndexPosition = instanceIndex;
      settings.isInstancePerModelIndexPosition = modelInstanceIndex;

      mSavedInstanceSettings = settings;
      modInstCamData.micSetConfigDirtyCallbackFunction(true);
    }

    std::shared_ptr<AssimpModel> currentModel =mCurrentInstance->getModel();
    bool modelIsStatic = !currentModel->hasAnimations();

    size_t numTrees = modInstCamData.micBehaviorData.size();

    if (numTrees == 0)  {
      mSelectedTreeName = "None";
      mBehaviorManager.reset();
      ImGui::BeginDisabled();
    } else {
      if (mSelectedTreeName.empty() || mSelectedTreeName == "None") {
        mSelectedTreeName = modInstCamData.micBehaviorData.begin()->first;
      }
      if (!mBehaviorManager) {
        mBehaviorManager = modInstCamData.micBehaviorData.begin()->second;
      }
    }

    if (modelIsStatic) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("Model Template:     %s", settings.isNodeTreeName.empty() ? "None" : settings.isNodeTreeName.c_str());
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Change Template:  ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    if (ImGui::BeginCombo("##NodeTreeTemplateCombo", mSelectedTreeName.c_str())) {
      for (const auto& tree : modInstCamData.micBehaviorData) {
        const bool isSelected = (tree.first == mSelectedTreeName);
        if (ImGui::Selectable(tree.first.c_str(), isSelected)) {
          mSelectedTreeName = tree.first;
          mBehaviorManager = tree.second;
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Set Template##Instance")) {
      settings.isNodeTreeName = mSelectedTreeName;
      modInstCamData.micInstanceAddBehaviorCallbackFunction(mCurrentInstance, mBehaviorManager);
    }
    ImGui::SameLine();

    if (numTrees == 0) {
      ImGui::EndDisabled();
    }

    bool nodeTreeEmpty = settings.isNodeTreeName.empty();
    if (nodeTreeEmpty) {
      ImGui::BeginDisabled();
    }
    if (ImGui::Button("Clear Template##Instance")) {
      modInstCamData.micInstanceDelBehaviorCallbackFunction(mCurrentInstance);
      settings.isNodeTreeName.clear();

      /* change data in instance while settngs are used */
      mCurrentInstance->setInstanceSettings(settings);
      mCurrentInstance->updateInstanceState(moveState::idle, moveDirection::none);
      settings = mCurrentInstance->getInstanceSettings();
    }
    if (nodeTreeEmpty) {
      ImGui::EndDisabled();
    }

    if (modelIsStatic) {
      ImGui::EndDisabled();
    }

    ImGui::Text("Movement State:     %s", modInstCamData.micMoveStateMap.at(settings.isMoveState).c_str());

    if (!mModelHasFaceAnims) {
      ImGui::BeginDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Face Anim Clip:   ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    if (ImGui::BeginCombo("##FaceAnimClipCombo", modInstCamData.micFaceAnimationNameMap.at(settings.isFaceAnimType).c_str())) {
      for (unsigned int i = 0; i < modInstCamData.micFaceAnimationNameMap.size(); ++i) {
        const bool isSelected = (static_cast<int>(settings.isFaceAnimType) == i);

        if (ImGui::Selectable(modInstCamData.micFaceAnimationNameMap.at(static_cast<faceAnimation>(i)).c_str(), isSelected)) {
          settings.isFaceAnimWeight = 0.0f;
          settings.isFaceAnimType = static_cast<faceAnimation>(i);
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();


    ImGui::AlignTextToFramePadding();
    ImGui::Text("MorphAnim Weight: ");
    ImGui::SameLine();
    ImGui::SliderFloat("##MorphAnimWeight", &settings.isFaceAnimWeight, 0.0f, 1.0f, "%.2f", flags);

    if (!mModelHasFaceAnims) {
      ImGui::EndDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Ground Tri:      %10i", settings.isCurrentGroundTriangleIndex);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Neighbor Tris:   %10li", settings.isNeighborGroundTriangles.size());

    std::vector<int> navTargets = modInstCamData.micGetNavTargetsCallbackFunction();
    size_t numNavTargets = navTargets.size();

    if (mSelectedNavTarget > numNavTargets) {
      mSelectedNavTarget = 0;
    }

    if (numNavTargets == 0 || modelIsStatic) {
      ImGui::BeginDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Enable Navigation:");
    ImGui::SameLine();
    ImGui::Checkbox("##EnableNavInstance", &settings.isNavigationEnabled);

    if (!settings.isNavigationEnabled) {
      ImGui::BeginDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Nav Target:      %10i", settings.isPathTargetInstance);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Nav Targets:      ");
    ImGui::SameLine();

    if (numNavTargets > 0) {
      ImGui::PushItemWidth(250.0f);
      if (ImGui::BeginCombo("##NavTargetCombo",
        std::to_string(navTargets.at(mSelectedNavTarget)).c_str())) {
        for (int i = 0; i < numNavTargets; ++i) {
          const bool isSelected = (mSelectedNavTarget == i);
          if (ImGui::Selectable(std::to_string(navTargets.at(i)).c_str(), isSelected)) {
            mSelectedNavTarget = i;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      ImGui::SameLine();

      if (ImGui::Button("Set##Target")) {
        settings.isPathTargetInstance = navTargets.at(mSelectedNavTarget);
      }
      ImGui::SameLine();

      bool noTargetSelected = (settings.isPathTargetInstance == -1);
      if (noTargetSelected) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Button("Clear##Target")) {
        settings.isPathTargetInstance = -1;
      }
      if (noTargetSelected) {
        ImGui::EndDisabled();
      }

      ImGui::Text("                  ");
      ImGui::SameLine();
      if (ImGui::Button("Center Target##NavTarget")) {
        std::shared_ptr<AssimpInstance> instance =
          modInstCamData.micAssimpInstances.at(navTargets.at(mSelectedNavTarget));
        modInstCamData.micInstanceCenterCallbackFunction(instance);
      }
    } else {
      ImGui::Text("None");
    }

    if (!settings.isNavigationEnabled) {
      ImGui::EndDisabled();
    }

    if (numNavTargets == 0 || modelIsStatic) {
      ImGui::EndDisabled();
    }

    if (numberOfInstances == 0 || nullInstanceSelected) {
      ImGui::EndDisabled();
    }

    if (numberOfInstances > 0) {
      mCurrentInstance->setInstanceSettings(settings);
    }
  }

  if (ImGui::CollapsingHeader("Node Tree Template")) {
    ImGuiInputTextFlags textinputFlags = ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter;

    bool showDuplicateNameDialog = false;

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Tree Template Name: ");
    ImGui::SameLine();
    ImGui::PushItemWidth(150.0f);
    if (ImGui::InputText("##TreeTemplateName", &mNewTreeName, textinputFlags, nameInputFilter)) {
      if (modInstCamData.micBehaviorData.count(mNewTreeName) > 0) {
        showDuplicateNameDialog = true;
      }
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Create Tree Template")) {
      if (modInstCamData.micBehaviorData.count(mNewTreeName) > 0) {
        showDuplicateNameDialog = true;
      } else {
        modInstCamData.micBehaviorData[mNewTreeName] = modInstCamData.micCreateEmptyNodeGraphCallbackFunction();
        modInstCamData.micBehaviorData[mNewTreeName]->getBehaviorData()->bdName = mNewTreeName;
      }
    }

    if (showDuplicateNameDialog) {
      ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
      ImGui::OpenPopup("Duplicate Tree Template Name");
      showDuplicateNameDialog = false;
    }

    if (ImGui::BeginPopupModal("Duplicate Tree Template Name", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
      ImGui::Text("Tree Template Name '%s' alread exists!", mNewTreeName.c_str());

      /* cheating a bit to get buttons more to the center */
      ImGui::Indent();
      ImGui::Indent();
      ImGui::Indent();
      ImGui::Indent();
      ImGui::Indent();
      if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    unsigned int buttonId = 0;
    bool showDeleteRequest = false;

    for (auto iter = modInstCamData.micBehaviorData.begin(); iter != modInstCamData.micBehaviorData.end(); /* done while erasing */) {
      std::string treeName = (*iter).first;
      std::shared_ptr<BehaviorData> treeData = (*iter).second->getBehaviorData();

      size_t nodeSize = treeData->bdGraphNodes.size();
      size_t linkSize = treeData->bdGraphLinks.size();
      ImGui::AlignTextToFramePadding();
      ImGui::Text("%8s: %lu node%s, %lu link%s", treeName.c_str(), nodeSize, nodeSize == 1 ? "" : "s",
                  linkSize, linkSize == 1 ? "" : "s");

      ImGui::SameLine();
      ImGui::PushID(buttonId++);
      if (ImGui::Button("Edit Template##TreeTemplate")) {
        modInstCamData.micEditNodeGraphCallbackFunction(treeName);
      }
      ImGui::PopID();
      ImGui::SameLine();
      ImGui::PushID(buttonId++);
      if (ImGui::Button("Remove Template##TreeTemplate")) {
        /* delete empty trees without rquest */
        if (nodeSize > 1) {
          mTreeToDelete = treeName;
          showDeleteRequest = true;
          ++iter;
        } else {
          iter = modInstCamData.micBehaviorData.erase(iter);
          modInstCamData.micPostNodeTreeDelBehaviorCallbackFunction(treeName);
        }
      } else {
        ++iter;
      }
      ImGui::PopID();
    }

    if (showDeleteRequest) {
      ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
      ImGui::OpenPopup("Delete Tree Template?");
      showDeleteRequest = false;
    }

    if (ImGui::BeginPopupModal("Delete Tree Template?", nullptr, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
      ImGui::Text(" Delete Tree Template '%s'?  ", mTreeToDelete.c_str());

      /* cheating a bit to get buttons more to the center */
      ImGui::Indent();
      if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
        modInstCamData.micBehaviorData.erase(mTreeToDelete);
        modInstCamData.micPostNodeTreeDelBehaviorCallbackFunction(mTreeToDelete);
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();
      if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }

  if (ImGui::CollapsingHeader("Collisions")) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Number of Collisions:  %4li", renderData.rdNumberOfCollisions);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      int averageNumCollisions = 0;
      for (const auto value : mNumCollisionsValues) {
        averageNumCollisions += value;
      }
      averageNumCollisions /= static_cast<float>(mNumNumCollisionValues);
      std::string numCollisionsOverlay = "now:     " + std::to_string(renderData.rdNumberOfCollisions) +
        "\n30s avg: " + std::to_string(averageNumCollisions);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Collisions");
      ImGui::SameLine();
      ImGui::PlotLines("##NumCollisions", mNumCollisionsValues.data(), mNumCollisionsValues.size(), mNumCollisionOffset,
        numCollisionsOverlay.c_str(), 0.0f, std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Collisions:             ");
    ImGui::SameLine();
    if (ImGui::RadioButton("None##CollCheck",
      renderData.rdCheckCollisions == collisionChecks::none)) {
      renderData.rdCheckCollisions = collisionChecks::none;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("2D Bounding Box##CollCheck",
      renderData.rdCheckCollisions == collisionChecks::boundingBox)) {
      renderData.rdCheckCollisions = collisionChecks::boundingBox;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Bounding Spheres##CollCheck",
      renderData.rdCheckCollisions == collisionChecks::boundingSpheres)) {
      renderData.rdCheckCollisions = collisionChecks::boundingSpheres;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Draw AABB Lines:        ");
    ImGui::SameLine();
    if (ImGui::RadioButton("None##AABB",
      renderData.rdDrawCollisionAABBs == collisionDebugDraw::none)) {
      renderData.rdDrawCollisionAABBs = collisionDebugDraw::none;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Colliding##AABB",
      renderData.rdDrawCollisionAABBs == collisionDebugDraw::colliding)) {
      renderData.rdDrawCollisionAABBs = collisionDebugDraw::colliding;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("All##AABB",
      renderData.rdDrawCollisionAABBs == collisionDebugDraw::all)) {
      renderData.rdDrawCollisionAABBs = collisionDebugDraw::all;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Draw Bounding Spheres:  ");
    ImGui::SameLine();
    if (ImGui::RadioButton("None##Sphere",
      renderData.rdDrawBoundingSpheres == collisionDebugDraw::none)) {
      renderData.rdDrawBoundingSpheres = collisionDebugDraw::none;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Colliding##Sphere",
      renderData.rdDrawBoundingSpheres == collisionDebugDraw::colliding)) {
      renderData.rdDrawBoundingSpheres = collisionDebugDraw::colliding;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Selected##Sphere",
      renderData.rdDrawBoundingSpheres == collisionDebugDraw::selected)) {
      renderData.rdDrawBoundingSpheres = collisionDebugDraw::selected;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("All##Sphere",
      renderData.rdDrawBoundingSpheres == collisionDebugDraw::all)) {
      renderData.rdDrawBoundingSpheres = collisionDebugDraw::all;
    }
  }

  if (ImGui::CollapsingHeader("Interaction")) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Interaction:           ");
    ImGui::SameLine();
    ImGui::Checkbox("##EnableInteraction", &renderData.rdInteraction);

    if (!renderData.rdInteraction) {
      ImGui::BeginDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Number Of Candidates:   %li", renderData.rdNumberOfInteractionCandidates);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Interaction Candidate:  %i", renderData.rdInteractWithInstanceId);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Min Interaction Range: ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    ImGui::SliderFloat("##MinInteractionRange", &renderData.rdInteractionMinRange, 0.0f, 20.0f, "%.3f", flags);
    ImGui::PopItemWidth();

    if (renderData.rdInteractionMinRange > renderData.rdInteractionMaxRange) {
      renderData.rdInteractionMaxRange = renderData.rdInteractionMinRange;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Max Interaction Range: ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    ImGui::SliderFloat("##MaxInteractionRange", &renderData.rdInteractionMaxRange, 0.0f, 20.0f, "%.3f", flags);
    ImGui::PopItemWidth();

    if (renderData.rdInteractionMaxRange < renderData.rdInteractionMinRange) {
      renderData.rdInteractionMinRange = renderData.rdInteractionMaxRange;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Interaction FOV:       ");
    ImGui::SameLine();
    ImGui::PushItemWidth(200.0f);
    ImGui::SliderFloat("##InteractionFOV", &renderData.rdInteractionFOV, 30.0f, 60.0f, "%.3f", flags);
    ImGui::PopItemWidth();

    ImGui::NewLine();

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Draw Interaction Range:");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawInteractionRange", &renderData.rdDrawInteractionRange);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Draw Interaction FOV:  ");
    ImGui::SameLine();
    ImGui::Checkbox("##DrawInteractionFOV", &renderData.rdDrawInteractionFOV);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Draw Interaction Debug:");
    ImGui::SameLine();
    if (ImGui::RadioButton("None##Interaction",
      renderData.rdDrawInteractionAABBs == interactionDebugDraw::none)) {
      renderData.rdDrawInteractionAABBs = interactionDebugDraw::none;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("All in Range##Interaction",
      renderData.rdDrawInteractionAABBs == interactionDebugDraw::distance)) {
      renderData.rdDrawInteractionAABBs = interactionDebugDraw::distance;
    }
    ImGui::Text("                       ");
    ImGui::SameLine();
    if (ImGui::RadioButton("Correct Facing##Interaction",
      renderData.rdDrawInteractionAABBs == interactionDebugDraw::facingTowardsUs)) {
      renderData.rdDrawInteractionAABBs = interactionDebugDraw::facingTowardsUs;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Nearest Candidata##Interaction",
      renderData.rdDrawInteractionAABBs == interactionDebugDraw::nearestCandidate)) {
      renderData.rdDrawInteractionAABBs = interactionDebugDraw::nearestCandidate;
    }

    if (!renderData.rdInteraction) {
      ImGui::EndDisabled();
    }
  }

  if (ImGui::CollapsingHeader("Navigation")) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Enable Navigation:     ");
    ImGui::SameLine();
    ImGui::Checkbox("##EnableNavGlobal", &renderData.rdEnableNavigation);
  }

  ImGui::End();
}

void UserInterface::createPositionsWindow(VkRenderData& renderData, ModelInstanceCamData& modInstCamData) {
  std::shared_ptr<BoundingBox3D> worldBoundaries = modInstCamData.micWorldGetBoundariesCallbackFunction();
  /* window closed */
  if (!mInstancePosWindowOpen) {
    return;
  }

  ImGuiWindowFlags posWinFlags = 0;
  ImGui::SetNextWindowBgAlpha(0.5f);

  if (!ImGui::Begin("Instance Positions", &mInstancePosWindowOpen, posWinFlags)) {
    /* window collapsed */
    ImGui::End();
    return;
  }

  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
    /* zoom in/out with mouse wheel */
    ImGuiIO& io = ImGui::GetIO();
    mOctreeZoomFactor += 0.025f * io.MouseWheel;
    mOctreeZoomFactor = std::clamp(mOctreeZoomFactor, 0.1f, 5.0f);

    /* rotate octree view when right mouse button is pressed */
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
      mOctreeRotation.y += io.MouseDelta.x;
      mOctreeRotation.x += io.MouseDelta.y;
    }

    /* move octree view when middle mouse button is pressed */
    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
      mOctreeTranslation.x += io.MouseDelta.x;
      mOctreeTranslation.y += io.MouseDelta.y;
    }
  }

  glm::vec4 red = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
  glm::vec4 yellow = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
  glm::vec4 green = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
  glm::vec4 white = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

  mOctreeLines.vertices.clear();
  /* draw octree boxes first */
  const std::vector<BoundingBox3D> treeBoxes = modInstCamData.micOctreeGetBoxesCallbackFunction();
  for (const auto& box : treeBoxes) {
    AABB boxAABB{};
    boxAABB.create(box.getFrontTopLeft());
    boxAABB.addPoint(box.getFrontTopLeft() + box.getSize());

    std::shared_ptr<VkLineMesh> instanceLines = boxAABB.getAABBLines(white);
    mOctreeLines.vertices.insert(mOctreeLines.vertices.end(), instanceLines->vertices.begin(), instanceLines->vertices.end());
  }

  /* draw instance AABBs second */
  for (const auto& instance : modInstCamData.micAssimpInstances) {
    InstanceSettings instSettings = instance->getInstanceSettings();
    int instanceId = instSettings.isInstanceIndexPosition;
    /* skip null instance */
    if (instanceId == 0) {
      continue;
    }

    AABB instanceAABB = instance->getModel()->getAABB(instSettings);

    const auto iter = std::find_if(modInstCamData.micInstanceCollisions.begin(), modInstCamData.micInstanceCollisions.end(),
      [instanceId](std::pair<int, int> values) {
        return instanceId == values.first || instanceId == values.second;
      });

    std::shared_ptr<VkLineMesh> instanceLines = nullptr;
    if (iter != modInstCamData.micInstanceCollisions.end()) {
      /* colliding instances in red */
      instanceLines = instanceAABB.getAABBLines(red);
    } else {
      /* all other instances in yellow */
      instanceLines = instanceAABB.getAABBLines(yellow);
    }
    mOctreeLines.vertices.insert(mOctreeLines.vertices.end(), instanceLines->vertices.begin(), instanceLines->vertices.end());

    /* draw a green box around the selected instance */
    if (modInstCamData.micSelectedInstance == instanceId) {
      instanceAABB.setMinPos(instanceAABB.getMinPos() - glm::vec3(1.0f));
      instanceAABB.setMaxPos(instanceAABB.getMaxPos() + glm::vec3(1.0f));
      instanceLines = instanceAABB.getAABBLines(green);
    }
    mOctreeLines.vertices.insert(mOctreeLines.vertices.end(), instanceLines->vertices.begin(), instanceLines->vertices.end());
  }

  ImDrawList* drawList = ImGui::GetWindowDrawList();

  ImVec2 cursorPos = ImGui::GetCursorScreenPos();
  ImVec2 windowSize = ImGui::GetWindowSize();

  ImVec2 drawArea = ImVec2(cursorPos.x + windowSize.x - 16.0f, cursorPos.y + windowSize.y - 32.0f);
  ImVec2 drawAreaCenter = ImVec2(cursorPos.x + windowSize.x / 2.0f - 8.0f, cursorPos.y + windowSize.y / 2.0f - 16.0f);

  drawList->AddRect(cursorPos, drawArea, IM_COL32(255, 255, 255, 192));
  drawList->AddRectFilled(cursorPos, drawArea, IM_COL32(64, 64, 64, 128));
  drawList->PushClipRect(cursorPos, drawArea, true);

  mScaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(mOctreeZoomFactor));
  mRotationMat = glm::rotate(mScaleMat, glm::radians(mOctreeRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
  mOctreeViewMat = glm::rotate(mRotationMat, glm::radians(mOctreeRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));

  for (int i = 0; i < mOctreeLines.vertices.size(); i += 2) {
    VkLineVertex startVert = mOctreeLines.vertices.at(i);
    VkLineVertex endVert = mOctreeLines.vertices.at(i+1);

    glm::vec3 startPos = mOctreeViewMat * startVert.position;
    glm::vec3 endPos = mOctreeViewMat * endVert.position;

    ImVec2 pointStart = ImVec2(drawAreaCenter.x + startPos.x + mOctreeTranslation.x,
      drawAreaCenter.y + startPos.z + mOctreeTranslation.y);
    ImVec2 pointEnd = ImVec2(drawAreaCenter.x + endPos.x + mOctreeTranslation.x,
      drawAreaCenter.y + endPos.z + mOctreeTranslation.y);

    drawList->AddLine(pointStart, pointEnd,
      ImColor(startVert.color.r, startVert.color.g, startVert.color.b, 0.6f));
  }

  if (renderData.rdDrawLevelWireframeMiniMap) {
    for (int i = 0; i <  renderData.rdLevelWireframeMiniMapMesh->vertices.size(); i += 2) {
      VkLineVertex startVert = renderData.rdLevelWireframeMiniMapMesh->vertices.at(i);
      VkLineVertex endVert = renderData.rdLevelWireframeMiniMapMesh->vertices.at(i+1);

      glm::vec3 startPos = mOctreeViewMat * startVert.position;
      glm::vec3 endPos = mOctreeViewMat * endVert.position;

      ImVec2 pointStart = ImVec2(drawAreaCenter.x + startPos.x + mOctreeTranslation.x,
        drawAreaCenter.y + startPos.z + mOctreeTranslation.y);
      ImVec2 pointEnd = ImVec2(drawAreaCenter.x + endPos.x + mOctreeTranslation.x,
        drawAreaCenter.y + endPos.z + mOctreeTranslation.y);

      drawList->AddLine(pointStart, pointEnd,
        ImColor(startVert.color.r, startVert.color.g, startVert.color.b, 0.1f));
    }
  }

  drawList->PopClipRect();

  ImGui::End();
}

void UserInterface::resetPositionWindowOctreeView() {
  mOctreeZoomFactor = 0.5f;
  mOctreeRotation = glm::vec3(-65.0f, 55.0f, 0.0f);
  mOctreeTranslation = glm::vec3(0.0f);
}

void UserInterface::createStatusBar(VkRenderData& renderData, ModelInstanceCamData& modInstCamData) {
  ImGuiWindowFlags statusBarFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize;
  /* status bar disabled */
  if (!mStatusBarVisible) {
    return;
  }

  ImGui::SetNextWindowPos(ImVec2(0.0f,  renderData.rdHeight - 35.0f), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(renderData.rdWidth, 35.0f));
  ImGui::SetNextWindowBgAlpha(0.5f);

  if (mCurrentInstance != modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance)) {
    mCurrentInstance = modInstCamData.micAssimpInstances.at(modInstCamData.micSelectedInstance);
  }
  InstanceSettings settings = mCurrentInstance->getInstanceSettings();

  ImGui::Begin("Status", nullptr, statusBarFlags);

  ImGui::AlignTextToFramePadding();
  ImGui::Text("Mode (F10):");
  ImGui::SameLine();
  if (ImGui::Button(renderData.mAppModeMap.at(renderData.rdApplicationMode).c_str())) {
    modInstCamData.micSsetAppModeCallbackFunction(++renderData.rdApplicationMode);
  }

  /* In case more modes are added, use a combo box */
  /*
  static appMode mode = static_cast<appMode>(0);
  ImGui::AlignTextToFramePadding();
  ImGui::Text("Mode (F10):");
  ImGui::SameLine();
  ImGui::PushItemWidth(75.0f);
  if (ImGui::BeginCombo("##AppStateCombo",
    renderData.mAppModeMap.at(renderData.rdApplicationMode).c_str())) {
    for (int i = 0; i < static_cast<int>(appMode::NUM); ++i) {
      const bool isSelected = (static_cast<int>(mode) == i);
      if (ImGui::Selectable(renderData.mAppModeMap[static_cast<appMode>(i)].c_str(), isSelected)) {
        modInstCamData.micSsetAppModeCallbackFunction(static_cast<appMode>(i));
      }

      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::PopItemWidth();
  */

  ImGui::SameLine();
  ImGui::Text(" | Active Camera:  %16s | FPS:  %7.2f | Speed: %2.4f | Accel: %2.4f | State: %6s",
    modInstCamData.micCameras.at(modInstCamData.micSelectedCamera)->getName().c_str(), mFramesPerSecond,
    glm::length(settings.isSpeed), glm::length(settings.isAccel),
    modInstCamData.micMoveStateMap.at(settings.isMoveState).c_str());

  ImGui::End();
}

void UserInterface::render(VkRenderData& renderData) {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), renderData.rdImGuiCommandBuffer);
}

void UserInterface::cleanup(VkRenderData& renderData) {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();

  ImNodes::DestroyContext();
  vkDestroyDescriptorPool(renderData.rdVkbDevice.device, renderData.rdImguiDescriptorPool, nullptr);
  ImGui::DestroyContext();
}

int UserInterface::nameInputFilter(ImGuiInputTextCallbackData* data) {
  ImWchar c = data->EventChar;
  if (std::isdigit(c) || std::isalnum(c) || c == '-' || c == '_') {
    return 0;
  }
  return 1;
}
