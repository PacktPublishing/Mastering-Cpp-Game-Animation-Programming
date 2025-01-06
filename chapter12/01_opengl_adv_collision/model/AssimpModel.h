/* Assimp model, ready to draw */
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <glad/glad.h>

#include <glm/glm.hpp>

#include "Texture.h"
#include "AssimpMesh.h"
#include "AssimpNode.h"
#include "AssimpAnimClip.h"
#include "VertexIndexBuffer.h"
#include "ShaderStorageBuffer.h"
#include "ModelSettings.h"
#include "InstanceSettings.h"

#include "OGLRenderData.h"

class AssimpModel {
  public:
    bool loadModel(std::string modelFilename, unsigned int extraImportFlags = 0);
    glm::mat4 getRootTranformationMatrix();

    void draw();
    void drawInstanced(int instanceCount);
    void drawInstancedNoMorphAnims(int instanceCount);
    void drawInstancedMorphAnims(int instanceCount);
    unsigned int getTriangleCount();

    std::string getModelFileName();
    std::string getModelFileNamePath();

    bool hasAnimations();
    const std::vector<std::shared_ptr<AssimpAnimClip>>& getAnimClips();
    float getMaxClipDuration();

    const std::vector<std::shared_ptr<AssimpNode>>& getNodeList();
    const std::map<std::string, std::shared_ptr<AssimpNode>>& getNodeMap();

    const std::vector<std::shared_ptr<AssimpBone>>& getBoneList();
    const std::vector<std::string> getBoneNameList();

    void bindBoneMatrixOffsetBuffer(int bindingPoint);
    void bindInverseBoneMatrixOffsetBuffer(int bindingPoint);
    void bindBoneParentBuffer(int bindingPoint);
    void bindAnimLookupBuffer(int bindingPoint);

    std::vector<int32_t> getBoneParentIndexList();

    void setModelSettings(ModelSettings settings);
    ModelSettings getModelSettings();

    void setAABBLokkup(std::vector<std::vector<AABB>> lookupData);
    AABB getAABB(InstanceSettings instSettings);
    AABB getAnimatedAABB(InstanceSettings instSettings);
    AABB getNonAnimatedAABB(InstanceSettings instSettings);

    bool hasAnimMeshes();
    unsigned int getAnimMeshVertexSize();
    void bindMorphAnimBuffer(int bindingPoint);

    bool hasHeadMovementAnimationsMapped();

    glm::mat4 getBoneOffsetMatrix(int boneId);
    glm::mat4 getInverseBoneOffsetMatrix(int boneId);

    void setIkNodeChain(int footId, int effektorNode, int targetNode);

    void cleanup();

private:
    void processNode(std::shared_ptr<AssimpNode> node, aiNode* aNode, const aiScene* scene, std::string assetDirectory);
    void createNodeList(std::shared_ptr<AssimpNode> node, std::shared_ptr<AssimpNode> newNode, std::vector<std::shared_ptr<AssimpNode>> &list);
    void drawInstanced(OGLMesh& mesh, unsigned int meshIndex, int instanceCount);

    unsigned int mTriangleCount = 0;
    unsigned int mVertexCount = 0;

    float mMaxClipDuration = 0.0f;

    /* store the root node for direct access */
    std::shared_ptr<AssimpNode> mRootNode = nullptr;
    /* a map to find the node by name */
    std::map<std::string, std::shared_ptr<AssimpNode>> mNodeMap{};
    /* and a 'flat' map to keep the order of insertation  */
    std::vector<std::shared_ptr<AssimpNode>> mNodeList{};

    std::vector<std::shared_ptr<AssimpBone>> mBoneList{};
    std::vector<std::string> mBoneNameList{};
    std::map<std::string, glm::mat4> mBoneOffsetMatrices{};

    std::vector<glm::mat4> mBoneOffsetMatricesList{};
    std::vector<glm::mat4> mInverseBoneOffsetMatricesList{};

    std::vector<std::shared_ptr<AssimpAnimClip>> mAnimClips{};

    std::vector<OGLMesh> mModelMeshes{};
    std::vector<VertexIndexBuffer> mVertexBuffers{};

    ShaderStorageBuffer mShaderBoneParentBuffer{};
    std::vector<int32_t> mBoneParentIndexList{};
    ShaderStorageBuffer mShaderBoneMatrixOffsetBuffer{};
    ShaderStorageBuffer mShaderInverseBoneMatrixOffsetBuffer{};
    ShaderStorageBuffer mAnimLookupBuffer{};

    // map textures to external or internal texture names
    std::unordered_map<std::string, std::shared_ptr<Texture>> mTextures{};
    std::shared_ptr<Texture> mPlaceholderTexture = nullptr;

    glm::mat4 mRootTransformMatrix = glm::mat4(1.0f);

    ModelSettings mModelSettings{};
    std::vector<std::vector<AABB>> mAabbLookups{};

    unsigned int mNumAnimatedMeshes = 0;
    unsigned int mAnimatedMeshVertexSize = 0;
    ShaderStorageBuffer mAnimMeshVerticesBuffer{};
};
