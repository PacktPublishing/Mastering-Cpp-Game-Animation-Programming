#pragma once
#include <vector>
#include <string>
#include <memory>
#include <map>

#include <glm/glm.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Timer.h"
#include "Framebuffer.h"
#include "LineVertexBuffer.h"
#include "Texture.h"
#include "Shader.h"
#include "UniformBuffer.h"
#include "ShaderStorageBuffer.h"
#include "UserInterface.h"
#include "Camera.h"
#include "CoordArrowsModel.h"
#include "RotationArrowsModel.h"
#include "ScaleArrowsModel.h"
#include "AssimpModel.h"
#include "AssimpInstance.h"

#include "OGLRenderData.h"
#include "ModelAndInstanceData.h"

class OGLRenderer {
  public:
    OGLRenderer(GLFWwindow *window);

    bool init(unsigned int width, unsigned int height);
    void setSize(unsigned int width, unsigned int height);

    void uploadAssimpData(OGLMesh vertexData);
    bool draw(float deltaTime);

    void handleKeyEvents(int key, int scancode, int action, int mods);
    void handleMouseButtonEvents(int button, int action, int mods);
    void handleMousePositionEvents(double xPos, double yPos);

    bool hasModel(std::string modelFileName);
    bool addModel(std::string modelFileName);
    void deleteModel(std::string modelFileName);

    std::shared_ptr<AssimpInstance> addInstance(std::shared_ptr<AssimpModel> model);
    void addInstances(std::shared_ptr<AssimpModel> model, int numInstances);
    void deleteInstance(std::shared_ptr<AssimpInstance> instance);
    void cloneInstance(std::shared_ptr<AssimpInstance> instance);
    void cloneInstances(std::shared_ptr<AssimpInstance> instance, int numClones);

    void centerInstance(std::shared_ptr<AssimpInstance> instance);

    void cleanup();

  private:
    OGLRenderData mRenderData{};
    ModelAndInstanceData mModelInstData{};

    Timer mFrameTimer{};
    Timer mMatrixGenerateTimer{};
    Timer mIKTimer{};
    Timer mUploadToVBOTimer{};
    Timer mUploadToUBOTimer{};
    Timer mUIGenerateTimer{};
    Timer mUIDrawTimer{};

    Shader mLineShader{};
    Shader mAssimpShader{};
    Shader mAssimpSkinningShader{};

    Shader mAssimpSelectionShader{};
    Shader mAssimpSkinningSelectionShader{};

    Shader mAssimpTransformComputeShader{};
    Shader mAssimpMatrixComputeShader{};

    Framebuffer mFramebuffer{};
    LineVertexBuffer mLineVertexBuffer{};
    UniformBuffer mUniformBuffer{};
    UserInterface mUserInterface{};
    Camera mCamera{};

    /* for animated and non-animated models */
    ShaderStorageBuffer mShaderModelRootMatrixBuffer{};
    std::vector<glm::mat4> mWorldPosMatrices{};

    /* color hightlight for selection etc */
    std::vector<glm::vec2> mSelectedInstance{};
    ShaderStorageBuffer mSelectedInstanceBuffer{};

    /* for animated models */
    ShaderStorageBuffer mShaderBoneMatrixBuffer{};

    /* for compute shader */
    ShaderStorageBuffer mShaderTRSMatrixBuffer{};
    ShaderStorageBuffer mNodeTransformBuffer{};
    std::vector<NodeTransformData> mNodeTransFormData{};

    CoordArrowsModel mCoordArrowsModel{};
    RotationArrowsModel mRotationArrowsModel{};
    ScaleArrowsModel mScaleArrowsModel{};
    OGLLineMesh mCoordArrowsMesh{};
    std::shared_ptr<OGLLineMesh> mLineMesh = nullptr;

    unsigned int mCoordArrowsLineIndexCount = 0;

    bool mMouseLock = false;
    int mMouseXPos = 0;
    int mMouseYPos = 0;
    bool mMousePick = false;

    bool mMouseMove = false;
    bool mMouseMoveVertical = false;
    int mMouseMoveVerticalShiftKey = 0;

    void handleMovementKeys();
    void updateTriangleCount();
    void assignInstanceIndices();

    /* create identity matrix by default */
    glm::mat4 mViewMatrix = glm::mat4(1.0f);
    glm::mat4 mProjectionMatrix = glm::mat4(1.0f);

};
