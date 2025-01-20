#include <imgui_impl_glfw.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <memory>

#include "OGLRenderer.h"
#include "InstanceSettings.h"
#include "AssimpSettingsContainer.h"
#include "Logger.h"

OGLRenderer::OGLRenderer(GLFWwindow *window) {
  mRenderData.rdWindow = window;
}

bool OGLRenderer::init(unsigned int width, unsigned int height) {
  /* randomize rand() */
  std::srand(static_cast<int>(time(nullptr)));

  /* save orig window title, add current mode */
  mOrigWindowTitle = getWindowTitle();
  setModeInWindowTitle();

  /* required for perspective */
  mRenderData.rdWidth = width;
  mRenderData.rdHeight = height;

  /* initalize GLAD */
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    Logger::log(1, "%s error: failed to initialize GLAD\n", __FUNCTION__);
    return false;
  }

  if (!GLAD_GL_VERSION_4_6) {
    Logger::log(1, "%s error: failed to get at least OpenGL 4.6\n", __FUNCTION__);
    return false;
  }

  GLint majorVersion, minorVersion;
  glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
  glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
  Logger::log(1, "%s: OpenGL %d.%d initializeed\n", __FUNCTION__, majorVersion, minorVersion);

  if (!mFramebuffer.init(width, height)) {
    Logger::log(1, "%s error: could not init Framebuffer\n", __FUNCTION__);
    return false;
  }
  Logger::log(1, "%s: framebuffer succesfully initialized\n", __FUNCTION__);

  mLineVertexBuffer.init();
  Logger::log(1, "%s: line vertex buffer successfully created\n", __FUNCTION__);

  size_t uniformMatrixBufferSize = 3 * sizeof(glm::mat4);
  mUniformBuffer.init(uniformMatrixBufferSize);
  Logger::log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__, uniformMatrixBufferSize);

  if (!mLineShader.loadShaders("shader/line.vert", "shader/line.frag")) {
    Logger::log(1, "%s: line shader loading failed\n", __FUNCTION__);
    return false;
  }

  if (!mAssimpShader.loadShaders("shader/assimp.vert", "shader/assimp.frag")) {
    Logger::log(1, "%s: Assimp shader loading failed\n", __FUNCTION__);
    return false;
  }

  if (!mAssimpSkinningShader.loadShaders("shader/assimp_skinning.vert", "shader/assimp_skinning.frag")) {
    Logger::log(1, "%s: Assimp GPU skinning shader loading failed\n", __FUNCTION__);
    return false;
  }
  if (!mAssimpSkinningShader.getUniformLocation("aModelStride")) {
    Logger::log(1, "%s: could not find symobl 'aModelStride' in GPU skinning shader\n", __FUNCTION__);
    return false;
  }

  if (!mAssimpSelectionShader.loadShaders("shader/assimp_selection.vert", "shader/assimp_selection.frag")) {
    Logger::log(1, "%s: Assimp slection shader loading failed\n", __FUNCTION__);
    return false;
  }

  if (!mAssimpSkinningSelectionShader.loadShaders("shader/assimp_skinning_selection.vert", "shader/assimp_skinning_selection.frag")) {
    Logger::log(1, "%s: Assimp GPU skinning selection shader loading failed\n", __FUNCTION__);
    return false;
  }
  if (!mAssimpSkinningSelectionShader.getUniformLocation("aModelStride")) {
    Logger::log(1, "%s: could not find symobl 'aModelStride' in GPU skinning selection shader\n", __FUNCTION__);
    return false;
  }

  if (!mAssimpTransformComputeShader.loadComputeShader("shader/assimp_instance_transform.comp")) {
    Logger::log(1, "%s: Assimp GPU node transform compute shader loading failed\n", __FUNCTION__);
    return false;
  }
  if (!mAssimpMatrixComputeShader.loadComputeShader("shader/assimp_instance_matrix_mult.comp")) {
    Logger::log(1, "%s: Assimp GPU matrix compute shader loading failed\n", __FUNCTION__);
    return false;
  }

  Logger::log(1, "%s: shaders succesfully loaded\n", __FUNCTION__);

  mUserInterface.init(mRenderData);
  Logger::log(1, "%s: user interface initialized\n", __FUNCTION__);

  /* add backface culling and depth test already here */
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glLineWidth(3.0);

  // register callbacks
  mModelInstData.miModelCheckCallbackFunction = [this](std::string fileName) { return hasModel(fileName); };
  mModelInstData.miModelAddCallbackFunction = [this](std::string fileName) { return addModel(fileName); };
  mModelInstData.miModelDeleteCallbackFunction = [this](std::string modelName) { deleteModel(modelName); };

  mModelInstData.miInstanceAddCallbackFunction = [this](std::shared_ptr<AssimpModel> model) { return addInstance(model); };
  mModelInstData.miInstanceAddManyCallbackFunction = [this](std::shared_ptr<AssimpModel> model, int numInstances) { addInstances(model, numInstances); };
  mModelInstData.miInstanceDeleteCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance) { deleteInstance(instance) ;};
  mModelInstData.miInstanceCloneCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance) { cloneInstance(instance); };
  mModelInstData.miInstanceCloneManyCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance, int numClones) { cloneInstances(instance, numClones); };

  mModelInstData.miInstanceCenterCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance) { centerInstance(instance); };

  mModelInstData.miUndoCallbackFunction = [this]() { undoLastOperation(); };
  mModelInstData.miRedoCallbackFunction = [this]() { redoLastOperation(); };

  mNodeTransformBuffer.init(256);
  mShaderTRSMatrixBuffer.init(256);
  mShaderBoneMatrixBuffer.init(256);

  mShaderModelRootMatrixBuffer.init(64);

  /* valid, but emtpy */
  mLineMesh = std::make_shared<OGLLineMesh>();
  Logger::log(1, "%s: line mesh storage initialized\n", __FUNCTION__);

  /* create an empty null model and an instance from it */
  std::shared_ptr<AssimpModel> nullModel = std::make_shared<AssimpModel>();
  mModelInstData.miModelList.emplace_back(nullModel);
  std::shared_ptr<AssimpInstance> nullInstance = std::make_shared<AssimpInstance>(nullModel);
  mModelInstData.miAssimpInstancesPerModel[nullModel->getModelFileName()].emplace_back(nullInstance);
  mModelInstData.miAssimpInstances.emplace_back(nullInstance);
  enumerateInstances();

  /* init the central settings container */
  mModelInstData.miSettingsContainer = std::make_shared<AssimpSettingsContainer>(nullInstance);

  mFrameTimer.start();

  return true;
}

