/* Assimp model, ready to draw */
#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>

#include <glm/glm.hpp>

#include "Texture.h"
#include "AssimpMesh.h"
#include "AssimpNode.h"
#include "AssimpAnimClip.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ShaderStorageBuffer.h"
#include "ModelSettings.h"
#include "InstanceSettings.h"
#include "AABB.h"

#include "VkRenderData.h"

class AssimpModel {
  public:
    bool loadModel(VkRenderData &renderData, std::string modelFilename, unsigned int extraImportFlags = 0);
    glm::mat4 getRootTranformationMatrix();

    void draw(VkRenderData &renderData, bool selectionModeActive);
    void drawInstanced(VkRenderData &renderData, uint32_t instanceCount, bool selectionModeActive);
    void drawInstancedNoMorphAnims(VkRenderData &renderData, uint32_t instanceCount, bool selectionModeActive);
    void drawInstancedMorphAnims(VkRenderData &renderData, uint32_t instanceCount, bool selectionModeActive);
    unsigned int getTriangleCount();

    std::string getModelFileName();
    std::string getModelFileNamePath();

    bool hasAnimations();
    const std::vector<std::shared_ptr<AssimpAnimClip>>& getAnimClips();
    float getMaxClipDuration();

    const std::vector<std::shared_ptr<AssimpNode>>& getNodeList();
    const std::unordered_map<std::string, std::shared_ptr<AssimpNode>>& getNodeMap();

    const std::vector<std::shared_ptr<AssimpBone>>& getBoneList();
    const std::vector<std::string> getBoneNameList();

    VkShaderStorageBufferData& getBoneMatrixOffsetBuffer();
    VkShaderStorageBufferData& getBoneParentBuffer();
    VkShaderStorageBufferData& getAnimLookupBuffer();

    std::vector<int32_t> getBoneParentIndexList();

    void setModelSettings(ModelSettings settings);
    ModelSettings getModelSettings();

    VkDescriptorSet& getTransformDescriptorSet();
    VkDescriptorSet& getMatrixMultDescriptorSet();
    VkDescriptorSet& getMatrixMultEmptyOffsetDescriptorSet();
    VkDescriptorSet& getBoundingSphereDescriptorSet();

    void updateBoundingSphereAdjustments(VkRenderData &renderData);

    void setAABBLookup(std::vector<std::vector<AABB>> lookupData);
    AABB getAABB(InstanceSettings instSettings);
    AABB getAnimatedAABB(InstanceSettings instSettings);
    AABB getNonAnimatedAABB(InstanceSettings instSettings);

    bool hasAnimMeshes();
    unsigned int getAnimMeshVertexSize();

    bool hasHeadMovementAnimationsMapped();

    glm::mat4 getBoneOffsetMatrix(int boneId);
    glm::mat4 getInverseBoneOffsetMatrix(int boneId);

    void setIkNodeChain(int footId, int effektorNode, int targetNode);

    void setAsNavigationTarget(bool value);
    bool isNavigationTarget();

    void cleanup(VkRenderData &renderData);

private:
    void processNode(VkRenderData &renderData, std::shared_ptr<AssimpNode> node, aiNode* aNode, const aiScene* scene, std::string assetDirectory);
    void createNodeList(std::shared_ptr<AssimpNode> node, std::shared_ptr<AssimpNode> newNode, std::vector<std::shared_ptr<AssimpNode>> &list);
    void drawInstanced(VkRenderData &renderData, int bufferIndex, uint32_t instanceCount, bool selectionModeActive, bool drawMorphMeshes);

    bool createDescriptorSet(VkRenderData &renderData);

    unsigned int mTriangleCount = 0;
    unsigned int mVertexCount = 0;

    float mMaxClipDuration = 0.0f;

    /* store the root node for direct access */
    std::shared_ptr<AssimpNode> mRootNode = nullptr;
    /* a map to find the node by name */
    std::unordered_map<std::string, std::shared_ptr<AssimpNode>> mNodeMap{};
    /* and a 'flat' map to keep the order of insertation  */
    std::vector<std::shared_ptr<AssimpNode>> mNodeList{};

    std::vector<std::shared_ptr<AssimpBone>> mBoneList;
    std::vector<std::string> mBoneNameList{};

    std::vector<glm::mat4> mBoneOffsetMatricesList{};
    std::vector<glm::mat4> mInverseBoneOffsetMatricesList{};

    std::vector<std::shared_ptr<AssimpAnimClip>> mAnimClips{};

    std::vector<VkMesh> mModelMeshes{};
    std::vector<VkVertexBufferData> mVertexBuffers{};
    std::vector<VkIndexBufferData> mIndexBuffers{};

    VkShaderStorageBufferData mShaderBoneParentBuffer{};
    std::vector<int32_t> mBoneParentIndexList{};

    VkShaderStorageBufferData mShaderBoneMatrixOffsetBuffer{};
    VkShaderStorageBufferData mEmptyBoneOffsetBuffer{};
    VkShaderStorageBufferData mInverseBoneMatrixOffsetBuffer{};
    VkShaderStorageBufferData mAnimLookupBuffer{};

    /* per-model-and-node adjustments for the spheres */
    VkShaderStorageBufferData mBoundingSphereAdjustmentBuffer{};

    // map textures to external or internal texture names
    std::unordered_map<std::string, VkTextureData> mTextures{};
    VkTextureData mPlaceholderTexture{};
    VkTextureData mWhiteTexture{};

    glm::mat4 mRootTransformMatrix = glm::mat4(1.0f);

    ModelSettings mModelSettings{};

    std::vector<std::vector<AABB>> mAabbLookups{};

    unsigned int mNumAnimatedMeshes = 0;
    unsigned int mAnimatedMeshVertexSize = 0;
    VkShaderStorageBufferData mAnimMeshVerticesBuffer{};

    VkDescriptorSet mTransformPerModelDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet mMatrixMultPerModelDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet mMatrixMultPerModelEmptyOffsetDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet mBoundingSphereAdjustmentPerModelDescriptorSet = VK_NULL_HANDLE;
    void updateBoundingSphereDescriptorSet(VkRenderData &renderData);

    VkDescriptorSet mMorphAnimPerModelDescriptorSet = VK_NULL_HANDLE;
};
