#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <memory>

#include <imgui_impl_glfw.h>

#include <glm/gtc/matrix_transform.hpp>

#include "OGLRenderer.h"
#include "InstanceSettings.h"
#include "Logger.h"

OGLRenderer::OGLRenderer(GLFWwindow *window) {
  mRenderData.rdWindow = window;
}

bool OGLRenderer::init(unsigned int width, unsigned int height) {
  /* randomize rand() */
  std::srand(static_cast<int>(time(nullptr)));

  /* required for perspective */
  mRenderData.rdWidth = width;
  mRenderData.rdHeight = height;

  /* initialize GLAD */
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
  Logger::log(1, "%s: OpenGL %d.%d initialized\n", __FUNCTION__, majorVersion, minorVersion);

  if (!mFramebuffer.init(width, height)) {
    Logger::log(1, "%s error: could not init Framebuffer\n", __FUNCTION__);
    return false;
  }
  Logger::log(1, "%s: framebuffer successfully initialized\n", __FUNCTION__);

  size_t uniformMatrixBufferSize = 3 * sizeof(glm::mat4);
  mUniformBuffer.init(uniformMatrixBufferSize);
  Logger::log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__, uniformMatrixBufferSize);

  if (!mAssimpShader.loadShaders("shader/assimp.vert", "shader/assimp.frag")) {
    Logger::log(1, "%s: Assimp shader loading failed\n", __FUNCTION__);
    return false;
  }

  if (!mAssimpSkinningShader.loadShaders("shader/assimp_skinning.vert", "shader/assimp_skinning.frag")) {
    Logger::log(1, "%s: Assimp GPU skinning shader loading failed\n", __FUNCTION__);
    return false;
  }
  if (!mAssimpSkinningShader.getUniformLocation("aModelStride")) {
    Logger::log(1, "%s: could not find symbol 'aModelStride' in GPU skinning shader\n", __FUNCTION__);
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

  Logger::log(1, "%s: shaders successfully loaded\n", __FUNCTION__);

  mUserInterface.init(mRenderData);
  Logger::log(1, "%s: user interface initialized\n", __FUNCTION__);

  /* add backface culling and depth test already here */
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glLineWidth(3.0);
  Logger::log(1, "%s: rendering defaults set\n", __FUNCTION__);

  /* SSBO init */
  mShaderBoneMatrixBuffer.init(256);
  mShaderModelRootMatrixBuffer.init(256);
  mShaderTRSMatrixBuffer.init(256);
  mNodeTransformBuffer.init(256);
  Logger::log(1, "%s: SSBOs initialized\n", __FUNCTION__);

  /* register callbacks */
  mModelInstData.miModelCheckCallbackFunction = [this](std::string fileName) { return hasModel(fileName); };
  mModelInstData.miModelAddCallbackFunction = [this](std::string fileName) { return addModel(fileName); };
  mModelInstData.miModelDeleteCallbackFunction = [this](std::string modelName) { deleteModel(modelName); };

  mModelInstData.miInstanceAddCallbackFunction = [this](std::shared_ptr<AssimpModel> model) { return addInstance(model); };
  mModelInstData.miInstanceAddManyCallbackFunction = [this](std::shared_ptr<AssimpModel> model, int numInstances) { addInstances(model, numInstances); };
  mModelInstData.miInstanceDeleteCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance) { deleteInstance(instance) ;};
  mModelInstData.miInstanceCloneCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance) { cloneInstance(instance); };

  mFrameTimer.start();

  return true;
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

  return true;
}

void OGLRenderer::deleteModel(std::string modelFileName) {
  std::string shortModelFileName = std::filesystem::path(modelFileName).filename().generic_string();

  if (!mModelInstData.miAssimpInstances.empty()) {
    mModelInstData.miAssimpInstances.erase(
      std::remove_if(
        mModelInstData.miAssimpInstances.begin(),
        mModelInstData.miAssimpInstances.end(),
        [shortModelFileName](std::shared_ptr<AssimpInstance> instance) { return instance->getModel()->getModelFileName() == shortModelFileName; }
      ), mModelInstData.miAssimpInstances.end()
    );
  }

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

  updateTriangleCount();
}

std::shared_ptr<AssimpInstance> OGLRenderer::addInstance(std::shared_ptr<AssimpModel> model) {
  std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(model);
  mModelInstData.miAssimpInstances.emplace_back(newInstance);
  mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].emplace_back(newInstance);

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

    std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(model, glm::vec3(xPos, 0.0f, zPos), glm::vec3(0.0f, rotation, 0.0f));
    if (animClipNum > 0) {
      InstanceSettings instSettings = newInstance->getInstanceSettings();
      instSettings.isAnimClipNr = clipNr;
      newInstance->setInstanceSettings(instSettings);
    }

    mModelInstData.miAssimpInstances.emplace_back(newInstance);
    mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].emplace_back(newInstance);
  }
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

  updateTriangleCount();
}