void OGLRenderer::undoLastOperation() {
  mModelInstData.miSettingsContainer->undo();
  /* we need to update the index numbers in case instances were deleted,
   * and the settings files still contain the old index number */
  enumerateInstances();

  const auto currentUndoInstance = mModelInstData.miSettingsContainer->getCurrentInstance();
  const auto instancePos = std::find_if(mModelInstData.miAssimpInstances.begin(), mModelInstData.miAssimpInstances.end(),
    [currentUndoInstance] (std::shared_ptr<AssimpInstance> instance) { return currentUndoInstance == instance; });
  if (instancePos != mModelInstData.miAssimpInstances.end()) {
    mModelInstData.miSelectedInstance = (*instancePos)->getInstanceSettings().isInstanceIndexPosition;
  } else {
    mModelInstData.miSelectedInstance = 0;
  }
}

void OGLRenderer::redoLastOperation() {
  mModelInstData.miSettingsContainer->redo();
  enumerateInstances();

  const auto currentRedoInstance = mModelInstData.miSettingsContainer->getCurrentInstance();
  const auto instancePos = std::find_if(mModelInstData.miAssimpInstances.begin(), mModelInstData.miAssimpInstances.end(),
    [currentRedoInstance] (std::shared_ptr<AssimpInstance> instance) { return currentRedoInstance == instance; });
  if (instancePos != mModelInstData.miAssimpInstances.end()) {
    mModelInstData.miSelectedInstance = (*instancePos)->getInstanceSettings().isInstanceIndexPosition;
  } else {
    mModelInstData.miSelectedInstance = 0;
  }
}

bool OGLRenderer::hasModel(std::string modelFileName) {
  auto modelIter =  std::find_if(mModelInstData.miModelList.begin(), mModelInstData.miModelList.end(),
    [modelFileName](const auto& model) {
      return model->getModelFileNamePath() == modelFileName || model->getModelFileName() == modelFileName;
    });
  return modelIter != mModelInstData.miModelList.end();
}

bool OGLRenderer::addModel(std::string modelFileName) {
  if (hasModel(modelFileName)) {
    Logger::log(1, "%s warning: model '%s' already existed, skipping\n", __FUNCTION__, modelFileName.c_str());
    return false;
  }

  std::shared_ptr<AssimpModel> model = std::make_shared<AssimpModel>();
  if (!model->loadModel(modelFileName)) {
    Logger::log(1, "%s error: could not load model file '%s'\n", __FUNCTION__, modelFileName.c_str());
    return false;
  }

  mModelInstData.miModelList.emplace_back(model);

  /* also add a new instance here to see the model */
  addInstance(model);

  /* center the first real model instance */
  if (mModelInstData.miAssimpInstances.size() == 2) {
    std::shared_ptr<AssimpInstance> firstInstance = mModelInstData.miAssimpInstances.at(1);
    centerInstance(firstInstance);
  }

  return true;
}

void OGLRenderer::deleteModel(std::string modelFileName) {
  std::string shortModelFileName = std::filesystem::path(modelFileName).filename().generic_string();

  mModelInstData.miAssimpInstances.erase(
    std::remove_if(
      mModelInstData.miAssimpInstances.begin(),
      mModelInstData.miAssimpInstances.end(),
      [shortModelFileName](std::shared_ptr<AssimpInstance> instance) { return instance->getModel()->getModelFileName() == shortModelFileName; }
    ), mModelInstData.miAssimpInstances.end()
  );

  if (mModelInstData.miAssimpInstancesPerModel.count(shortModelFileName) > 0) {
    mModelInstData.miAssimpInstancesPerModel[shortModelFileName].clear();
    mModelInstData.miAssimpInstancesPerModel.erase(shortModelFileName);
  }

  mModelInstData.miModelList.erase(
    std::remove_if(
      mModelInstData.miModelList.begin(),
      mModelInstData.miModelList.end(),
      [modelFileName](std::shared_ptr<AssimpModel> model) { return model->getModelFileName() == modelFileName; }
    )
  );

  enumerateInstances();
  updateTriangleCount();
}

std::shared_ptr<AssimpInstance> OGLRenderer::addInstance(std::shared_ptr<AssimpModel> model) {
  std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(model);
  mModelInstData.miAssimpInstances.emplace_back(newInstance);
  mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].emplace_back(newInstance);

  enumerateInstances();
  updateTriangleCount();

  return newInstance;
}

void OGLRenderer::addInstances(std::shared_ptr<AssimpModel> model, int numInstances) {
  size_t animClipNum = model->getAnimClips().size();
  for (int i = 0; i < numInstances; ++i) {
    int xPos = std::rand() % 50 - 25;
    int zPos = std::rand() % 50 - 25;
    int rotation = std::rand() % 360 - 180;
    int clipNr = std::rand() % animClipNum;
    float animSpeed = (std::rand() % 50 + 75) / 100.0f;

    std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(model, glm::vec3(xPos, 0.0f, zPos), glm::vec3(0.0f, rotation, 0.0f));
    if (animClipNum > 0) {
      InstanceSettings instSettings = newInstance->getInstanceSettings();
      instSettings.isAnimClipNr = clipNr;
      instSettings.isAnimSpeedFactor = animSpeed;
      newInstance->setInstanceSettings(instSettings);
    }

    mModelInstData.miAssimpInstances.emplace_back(newInstance);
    mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].emplace_back(newInstance);
  }

  enumerateInstances();
  updateTriangleCount();
}

void OGLRenderer::deleteInstance(std::shared_ptr<AssimpInstance> instance) {
  std::shared_ptr<AssimpModel> currentModel = instance->getModel();
  std::string currentModelName = currentModel->getModelFileName();

  mModelInstData.miAssimpInstances.erase(
    std::remove_if(
      mModelInstData.miAssimpInstances.begin(),
      mModelInstData.miAssimpInstances.end(),
      [instance](std::shared_ptr<AssimpInstance> inst) { return inst == instance; }));


  mModelInstData.miAssimpInstancesPerModel[currentModelName].erase(
    std::remove_if(
      mModelInstData.miAssimpInstancesPerModel[currentModelName].begin(),
      mModelInstData.miAssimpInstancesPerModel[currentModelName].end(),
      [instance](std::shared_ptr<AssimpInstance> inst) { return inst == instance; }));

  enumerateInstances();
  updateTriangleCount();
}

void OGLRenderer::cloneInstance(std::shared_ptr<AssimpInstance> instance) {
  std::shared_ptr<AssimpModel> currentModel = instance->getModel();
  std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(currentModel);
  InstanceSettings newInstanceSettings = instance->getInstanceSettings();

  /* slight offset to see new instance */
  newInstanceSettings.isWorldPosition += glm::vec3(1.0f, 0.0f, -1.0f);
  newInstance->setInstanceSettings(newInstanceSettings);

  mModelInstData.miAssimpInstances.emplace_back(newInstance);
  mModelInstData.miAssimpInstancesPerModel[currentModel->getModelFileName()].emplace_back(newInstance);

  enumerateInstances();
  updateTriangleCount();
}

/* keep scaling and axis flipping */
void OGLRenderer::cloneInstances(std::shared_ptr<AssimpInstance> instance, int numClones) {
  std::shared_ptr<AssimpModel> model = instance->getModel();
  size_t animClipNum = model->getAnimClips().size();
  for (int i = 0; i < numClones; ++i) {
    int xPos = std::rand() % 50 - 25;
    int zPos = std::rand() % 50 - 25;
    int rotation = std::rand() % 360 - 180;

    int clipNr = std::rand() % animClipNum;
    float animSpeed = (std::rand() % 50 + 75) / 100.0f;

    std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(model);
    InstanceSettings instSettings = instance->getInstanceSettings();
    instSettings.isWorldPosition = glm::vec3(xPos, 0.0f, zPos);
    instSettings.isWorldRotation = glm::vec3(0.0f, rotation, 0.0f);
    if (animClipNum > 0) {
      instSettings.isAnimClipNr = clipNr;
      instSettings.isAnimSpeedFactor = animSpeed;
    }

    newInstance->setInstanceSettings(instSettings);

    mModelInstData.miAssimpInstances.emplace_back(newInstance);
    mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].emplace_back(newInstance);
  }

  enumerateInstances();
  updateTriangleCount();
}