void OGLRenderer::updateTriangleCount() {
  mRenderData.rdTriangleCount = 0;
  for (const auto& instance : mModelInstData.miAssimpInstances) {
    mRenderData.rdTriangleCount += instance->getModel()->getTriangleCount();
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

void OGLRenderer::handleKeyEvents(int key, int scancode, int action, int mods) {
}

void OGLRenderer::handleMouseButtonEvents(int button, int action, int mods) {
  /* forward to ImGui */
  ImGuiIO& io = ImGui::GetIO();
  if (button >= 0 && button < ImGuiMouseButton_COUNT) {
    io.AddMouseButtonEvent(button, action == GLFW_PRESS);
  }

  /* hide from application if above ImGui window */
  if (io.WantCaptureMouse && io.WantCaptureMouseUnlessPopupClose) {
    return;
  }

  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
    mMouseLock = !mMouseLock;

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
}

void OGLRenderer::handleMousePositionEvents(double xPos, double yPos) {
  /* forward to ImGui */
  ImGuiIO& io = ImGui::GetIO();
  io.AddMousePosEvent((float)xPos, (float)yPos);

  /* hide from application if above ImGui window */
  if (io.WantCaptureMouse && io.WantCaptureMouseUnlessPopupClose) {
    return;
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

  handleMovementKeys();

  /* draw to framebuffer */
  mFramebuffer.bind();

  glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
  glClearDepth(1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_FRAMEBUFFER_SRGB);

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

  /* draw the models */
  for (const auto& modelType : mModelInstData.miAssimpInstancesPerModel) {
    size_t numberOfInstances = modelType.second.size();
    if (numberOfInstances > 0) {
      std::shared_ptr<AssimpModel> model = modelType.second.at(0)->getModel();

      /* animated models */
      if (model->hasAnimations() && !model->getBoneList().empty()) {
        size_t numberOfBones = model->getBoneList().size();

        mMatrixGenerateTimer.start();

        mNodeTransFormData.resize(numberOfInstances * numberOfBones);
        mWorldPosMatrices.resize(numberOfInstances);

        for (unsigned int i = 0; i < numberOfInstances; ++i) {
          modelType.second.at(i)->updateAnimation(deltaTime);
          std::vector<NodeTransformData> instanceNodeTransform = modelType.second.at(i)->getNodeTransformData();
          std::copy(instanceNodeTransform.begin(), instanceNodeTransform.end(), mNodeTransFormData.begin() + i * numberOfBones);
          mWorldPosMatrices.at(i) = modelType.second.at(i)->getWorldTransformMatrix();
        }
        mRenderData.rdMatrixGenerateTime += mMatrixGenerateTimer.stop();

        size_t trsMatrixSize = numberOfBones * numberOfInstances * sizeof(glm::mat4);
        mRenderData.rdMatricesSize += trsMatrixSize;

        /* we may have to resize the buffers (uploadSsboData() checks for the size automatically, bind() not) */
        mShaderBoneMatrixBuffer.checkForResize(trsMatrixSize);
        mShaderTRSMatrixBuffer.checkForResize(trsMatrixSize);

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
        mAssimpSkinningShader.use();

        mUploadToUBOTimer.start();
        mAssimpSkinningShader.setUniformValue(numberOfBones);
        mShaderBoneMatrixBuffer.bind(1);
        mShaderModelRootMatrixBuffer.uploadSsboData(mWorldPosMatrices, 2);
        mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();
      } else {
        /* non-animated models */
        mMatrixGenerateTimer.start();
        mWorldPosMatrices.clear();

        for (const auto& instance : modelType.second) {
          mWorldPosMatrices.emplace_back(instance->getWorldTransformMatrix());
        }
        mRenderData.rdMatrixGenerateTime += mMatrixGenerateTimer.stop();
        mRenderData.rdMatricesSize += mWorldPosMatrices.size() * sizeof(glm::mat4);

        mAssimpShader.use();
        mUploadToUBOTimer.start();
        mShaderModelRootMatrixBuffer.uploadSsboData(mWorldPosMatrices, 1);
        mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();
      }

      model->drawInstanced(numberOfInstances);
    }
  }

  mFramebuffer.unbind();

  /* blit color buffer to screen */
  /* XXX: enable sRGB ONLY for the final framebuffer draw */
  glEnable(GL_FRAMEBUFFER_SRGB);
  mFramebuffer.drawToScreen();
  glDisable(GL_FRAMEBUFFER_SRGB);

  mUIGenerateTimer.start();
  mUserInterface.hideMouse(mMouseLock);
  mUserInterface.createFrame(mRenderData, mModelInstData);
  mRenderData.rdUIGenerateTime += mUIGenerateTimer.stop();

  mUIDrawTimer.start();
  mUserInterface.render();
  mRenderData.rdUIDrawTime = mUIDrawTimer.stop();

  return true;
}

void OGLRenderer::cleanup() {
  mShaderModelRootMatrixBuffer.cleanup();

  mShaderBoneMatrixBuffer.cleanup();
  mShaderTRSMatrixBuffer.cleanup();

  mNodeTransformBuffer.cleanup();

  mAssimpTransformComputeShader.cleanup();
  mAssimpMatrixComputeShader.cleanup();

  mAssimpSkinningShader.cleanup();
  mAssimpShader.cleanup();

  mUserInterface.cleanup();

  mUniformBuffer.cleanup();

  mFramebuffer.cleanup();
}