void OGLRenderer::centerInstance(std::shared_ptr<AssimpInstance> instance) {
  InstanceSettings instSettings = instance->getInstanceSettings();
  mCamera.moveCameraTo(mRenderData, instSettings.isWorldPosition + glm::vec3(5.0f));
}

void OGLRenderer::updateTriangleCount() {
  mRenderData.rdTriangleCount = 0;
  for (const auto& instance : mModelInstData.miAssimpInstances) {
    mRenderData.rdTriangleCount += instance->getModel()->getTriangleCount();
  }
}

void OGLRenderer::enumerateInstances() {
  for (size_t i = 0; i < mModelInstData.miAssimpInstances.size(); ++i) {
    InstanceSettings instSettings = mModelInstData.miAssimpInstances.at(i)->getInstanceSettings();
    instSettings.isInstanceIndexPosition = i;
    mModelInstData.miAssimpInstances.at(i)->setInstanceSettings(instSettings);
  }
}

void OGLRenderer::setSize(unsigned int width, unsigned int height) {
  /* handle minimize */
  if (width == 0 || height == 0) {
    return;
  }

  mRenderData.rdWidth = width;
  mRenderData.rdHeight = height;

  mFramebuffer.resize(width, height);
  glViewport(0, 0, width, height);

  Logger::log(1, "%s: resized window to %dx%d\n", __FUNCTION__, width, height);
}

void OGLRenderer::setModeInWindowTitle() {
  if (mRenderData.rdApplicationMode == appMode::edit) {
    setWindowTitle(mOrigWindowTitle + " (Edit Mode)");
  } else {
    setWindowTitle(mOrigWindowTitle + " (View Mode)");
  }
}

void OGLRenderer::handleKeyEvents(int key, int scancode, int action, int mods) {
  /* forward to ImGui only when in edit mode */
  if (mRenderData.rdApplicationMode == appMode::edit) {
    ImGuiIO& io = ImGui::GetIO();

    /* hide from application if above ImGui window */
    if (io.WantCaptureKeyboard || io.WantTextInput) {
      return;
    }
  }

  /* toggle between edit and view mode by pressing F10 */
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_F10) == GLFW_PRESS) {
    mRenderData.rdApplicationMode = mRenderData.rdApplicationMode == appMode::edit ? appMode::view : appMode::edit;
    setModeInWindowTitle();
  }

  /* instance edit modes */
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_1) == GLFW_PRESS) {
     mRenderData.rdInstanceEditMode = instanceEditMode::move;
  }
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_2) == GLFW_PRESS) {
    mRenderData.rdInstanceEditMode = instanceEditMode::rotate;
  }
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_3) == GLFW_PRESS) {
    mRenderData.rdInstanceEditMode = instanceEditMode::scale;
  }

  /* undo/redo only in edit mode */
  if (mRenderData.rdApplicationMode == appMode::edit) {
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_Z) == GLFW_PRESS &&
      (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
      glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) {
      undoLastOperation();
    }

    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_Y) == GLFW_PRESS &&
      (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
      glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) {
      redoLastOperation();
    }
  }

  /* toggle moving instance on Y axis when SHIFT is pressed */
  /* hack to react to both shift keys - remember which one was pressed */
  if (mMouseMove) {
    if  (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      mMouseMoveVerticalShiftKey = GLFW_KEY_LEFT_SHIFT;
      mMouseMoveVertical = true;
    }
    if  (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
      mMouseMoveVerticalShiftKey = GLFW_KEY_RIGHT_SHIFT;
      mMouseMoveVertical = true;
    }
  }
  if  (glfwGetKey(mRenderData.rdWindow, mMouseMoveVerticalShiftKey) == GLFW_RELEASE) {
    mMouseMoveVerticalShiftKey = 0;
    mMouseMoveVertical = false;
  }
}

void OGLRenderer::handleMouseButtonEvents(int button, int action, int mods) {
  /* forward to ImGui only when in edit mode */
  if (mRenderData.rdApplicationMode == appMode::edit) {
    ImGuiIO& io = ImGui::GetIO();
    if (button >= 0 && button < ImGuiMouseButton_COUNT) {
      io.AddMouseButtonEvent(button, action == GLFW_PRESS);
    }

    /* hide from application if above ImGui window */
    if (io.WantCaptureMouse && io.WantCaptureMouseUnlessPopupClose) {
      return;
    }
  }

  /* trigger selection when left button has been released */
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE
      && mRenderData.rdApplicationMode == appMode::edit) {
    mMousePick = true;
    mRenderData.rdInstanceEditMode = instanceEditMode::move;
  }

  /* move instance around with middle button pressed */
  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS
      && mRenderData.rdApplicationMode == appMode::edit) {
    mMouseMove = true;
    if  (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      mMouseMoveVerticalShiftKey = GLFW_KEY_LEFT_SHIFT;
      mMouseMoveVertical = true;
    }
    if  (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
      mMouseMoveVerticalShiftKey = GLFW_KEY_RIGHT_SHIFT;
      mMouseMoveVertical = true;
    }

    if (mModelInstData.miSelectedInstance > 0) {
      mSavedInstanceSettings = mModelInstData.miAssimpInstances.at(mModelInstData.miSelectedInstance)->getInstanceSettings();
    }
  }

  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE
      && mRenderData.rdApplicationMode == appMode::edit) {
    mMouseMove = false;
    if (mModelInstData.miSelectedInstance > 0) {
      std::shared_ptr<AssimpInstance> instance = mModelInstData.miAssimpInstances.at(mModelInstData.miSelectedInstance);
      InstanceSettings settings = instance->getInstanceSettings();
      mModelInstData.miSettingsContainer->apply(instance, settings, mSavedInstanceSettings);
    }
  }

  /* move camera view while right button is hold   */
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
    mMouseLock = true;
  }
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
    mMouseLock = false;
  }

  if (mMouseLock) {
    glfwSetInputMode(mRenderData.rdWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    /* enable raw mode if possible */
    if (glfwRawMouseMotionSupported()) {
      glfwSetInputMode(mRenderData.rdWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
  } else {
    glfwSetInputMode(mRenderData.rdWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }
}

void OGLRenderer::handleMousePositionEvents(double xPos, double yPos) {
  /* forward to ImGui only when in edit mode */
  if (mRenderData.rdApplicationMode == appMode::edit) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent((float)xPos, (float)yPos);

    /* hide from application if above ImGui window */
    if (io.WantCaptureMouse && io.WantCaptureMouseUnlessPopupClose) {
      return;
    }
  }

  /* calculate relative movement from last position */
  int mouseMoveRelX = static_cast<int>(xPos) - mMouseXPos;
  int mouseMoveRelY = static_cast<int>(yPos) - mMouseYPos;

  if (mMouseLock) {
    mRenderData.rdViewAzimuth += mouseMoveRelX / 10.0;
    /* keep between 0 and 360 degree */
    if (mRenderData.rdViewAzimuth < 0.0) {
      mRenderData.rdViewAzimuth += 360.0;
    }
    if (mRenderData.rdViewAzimuth >= 360.0) {
      mRenderData.rdViewAzimuth -= 360.0;
    }

    mRenderData.rdViewElevation -= mouseMoveRelY / 10.0;
    /* keep between -89 and +89 degree */
    mRenderData.rdViewElevation = std::clamp(mRenderData.rdViewElevation, -89.0f, 89.0f);
  }

  if (mMouseMove) {
    if (mModelInstData.miSelectedInstance != 0) {
      InstanceSettings settings = mModelInstData.miAssimpInstances.at(mModelInstData.miSelectedInstance)->getInstanceSettings();

      float mouseXScaled = mouseMoveRelX / 20.0f;
      float mouseYScaled = mouseMoveRelY / 20.0f;
      float sinAzimuth = std::sin(glm::radians(mRenderData.rdViewAzimuth));
      float cosAzimuth = std::cos(glm::radians(mRenderData.rdViewAzimuth));

      float modelDistance = glm::length(mRenderData.rdCameraWorldPosition - settings.isWorldPosition) / 50.0f;

      if (mMouseMoveVertical) {
        switch(mRenderData.rdInstanceEditMode) {
          case instanceEditMode::move:
            settings.isWorldPosition.y -= mouseYScaled * modelDistance;
            break;
          case instanceEditMode::rotate:
            settings.isWorldRotation.y -= mouseXScaled * 5.0f;
            /* keep between -180 and 180 degree */
            if (settings.isWorldRotation.y < -180.0f) {
             settings.isWorldRotation.y += 360.0f;
            }
            if (settings.isWorldRotation.y >= 180.0f) {
              settings.isWorldRotation.y -= 360.0f;
            }
            break;
          case instanceEditMode::scale:
            /* uniform scale, do nothing here  */
            break;
        }
      } else {
        switch(mRenderData.rdInstanceEditMode) {
          case instanceEditMode::move:
            settings.isWorldPosition.x += mouseXScaled * modelDistance * cosAzimuth - mouseYScaled * modelDistance * sinAzimuth;
            settings.isWorldPosition.z += mouseXScaled * modelDistance * sinAzimuth + mouseYScaled * modelDistance * cosAzimuth;
            break;
          case instanceEditMode::rotate:
            settings.isWorldRotation.z -= (mouseXScaled * cosAzimuth - mouseYScaled * sinAzimuth) * 5.0f;
            settings.isWorldRotation.x += (mouseXScaled * sinAzimuth + mouseYScaled * cosAzimuth) * 5.0f;

            /* keep between -180 and 180 degree */
            if (settings.isWorldRotation.z < -180.0f) {
             settings.isWorldRotation.z += 360.0f;
            }
            if (settings.isWorldRotation.z >= 180.0f) {
              settings.isWorldRotation.z -= 360.0f;
            }

            if (settings.isWorldRotation.x < -180.0f) {
             settings.isWorldRotation.x += 360.0f;
            }
            if (settings.isWorldRotation.x >= 180.0f) {
              settings.isWorldRotation.x -= 360.0f;
            }
            break;
          case instanceEditMode::scale:
            settings.isScale -= mouseYScaled / 2.0f;
            settings.isScale = std::max(0.001f, settings.isScale);
            break;
        }
      }

      mModelInstData.miAssimpInstances.at(mModelInstData.miSelectedInstance)->setInstanceSettings(settings);
    }
  }

  /* save old values */
  mMouseXPos = static_cast<int>(xPos);
  mMouseYPos = static_cast<int>(yPos);
}

void OGLRenderer::handleMovementKeys() {
  mRenderData.rdMoveForward = 0;
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_W) == GLFW_PRESS) {
    mRenderData.rdMoveForward += 1;
  }
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_S) == GLFW_PRESS) {
    mRenderData.rdMoveForward -= 1;
  }

  mRenderData.rdMoveRight = 0;
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_A) == GLFW_PRESS) {
    mRenderData.rdMoveRight -= 1;
  }
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_D) == GLFW_PRESS) {
    mRenderData.rdMoveRight += 1;
  }

  mRenderData.rdMoveUp = 0;
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_E) == GLFW_PRESS) {
    mRenderData.rdMoveUp += 1;
  }
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_Q) == GLFW_PRESS) {
    mRenderData.rdMoveUp -= 1;
  }

  /* speed up movement with shift */
  if ((glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
      (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)) {
    mRenderData.rdMoveForward *= 10;
    mRenderData.rdMoveRight *= 10;
    mRenderData.rdMoveUp *= 10;
  }
}

bool OGLRenderer::draw(float deltaTime) {
  /* no update on zero diff */
  if (deltaTime == 0.0f) {
    return true;
  }

  /* handle minimize */
  while (mRenderData.rdWidth == 0 || mRenderData.rdHeight == 0) {
    glfwGetFramebufferSize(mRenderData.rdWindow, &mRenderData.rdWidth, &mRenderData.rdHeight);
    glfwWaitEvents();
  }

  mRenderData.rdFrameTime = mFrameTimer.stop();
  mFrameTimer.start();

  /* reset timers and other values */
  mRenderData.rdMatricesSize = 0;
  mRenderData.rdUploadToUBOTime = 0.0f;
  mRenderData.rdUploadToVBOTime = 0.0f;
  mRenderData.rdMatrixGenerateTime = 0.0f;
  mRenderData.rdUIGenerateTime = 0.0f;
  mRenderData.rdUIDrawTime = 0.0f;

  handleMovementKeys();

  /* draw to framebuffer */
  mFramebuffer.bind();
  mFramebuffer.clearTextures();

  mMatrixGenerateTimer.start();
  mCamera.updateCamera(mRenderData, deltaTime);

  mProjectionMatrix = glm::perspective(
    glm::radians(static_cast<float>(mRenderData.rdFieldOfView)),
    static_cast<float>(mRenderData.rdWidth) / static_cast<float>(mRenderData.rdHeight),
    0.1f, 500.0f);

  mViewMatrix = mCamera.getViewMatrix(mRenderData);

  mRenderData.rdMatrixGenerateTime += mMatrixGenerateTimer.stop();

  mUploadToUBOTimer.start();
  std::vector<glm::mat4> matrixData;
  matrixData.emplace_back(mViewMatrix);
  matrixData.emplace_back(mProjectionMatrix);
  mUniformBuffer.uploadUboData(matrixData, 0);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  /* save the selected instance for color highlight */
  std::shared_ptr<AssimpInstance> currentSelectedInstance = nullptr;
  if (mRenderData.rdApplicationMode == appMode::edit) {
    if (mRenderData.rdHighlightSelectedInstance) {
      currentSelectedInstance = mModelInstData.miAssimpInstances.at(mModelInstData.miSelectedInstance);
      mRenderData.rdSelectedInstanceHighlightValue += deltaTime * 4.0f;
      if (mRenderData.rdSelectedInstanceHighlightValue > 2.0f) {
        mRenderData.rdSelectedInstanceHighlightValue = 0.1f;
      }
    }
  }

  mRenderData.rdMatricesSize = 0;
  for (const auto& model : mModelInstData.miModelList) {
    size_t numberOfInstances = mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].size();
    if (numberOfInstances > 0 && model->getTriangleCount() > 0) {

      /* animated models */
      if (model->hasAnimations() &&
        model->getBoneList().size() > 0) {

        size_t numberOfBones = model->getBoneList().size();

        mMatrixGenerateTimer.start();

        mNodeTransFormData.resize(numberOfInstances * numberOfBones);
        mWorldPosMatrices.resize(numberOfInstances);
        mSelectedInstance.resize(numberOfInstances);

        std::vector<std::shared_ptr<AssimpInstance>> instances = mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()];
        for (size_t i = 0; i < numberOfInstances; ++i) {
          instances.at(i)->updateAnimation(deltaTime);
          std::vector<NodeTransformData> instanceNodeTransform = instances.at(i)->getNodeTransformData();
          std::copy(instanceNodeTransform.begin(), instanceNodeTransform.end(), mNodeTransFormData.begin() + i * numberOfBones);
          mWorldPosMatrices.at(i) = instances.at(i)->getWorldTransformMatrix();

          if (mRenderData.rdApplicationMode == appMode::edit) {
            if (currentSelectedInstance == instances.at(i)) {
              mSelectedInstance.at(i).x = mRenderData.rdSelectedInstanceHighlightValue;
            } else {
              mSelectedInstance.at(i).x = 1.0f;
            }

            if (mMousePick) {
              InstanceSettings instSettings = instances.at(i)->getInstanceSettings();
              mSelectedInstance.at(i).y = static_cast<float>(instSettings.isInstanceIndexPosition);
            }
          } else {
          mSelectedInstance.at(i).x = 1.0f;
          }
        }

        size_t trsMatrixSize = numberOfBones * numberOfInstances * sizeof(glm::mat4);
        mRenderData.rdMatricesSize += trsMatrixSize;

        /* we may have to resize the buffers (uploadSsboData() checks for the size automatically, bind() not) */
        mShaderBoneMatrixBuffer.checkForResize(trsMatrixSize);
        mShaderTRSMatrixBuffer.checkForResize(trsMatrixSize);

        mRenderData.rdMatrixGenerateTime += mMatrixGenerateTimer.stop();

        /* calculate TRS matrices from node transforms */
        mAssimpTransformComputeShader.use();

        mUploadToUBOTimer.start();
        mNodeTransformBuffer.uploadSsboData(mNodeTransFormData, 0);
        mShaderTRSMatrixBuffer.bind(1);
        mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

        /* do the computation - in groups of 32 invocations */
        glDispatchCompute(numberOfBones, std::ceil(numberOfInstances / 32.0f), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /* multiply every bone TRS matrix with its parent bones TRS matrices, until the root bone has been reached
         * also, multiply the bone TRS and the bone offset matrix */
        mAssimpMatrixComputeShader.use();

        mUploadToUBOTimer.start();
        mShaderTRSMatrixBuffer.bind(0);
        model->bindBoneParentBuffer(1);
        model->bindBoneMatrixOffsetBuffer(2);
        mShaderBoneMatrixBuffer.bind(3);
        mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

        /* do the computation - in groups of 32 invocations */
        glDispatchCompute(numberOfBones, std::ceil(numberOfInstances / 32.0f), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /* now bind the final bone transforms to the vertex skinning shader */
        if (mMousePick && mRenderData.rdApplicationMode == appMode::edit) {
          mAssimpSkinningSelectionShader.use();
        } else {
          mAssimpSkinningShader.use();
        }

        mUploadToUBOTimer.start();
        mAssimpSkinningShader.setUniformValue(numberOfBones);
        mShaderBoneMatrixBuffer.bind(1);
        mShaderModelRootMatrixBuffer.uploadSsboData(mWorldPosMatrices, 2);
        mSelectedInstanceBuffer.uploadSsboData(mSelectedInstance, 3);
        mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();
      } else {
        /* non-animated models */

        mMatrixGenerateTimer.start();
        mWorldPosMatrices.resize(numberOfInstances);
        mSelectedInstance.resize(numberOfInstances);

        std::vector<std::shared_ptr<AssimpInstance>> instances = mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()];

        for (size_t i = 0; i < numberOfInstances; ++i) {
          mWorldPosMatrices.at(i) = instances.at(i)->getWorldTransformMatrix();
          if (mRenderData.rdApplicationMode == appMode::edit) {

            if (currentSelectedInstance == instances.at(i)) {
              mSelectedInstance.at(i).x = mRenderData.rdSelectedInstanceHighlightValue;
            } else {
              mSelectedInstance.at(i).x = 1.0f;
            }

            if (mMousePick) {
              InstanceSettings instSettings = instances.at(i)->getInstanceSettings();
              mSelectedInstance.at(i).y = static_cast<float>(instSettings.isInstanceIndexPosition);
            }
          } else {
            mSelectedInstance.at(i).x = 1.0f;
          }
        }

        mRenderData.rdMatrixGenerateTime += mMatrixGenerateTimer.stop();
        mRenderData.rdMatricesSize += mWorldPosMatrices.size() * sizeof(glm::mat4);

        if (mMousePick && mRenderData.rdApplicationMode == appMode::edit) {
          mAssimpSelectionShader.use();
        } else {
          mAssimpShader.use();
        }

        mUploadToUBOTimer.start();
        mShaderModelRootMatrixBuffer.uploadSsboData(mWorldPosMatrices, 1);
        mSelectedInstanceBuffer.uploadSsboData(mSelectedInstance, 2);
        mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();
      }

      model->drawInstanced(numberOfInstances);
    }
  }

  /* draw coord arrow, depending on edit mode */
  mCoordArrowsLineIndexCount = 0;
  mLineMesh->vertices.clear();
  if (mRenderData.rdApplicationMode == appMode::edit) {
    if (mModelInstData.miSelectedInstance > 0) {
      InstanceSettings instSettings = mModelInstData.miAssimpInstances.at(mModelInstData.miSelectedInstance)->getInstanceSettings();

      /* draw coordiante arrows at origin of selected instance */
      switch(mRenderData.rdInstanceEditMode) {
        case instanceEditMode::move:
          mCoordArrowsMesh = mCoordArrowsModel.getVertexData();
          break;
        case instanceEditMode::rotate:
          mCoordArrowsMesh = mRotationArrowsModel.getVertexData();
          break;
        case instanceEditMode::scale:
          mCoordArrowsMesh = mScaleArrowsModel.getVertexData();
          break;
      }

      mCoordArrowsLineIndexCount += mCoordArrowsMesh.vertices.size();
      std::for_each(mCoordArrowsMesh.vertices.begin(), mCoordArrowsMesh.vertices.end(),
        [=](auto &n) {
          n.color /= 2.0f;
          n.position = glm::quat(glm::radians(instSettings.isWorldRotation)) * n.position;
          n.position += instSettings.isWorldPosition;
      });
      mLineMesh->vertices.insert(mLineMesh->vertices.end(),
                                 mCoordArrowsMesh.vertices.begin(), mCoordArrowsMesh.vertices.end());
    }

    mUploadToVBOTimer.start();
    mLineVertexBuffer.uploadData(*mLineMesh);
    mRenderData.rdUploadToVBOTime += mUploadToVBOTimer.stop();

    /* draw the coordinate arrow WITH depth buffer */
    if (mCoordArrowsLineIndexCount > 0) {
      mLineShader.use();
      mLineVertexBuffer.bindAndDraw(GL_LINES, 0, mCoordArrowsLineIndexCount);
    }

    if (mMousePick) {
      /* wait until selection buffer has been filled */
      glFlush();
      glFinish();

      /* inverted Y */
      float selectedInstanceId = mFramebuffer.readPixelFromPos(mMouseXPos, (mRenderData.rdHeight - mMouseYPos - 1));

      if (selectedInstanceId >= 0.0f) {
        mModelInstData.miSelectedInstance = static_cast<int>(selectedInstanceId);
      } else {
        mModelInstData.miSelectedInstance = 0;
      }
      mMousePick = false;
    }
  }

  mFramebuffer.unbind();

  /* blit color buffer to screen */
  /* XXX: enable sRGB ONLY for the final framebuffer draw */
  glEnable(GL_FRAMEBUFFER_SRGB);
  mFramebuffer.drawToScreen();
  glDisable(GL_FRAMEBUFFER_SRGB);

  if (mRenderData.rdApplicationMode == appMode::edit) {
    mUIGenerateTimer.start();
    mUserInterface.hideMouse(mMouseLock);
    mUserInterface.createFrame(mRenderData, mModelInstData);
    mRenderData.rdUIGenerateTime += mUIGenerateTimer.stop();
  }

  if (mRenderData.rdApplicationMode == appMode::edit) {
    mUIDrawTimer.start();
    mUserInterface.render();
    mRenderData.rdUIDrawTime += mUIDrawTimer.stop();
  }

  return true;
}

void OGLRenderer::cleanup() {
  mSelectedInstanceBuffer.cleanup();

  mShaderModelRootMatrixBuffer.cleanup();

  mShaderBoneMatrixBuffer.cleanup();
  mShaderTRSMatrixBuffer.cleanup();

  mNodeTransformBuffer.cleanup();

  mAssimpTransformComputeShader.cleanup();
  mAssimpMatrixComputeShader.cleanup();

  mAssimpSkinningSelectionShader.cleanup();
  mAssimpSelectionShader.cleanup();
  mAssimpSkinningShader.cleanup();
  mAssimpShader.cleanup();
  mLineShader.cleanup();

  mUserInterface.cleanup();

  mLineVertexBuffer.cleanup();
  mUniformBuffer.cleanup();

  mFramebuffer.cleanup();
}
