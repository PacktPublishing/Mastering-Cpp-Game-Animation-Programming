#include <algorithm>
#include <filesystem>

#include <glm/gtx/quaternion.hpp>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "AssimpModel.h"
#include "Tools.h"
#include "Logger.h"

bool AssimpModel::loadModel(VkRenderData &renderData, std::string modelFilename, unsigned int extraImportFlags) {
  Logger::log(1, "%s: loading model from file '%s'\n", __FUNCTION__, modelFilename.c_str());

  Assimp::Importer importer;
  /* we need to flip texture coordinates for Vulkan */
  const aiScene *scene = importer.ReadFile(modelFilename,
    aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_ValidateDataStructure | aiProcess_FlipUVs | extraImportFlags);

  if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    Logger::log(1, "%s error: assimp error '%s' while loading file '%s'\n", __FUNCTION__, importer.GetErrorString(), modelFilename.c_str());
    return false;
  }

  unsigned int numMeshes = scene->mNumMeshes;
  Logger::log(1, "%s: found %i mesh%s\n", __FUNCTION__, numMeshes, numMeshes == 1 ? "" : "es");

  for (unsigned int i = 0; i < numMeshes; ++i) {
    unsigned int numVertices = scene->mMeshes[i]->mNumVertices;
    unsigned int numFaces = scene->mMeshes[i]->mNumFaces;

    mVertexCount += numVertices;
    mTriangleCount += numFaces;

    Logger::log(1, "%s: mesh %i contains %i vertices and %i faces\n", __FUNCTION__, i, numVertices, numFaces);
  }
  Logger::log(1, "%s: model contains %i vertices and %i faces\n", __FUNCTION__, mVertexCount, mTriangleCount);

  aiNode* rootNode = scene->mRootNode;

  if (scene->HasTextures()) {
    unsigned int numTextures = scene->mNumTextures;

    for (int i = 0; i < scene->mNumTextures; ++i) {
      std::string texName = scene->mTextures[i]->mFilename.C_Str();

      int height = scene->mTextures[i]->mHeight;
      int width = scene->mTextures[i]->mWidth;
      aiTexel* data = scene->mTextures[i]->pcData;

      VkTextureData newTex{};
      if (!Texture::loadTexture(renderData, newTex, texName, data, width, height)) {
        return false;
      }

      std::string internalTexName = "*" + std::to_string(i);
      Logger::log(1, "%s: - added internal texture '%s'\n", __FUNCTION__, internalTexName.c_str());
      mTextures.insert({internalTexName, newTex});
    }

    Logger::log(1, "%s: scene has %i embedded textures\n", __FUNCTION__, numTextures);
  }

  /* add a white texture in case there is no diffuse tex but colors */
  std::string whiteTexName = "textures/white.png";
  if (!Texture::loadTexture(renderData, mWhiteTexture, whiteTexName)) {
    Logger::log(1, "%s error: could not load white default texture '%s'\n", __FUNCTION__, whiteTexName.c_str());
    return false;
  }

  /* add a placeholder texture in case there is no diffuse tex */
  std::string placeholderTexName = "textures/missing_tex.png";
  if (!Texture::loadTexture(renderData, mPlaceholderTexture, placeholderTexName)) {
    Logger::log(1, "%s error: could not load placeholder texture '%s'\n", __FUNCTION__, placeholderTexName.c_str());
    return false;
  }

  /* the textures are stored directly or relative to the model file */
  std::string assetDirectory = modelFilename.substr(0, modelFilename.find_last_of('/'));

  /* nodes */
  Logger::log(1, "%s: ... processing nodes...\n", __FUNCTION__);

  std::string rootNodeName = rootNode->mName.C_Str();
  mRootNode = AssimpNode::createNode(rootNodeName);
  Logger::log(2, "%s: root node name: '%s'\n", __FUNCTION__, rootNodeName.c_str());

  processNode(renderData, mRootNode, rootNode, scene, assetDirectory);

  Logger::log(1, "%s: ... processing nodes finished...\n", __FUNCTION__);

  for (const auto& entry : mNodeList) {
    std::vector<std::shared_ptr<AssimpNode>> childNodes = entry->getChilds();

    std::string parentName = entry->getParentNodeName();
    Logger::log(1, "%s: --- found node %s in node list, it has %i children, parent is %s\n", __FUNCTION__, entry->getNodeName().c_str(), childNodes.size(), parentName.c_str());

    for (const auto& node : childNodes) {
      Logger::log(1, "%s: ---- child: %s\n", __FUNCTION__, node->getNodeName().c_str());
    }
  }

  for (const auto& bone : mBoneList) {
    mBoneOffsetMatricesList.emplace_back(bone->getOffsetMatrix());
    mInverseBoneOffsetMatricesList.emplace_back(glm::inverse(bone->getOffsetMatrix()));

    std::string parentNodeName = mNodeMap.at(bone->getBoneName())->getParentNodeName();
    const auto boneIter = std::find_if(mBoneList.begin(), mBoneList.end(), [parentNodeName](std::shared_ptr<AssimpBone>& bone) { return bone->getBoneName() == parentNodeName; });
    if (boneIter == mBoneList.end()) {
        mBoneParentIndexList.emplace_back(-1); // root node gets a -1 to identify
    } else {
        mBoneParentIndexList.emplace_back(std::distance(mBoneList.begin(), boneIter));
    }
  }

  Logger::log(1, "%s: -- bone parents --\n", __FUNCTION__);
  for (unsigned int i = 0; i < mBoneList.size(); ++i) {
    Logger::log(1, "%s: bone %i (%s) has parent %i (%s)\n", __FUNCTION__, i, mBoneList.at(i)->getBoneName().c_str(), mBoneParentIndexList.at(i),
      mBoneParentIndexList.at(i) < 0 ? "invalid" : mBoneList.at(mBoneParentIndexList.at(i))->getBoneName().c_str());
  }
  Logger::log(1, "%s: -- bone parents --\n", __FUNCTION__);

  /* create vertex buffers for the meshes */
  for (const auto& mesh : mModelMeshes) {
    VkVertexBufferData vertexBuffer;
    VertexBuffer::init(renderData, vertexBuffer, mesh.vertices.size() * sizeof(VkVertex));
    VertexBuffer::uploadData(renderData, vertexBuffer, mesh);
    mVertexBuffers.emplace_back(vertexBuffer);

    VkIndexBufferData indexBuffer;
    IndexBuffer::init(renderData, indexBuffer, mesh.indices.size() * sizeof(uint32_t));
    IndexBuffer::uploadData(renderData, indexBuffer, mesh);
    mIndexBuffers.emplace_back(indexBuffer);
  }

  /* init all SSBOs */
  ShaderStorageBuffer::init(renderData, mAnimMeshVerticesBuffer);
  ShaderStorageBuffer::init(renderData, mAnimLookupBuffer);
  ShaderStorageBuffer::init(renderData, mShaderBoneMatrixOffsetBuffer);
  ShaderStorageBuffer::init(renderData, mInverseBoneMatrixOffsetBuffer);
  ShaderStorageBuffer::init(renderData, mEmptyBoneOffsetBuffer);
  ShaderStorageBuffer::init(renderData, mShaderBoneParentBuffer);
  ShaderStorageBuffer::init(renderData, mBoundingSphereAdjustmentBuffer);

  /* create a SSBOs containing all vertices for all morph animation of this mesh */
  for (const auto& mesh : mModelMeshes) {
    if (mesh.morphMeshes.empty()) {
      continue;
    }
    VkMorphMesh animMesh;
    animMesh.morphVertices.resize(mesh.vertices.size() * mNumAnimatedMeshes);

    for (unsigned int i = 0; i < mNumAnimatedMeshes; ++i) {
      unsigned int vertexOffset = mesh.vertices.size() * i;
      std::copy(mesh.morphMeshes[i].morphVertices.begin(), mesh.morphMeshes[i].morphVertices.end(),
                animMesh.morphVertices.begin() + vertexOffset);
      mAnimatedMeshVertexSize = mesh.vertices.size();
    }

    ShaderStorageBuffer::uploadSsboData(renderData, mAnimMeshVerticesBuffer, animMesh.morphVertices);
    Logger::log(1, "%s: model has %i morphs, SSBO has %i vertices\n", __FUNCTION__, mNumAnimatedMeshes, mAnimatedMeshVertexSize);
  }

  /* animations */
  unsigned int numAnims = scene->mNumAnimations;
  for (unsigned int i = 0; i < numAnims; ++i) {
    aiAnimation* animation = scene->mAnimations[i];
    mMaxClipDuration = std::max(mMaxClipDuration, static_cast<float>(animation->mDuration));
  }
  Logger::log(1, "%s: longest clip duration is %f\n", __FUNCTION__, mMaxClipDuration);

  for (unsigned int i = 0; i < numAnims; ++i) {
    aiAnimation* animation = scene->mAnimations[i];

    Logger::log(1, "%s: -- animation clip %i has %i skeletal channels, %i mesh channels, and %i morph mesh channels\n",
      __FUNCTION__, i, animation->mNumChannels, animation->mNumMeshChannels, animation->mNumMorphMeshChannels);

    /* skeletal animations */
    if (animation->mNumChannels > 0) {
      std::shared_ptr<AssimpAnimClip> animClip = std::make_shared<AssimpAnimClip>();
      animClip->addChannels(animation, mMaxClipDuration, mBoneList);
      if (animClip->getClipName().empty()) {
        animClip->setClipName(std::to_string(i));
      }
      mAnimClips.emplace_back(animClip);
    }

    /* morph mesh channels  */
    if (animation->mNumMorphMeshChannels) {
      std::string clipName = animation->mName.C_Str();
      Logger::log(1, "%s: morph mesh animation '%s'\n", __FUNCTION__, clipName.c_str());
      for (unsigned int i = 0; i < animation->mNumMorphMeshChannels; ++i) {
        std::string meshName = animation->mMorphMeshChannels[i]->mName.C_Str();
        unsigned int numKeys = animation->mMorphMeshChannels[i]->mNumKeys;

        Logger::log(1, "%s: channel %i for morphing mesh %s has %i key(s)\n", __FUNCTION__, i, meshName.c_str(), numKeys);
        for (unsigned int k = 0; k < numKeys; ++k) {
          double time = animation->mMorphMeshChannels[i]->mKeys[k].mTime;
          unsigned int numValues = animation->mMorphMeshChannels[i]->mKeys[k].mNumValuesAndWeights;

          Logger::log(1, "%s: -- morph key %i has time %lf with %i value(s) and weight(s)\n", __FUNCTION__, k, time, numValues);

          for (unsigned int j = 0; j < numValues; ++j) {
            Logger::log(1, "%s: --- morph key %i val %i, weigth %lf\n", __FUNCTION__, k, j,
                        &animation->mMorphMeshChannels[i]->mKeys[k].mValues[j], &animation->mMorphMeshChannels[i]->mKeys[k].mWeights[j]);

          }
        }
      }
    }
  }

  if (!mAnimClips.empty()) {
    std::vector<glm::vec4> animLookupData{};

    /* store inverse scaling factor in first element of lookup row */
    const int LOOKUP_SIZE = 1023 + 1;

    std::vector<glm::vec4> emptyTranslateVector(LOOKUP_SIZE, glm::vec4(0.0f));
    emptyTranslateVector.at(0) = glm::vec4(0.0f);

    std::vector<glm::vec4> emptyRotateVector(LOOKUP_SIZE, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)); // x, y, z, w
    emptyRotateVector.at(0) = glm::vec4(0.0f);

    std::vector<glm::vec4> emptyScaleVector(LOOKUP_SIZE, glm::vec4(1.0f));
    emptyScaleVector.at(0) = glm::vec4(0.0f);

    /* init all transform values with defaults  */
    for (int i = 0; i < mBoneList.size() * mAnimClips.at(0)->getNumChannels(); ++i) {
      animLookupData.insert(animLookupData.end(), emptyTranslateVector.begin(), emptyTranslateVector.end());
      animLookupData.insert(animLookupData.end(), emptyRotateVector.begin(), emptyRotateVector.end());
      animLookupData.insert(animLookupData.end(), emptyScaleVector.begin(), emptyScaleVector.end());
    }

    for (int clipId = 0; clipId < mAnimClips.size(); ++clipId) {
      Logger::log(1, "%s: generating lookup data for clip %i\n", __FUNCTION__, clipId);
      for (const auto& channel : mAnimClips.at(clipId)->getChannels()) {
        int boneId = channel->getBoneId();
        if (boneId >= 0) {
          int offset = clipId * mBoneList.size() * LOOKUP_SIZE * 3 + boneId * LOOKUP_SIZE * 3;

          animLookupData.at(offset) = glm::vec4(channel->getInvTranslationScaling(), 0.0f, 0.0f, 0.0f);
          const std::vector<glm::vec4>& translations = channel->getTranslationData();
          std::copy(translations.begin(), translations.end(), animLookupData.begin() + offset + 1);

          offset += LOOKUP_SIZE;
          animLookupData.at(offset) = glm::vec4(channel->getInvRotationScaling(), 0.0f, 0.0f, 0.0f);
          const std::vector<glm::vec4>& rotations = channel->getRotationData();
          std::copy(rotations.begin(), rotations.end(), animLookupData.begin() + offset + 1);

          offset += LOOKUP_SIZE;
          animLookupData.at(offset) = glm::vec4(channel->getInvScaleScaling(), 0.0f, 0.0f, 0.0f);
          const std::vector<glm::vec4>& scalings = channel->getScalingData();
          std::copy(scalings.begin(), scalings.end(), animLookupData.begin() + offset + 1);
        }
      }
    }

    Logger::log(1, "%s: generated %i elements of lookup data (%i bytes)\n", __FUNCTION__, animLookupData.size(), animLookupData.size() * sizeof(glm::vec4));
    ShaderStorageBuffer::uploadSsboData(renderData, mAnimLookupBuffer, animLookupData);
  }

  ShaderStorageBuffer::uploadSsboData(renderData, mShaderBoneMatrixOffsetBuffer, mBoneOffsetMatricesList);
  ShaderStorageBuffer::uploadSsboData(renderData, mInverseBoneMatrixOffsetBuffer, mInverseBoneOffsetMatricesList);
  ShaderStorageBuffer::uploadSsboData(renderData, mShaderBoneParentBuffer, mBoneParentIndexList);

  /* we MUST set bone offsets to identity matrices to get the skeleton data for the AABBs */
  std::vector<glm::mat4> emptyBoneOffsets(mBoneList.size(), glm::mat4(1.0f));
  ShaderStorageBuffer::uploadSsboData(renderData, mEmptyBoneOffsetBuffer, emptyBoneOffsets);

  mModelSettings.msModelFilenamePath = modelFilename;
  mModelSettings.msModelFilename = std::filesystem::path(modelFilename).filename().generic_string();

  /* get root transformation matrix from model's root node */
  mRootTransformMatrix = Tools::convertAiToGLM(rootNode->mTransformation);

  if (!mBoneList.empty()) {
    for (const auto& bone: mBoneList) {
      mBoneNameList.emplace_back(bone->getBoneName());
    }
    mModelSettings.msBoundingSphereAdjustments = std::vector(mBoneList.size(), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
  }

  ShaderStorageBuffer::uploadSsboData(renderData, mBoundingSphereAdjustmentBuffer, mModelSettings.msBoundingSphereAdjustments);

  /* create descriptor set for per-model data */
  createDescriptorSet(renderData);

  Logger::log(1, "%s: - model has a total of %i texture%s\n", __FUNCTION__, mTextures.size(), mTextures.size() == 1 ? "" : "s");
  Logger::log(1, "%s: - model has a total of %i bone%s\n", __FUNCTION__, mBoneList.size(), mBoneList.size() == 1 ? "" : "s");
  Logger::log(1, "%s: - model has a total of %i skeletal animation%s\n", __FUNCTION__, numAnims, numAnims == 1 ? "" : "s");
  Logger::log(1, "%s: - model has a total of %i morph animation%s\n", __FUNCTION__, mNumAnimatedMeshes, mNumAnimatedMeshes == 1 ? "" : "s");

  Logger::log(1, "%s: successfully loaded model '%s' (%s)\n", __FUNCTION__, modelFilename.c_str(), mModelSettings.msModelFilename.c_str());
  return true;
}

void AssimpModel::processNode(VkRenderData &renderData, std::shared_ptr<AssimpNode> node, aiNode* aNode, const aiScene* scene, std::string assetDirectory) {
  std::string nodeName = aNode->mName.C_Str();
  Logger::log(1, "%s: node name: '%s'\n", __FUNCTION__, nodeName.c_str());

  unsigned int numMeshes = aNode->mNumMeshes;
  if (numMeshes > 0) {
    Logger::log(1, "%s: - node has %i meshes\n", __FUNCTION__, numMeshes);
    for (unsigned int i = 0; i < numMeshes; ++i) {
      aiMesh* modelMesh = scene->mMeshes[aNode->mMeshes[i]];

      AssimpMesh mesh;
      mesh.processMesh(renderData, modelMesh, scene, assetDirectory, mTextures);
      VkMesh vertexMesh = mesh.getMesh();
      mNumAnimatedMeshes += vertexMesh.morphMeshes.size();

      mModelMeshes.emplace_back(vertexMesh);

      /* avoid inserting duplicate bone Ids - meshes can reference the same bones */
      std::vector<std::shared_ptr<AssimpBone>> flatBones = mesh.getBoneList();
      for (const auto& bone : flatBones) {
        const auto iter = std::find_if(mBoneList.begin(), mBoneList.end(), [bone](std::shared_ptr<AssimpBone>& otherBone) { return bone->getBoneId() == otherBone->getBoneId(); });
        if (iter == mBoneList.end()) {
          mBoneList.emplace_back(bone);
        }
      }
    }
  }

  mNodeMap.insert({nodeName, node});
  mNodeList.emplace_back(node);

  unsigned int numChildren = aNode->mNumChildren;
  Logger::log(1, "%s: - node has %i children \n", __FUNCTION__, numChildren);

  for (unsigned int i = 0; i < numChildren; ++i) {
    std::string childName = aNode->mChildren[i]->mName.C_Str();
    Logger::log(1, "%s: --- found child node '%s'\n", __FUNCTION__, childName.c_str());

    std::shared_ptr<AssimpNode> childNode = node->addChild(childName);
    processNode(renderData, childNode, aNode->mChildren[i], scene, assetDirectory);
  }
}

glm::mat4 AssimpModel::getRootTranformationMatrix() {
  return mRootTransformMatrix;
}

bool AssimpModel::createDescriptorSet(VkRenderData& renderData) {
  {
    /* matrix multiplication, per-model data */
    VkDescriptorSetAllocateInfo computeMatrixMultPerModelDescriptorAllocateInfo{};
    computeMatrixMultPerModelDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    computeMatrixMultPerModelDescriptorAllocateInfo.descriptorPool = renderData.rdDescriptorPool;
    computeMatrixMultPerModelDescriptorAllocateInfo.descriptorSetCount = 1;
    computeMatrixMultPerModelDescriptorAllocateInfo.pSetLayouts = &renderData.rdAssimpComputeMatrixMultPerModelDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(renderData.rdVkbDevice.device, &computeMatrixMultPerModelDescriptorAllocateInfo,
      &mMatrixMultPerModelDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp Matrix Mult Compute per-model descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }

    VkDescriptorBufferInfo parentNodeInfo{};
    parentNodeInfo.buffer = mShaderBoneParentBuffer.buffer;
    parentNodeInfo.offset = 0;
    parentNodeInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo boneOffsetInfo{};
    boneOffsetInfo.buffer = mShaderBoneMatrixOffsetBuffer.buffer;
    boneOffsetInfo.offset = 0;
    boneOffsetInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet parentNodeWriteDescriptorSet{};
    parentNodeWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    parentNodeWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    parentNodeWriteDescriptorSet.dstSet = mMatrixMultPerModelDescriptorSet;
    parentNodeWriteDescriptorSet.dstBinding = 0;
    parentNodeWriteDescriptorSet.descriptorCount = 1;
    parentNodeWriteDescriptorSet.pBufferInfo = &parentNodeInfo;

    VkWriteDescriptorSet boneOffsetWriteDescriptorSet{};
    boneOffsetWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    boneOffsetWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    boneOffsetWriteDescriptorSet.dstSet = mMatrixMultPerModelDescriptorSet;
    boneOffsetWriteDescriptorSet.dstBinding = 1;
    boneOffsetWriteDescriptorSet.descriptorCount = 1;
    boneOffsetWriteDescriptorSet.pBufferInfo = &boneOffsetInfo;

    std::vector<VkWriteDescriptorSet> matrixMultWriteDescriptorSets =
      { parentNodeWriteDescriptorSet, boneOffsetWriteDescriptorSet };

    vkUpdateDescriptorSets(renderData.rdVkbDevice.device, static_cast<uint32_t>(matrixMultWriteDescriptorSets.size()),
       matrixMultWriteDescriptorSets.data(), 0, nullptr);
  }

  {
    /* matrix multiplication, per-model data but with identity matrix as bone matrix offsets */
    VkDescriptorSetAllocateInfo computeMatrixMultPerModelDescriptorAllocateInfo{};
    computeMatrixMultPerModelDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    computeMatrixMultPerModelDescriptorAllocateInfo.descriptorPool = renderData.rdDescriptorPool;
    computeMatrixMultPerModelDescriptorAllocateInfo.descriptorSetCount = 1;
    computeMatrixMultPerModelDescriptorAllocateInfo.pSetLayouts = &renderData.rdAssimpComputeMatrixMultPerModelDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(renderData.rdVkbDevice.device, &computeMatrixMultPerModelDescriptorAllocateInfo,
      &mMatrixMultPerModelEmptyOffsetDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp Emtpy Bone Offset Matrix Mult Compute per-model descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }

    VkDescriptorBufferInfo parentNodeInfo{};
    parentNodeInfo.buffer = mShaderBoneParentBuffer.buffer;
    parentNodeInfo.offset = 0;
    parentNodeInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo boneOffsetInfo{};
    boneOffsetInfo.buffer = mEmptyBoneOffsetBuffer.buffer;
    boneOffsetInfo.offset = 0;
    boneOffsetInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet parentNodeWriteDescriptorSet{};
    parentNodeWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    parentNodeWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    parentNodeWriteDescriptorSet.dstSet = mMatrixMultPerModelEmptyOffsetDescriptorSet;
    parentNodeWriteDescriptorSet.dstBinding = 0;
    parentNodeWriteDescriptorSet.descriptorCount = 1;
    parentNodeWriteDescriptorSet.pBufferInfo = &parentNodeInfo;

    VkWriteDescriptorSet boneOffsetWriteDescriptorSet{};
    boneOffsetWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    boneOffsetWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    boneOffsetWriteDescriptorSet.dstSet = mMatrixMultPerModelEmptyOffsetDescriptorSet;
    boneOffsetWriteDescriptorSet.dstBinding = 1;
    boneOffsetWriteDescriptorSet.descriptorCount = 1;
    boneOffsetWriteDescriptorSet.pBufferInfo = &boneOffsetInfo;

    std::vector<VkWriteDescriptorSet> matrixMultWriteDescriptorSets =
      { parentNodeWriteDescriptorSet, boneOffsetWriteDescriptorSet };

    vkUpdateDescriptorSets(renderData.rdVkbDevice.device, static_cast<uint32_t>(matrixMultWriteDescriptorSets.size()),
       matrixMultWriteDescriptorSets.data(), 0, nullptr);
  }

  {
    /* transform, per-model */
    VkDescriptorSetAllocateInfo computeTransformPerModelDescriptorAllocateInfo{};
    computeTransformPerModelDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    computeTransformPerModelDescriptorAllocateInfo.descriptorPool = renderData.rdDescriptorPool;
    computeTransformPerModelDescriptorAllocateInfo.descriptorSetCount = 1;
    computeTransformPerModelDescriptorAllocateInfo.pSetLayouts = &renderData.rdAssimpComputeTransformPerModelDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(renderData.rdVkbDevice.device, &computeTransformPerModelDescriptorAllocateInfo,
      &mTransformPerModelDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp Transform Compute per-model descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }

    VkDescriptorBufferInfo animLookupInfo{};
    animLookupInfo.buffer = mAnimLookupBuffer.buffer;
    animLookupInfo.offset = 0;
    animLookupInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet animLookupWriteDescriptorSet{};
    animLookupWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    animLookupWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    animLookupWriteDescriptorSet.dstSet = mTransformPerModelDescriptorSet;
    animLookupWriteDescriptorSet.dstBinding = 0;
    animLookupWriteDescriptorSet.descriptorCount = 1;
    animLookupWriteDescriptorSet.pBufferInfo = &animLookupInfo;

    vkUpdateDescriptorSets(renderData.rdVkbDevice.device, 1, &animLookupWriteDescriptorSet, 0, nullptr);
  }

  {
    /* bounding sphere adjustments, per-model */
    VkDescriptorSetAllocateInfo computeBoundingSphereAdjustmentsPerModelDescriptorAllocateInfo{};
    computeBoundingSphereAdjustmentsPerModelDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    computeBoundingSphereAdjustmentsPerModelDescriptorAllocateInfo.descriptorPool = renderData.rdDescriptorPool;
    computeBoundingSphereAdjustmentsPerModelDescriptorAllocateInfo.descriptorSetCount = 1;
    computeBoundingSphereAdjustmentsPerModelDescriptorAllocateInfo.pSetLayouts = &renderData.rdAssimpComputeBoundingSpheresPerModelDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(renderData.rdVkbDevice.device, &computeBoundingSphereAdjustmentsPerModelDescriptorAllocateInfo,
      &mBoundingSphereAdjustmentPerModelDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp Bounding Sphere Adjustment per-model descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }

    updateBoundingSphereDescriptorSet(renderData);
  }

  {
    /* morph anim, per-model */
    VkDescriptorSetAllocateInfo morphAnimPerModelDescriptorAllocateInfo{};
    morphAnimPerModelDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    morphAnimPerModelDescriptorAllocateInfo.descriptorPool = renderData.rdDescriptorPool;
    morphAnimPerModelDescriptorAllocateInfo.descriptorSetCount = 1;
    morphAnimPerModelDescriptorAllocateInfo.pSetLayouts = &renderData.rdAssimpSkinningMorphPerModelDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(renderData.rdVkbDevice.device, &morphAnimPerModelDescriptorAllocateInfo,
      &mMorphAnimPerModelDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp Morph Anim Vertex per-model descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }

    VkDescriptorBufferInfo morphAnimInfo{};
    morphAnimInfo.buffer = mAnimMeshVerticesBuffer.buffer;
    morphAnimInfo.offset = 0;
    morphAnimInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet morphAnimWriteDescriptorSet{};
    morphAnimWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    morphAnimWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    morphAnimWriteDescriptorSet.dstSet = mMorphAnimPerModelDescriptorSet;
    morphAnimWriteDescriptorSet.dstBinding = 0;
    morphAnimWriteDescriptorSet.descriptorCount = 1;
    morphAnimWriteDescriptorSet.pBufferInfo = &morphAnimInfo;

    vkUpdateDescriptorSets(renderData.rdVkbDevice.device, 1, &morphAnimWriteDescriptorSet, 0, nullptr);
  }

  return true;
}

void AssimpModel::updateBoundingSphereDescriptorSet(VkRenderData &renderData) {
  VkDescriptorBufferInfo parentNodeInfo{};
  parentNodeInfo.buffer = mShaderBoneParentBuffer.buffer;
  parentNodeInfo.offset = 0;
  parentNodeInfo.range = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo boundingSphereAdjustmentInfo{};
  boundingSphereAdjustmentInfo.buffer = mBoundingSphereAdjustmentBuffer.buffer;
  boundingSphereAdjustmentInfo.offset = 0;
  boundingSphereAdjustmentInfo.range = VK_WHOLE_SIZE;

  VkWriteDescriptorSet parentNodeWriteDescriptorSet{};
  parentNodeWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  parentNodeWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  parentNodeWriteDescriptorSet.dstSet = mBoundingSphereAdjustmentPerModelDescriptorSet;
  parentNodeWriteDescriptorSet.dstBinding = 0;
  parentNodeWriteDescriptorSet.descriptorCount = 1;
  parentNodeWriteDescriptorSet.pBufferInfo = &parentNodeInfo;

  VkWriteDescriptorSet boundingSphereAdjustmentWriteDescriptorSet{};
  boundingSphereAdjustmentWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  boundingSphereAdjustmentWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  boundingSphereAdjustmentWriteDescriptorSet.dstSet = mBoundingSphereAdjustmentPerModelDescriptorSet;
  boundingSphereAdjustmentWriteDescriptorSet.dstBinding = 1;
  boundingSphereAdjustmentWriteDescriptorSet.descriptorCount = 1;
  boundingSphereAdjustmentWriteDescriptorSet.pBufferInfo = &boundingSphereAdjustmentInfo;

  std::vector<VkWriteDescriptorSet> boundingSpheresWriteDescriptorSets =
  { parentNodeWriteDescriptorSet, boundingSphereAdjustmentWriteDescriptorSet };

  vkUpdateDescriptorSets(renderData.rdVkbDevice.device, static_cast<uint32_t>(boundingSpheresWriteDescriptorSets.size()),
    boundingSpheresWriteDescriptorSets.data(), 0, nullptr);
}

void AssimpModel::updateBoundingSphereAdjustments(VkRenderData& renderData) {
  ShaderStorageBuffer::uploadSsboData(renderData, mBoundingSphereAdjustmentBuffer, mModelSettings.msBoundingSphereAdjustments);
  updateBoundingSphereDescriptorSet(renderData);
}

void AssimpModel::draw(VkRenderData &renderData, bool selectionModeActive) {
  for (unsigned int i = 0; i < mModelMeshes.size(); ++i) {
    VkMesh& mesh = mModelMeshes.at(i);

    // find diffuse texture by name
    VkTextureData diffuseTex{};
    auto diffuseTexName = mesh.textures.find(aiTextureType_DIFFUSE);
    if (diffuseTexName != mesh.textures.end()) {
      auto diffuseTexture = mTextures.find(diffuseTexName->second);
      if (diffuseTexture != mTextures.end()) {
        diffuseTex = diffuseTexture->second;
      }
    }

    /* switch between animated and non-animated pipeline layout */
    VkPipelineLayout renderLayout;
    if (hasAnimations()) {
      if (selectionModeActive) {
        renderLayout = renderData.rdAssimpSkinningSelectionPipelineLayout;
      } else {
        renderLayout = renderData.rdAssimpSkinningPipelineLayout;
      }
    } else {
      if (selectionModeActive) {
        renderLayout = renderData.rdAssimpSelectionPipelineLayout;
      } else {
        renderLayout = renderData.rdAssimpPipelineLayout;
      }
    }

    if (diffuseTex.image != VK_NULL_HANDLE) {
      vkCmdBindDescriptorSets(renderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        renderLayout, 0, 1, &diffuseTex.descriptorSet, 0, nullptr);
    } else {
      if (mesh.usesPBRColors) {
        vkCmdBindDescriptorSets(renderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
          renderLayout, 0, 1, &mWhiteTexture.descriptorSet, 0, nullptr);
      } else {
        vkCmdBindDescriptorSets(renderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
          renderLayout, 0, 1, &mPlaceholderTexture.descriptorSet, 0, nullptr);
      }
    }

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(renderData.rdCommandBuffer, 0, 1, &mVertexBuffers.at(i).buffer, &offset);
    vkCmdBindIndexBuffer(renderData.rdCommandBuffer, mIndexBuffers.at(i).buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(renderData.rdCommandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
  }
}

void AssimpModel::drawInstanced(VkRenderData &renderData, uint32_t instanceCount, bool selectionModeActive) {
  for (unsigned int i = 0; i < mModelMeshes.size(); ++i) {
    drawInstanced(renderData, i, instanceCount, selectionModeActive, false);
  }
}

void AssimpModel::drawInstancedNoMorphAnims(VkRenderData &renderData, uint32_t instanceCount, bool selectionModeActive) {
  for (unsigned int i = 0; i < mModelMeshes.size(); ++i) {
    /* skip meshes with morph animations */
    if (!mModelMeshes.at(i).morphMeshes.empty()) {
      continue;
    }
    drawInstanced(renderData, i, instanceCount, selectionModeActive, false);
  }
}

void AssimpModel::drawInstancedMorphAnims(VkRenderData &renderData, uint32_t instanceCount, bool selectionModeActive) {
  for (unsigned int i = 0; i < mModelMeshes.size(); ++i) {
    /* draw only meshes with morph animations */
    if (mModelMeshes.at(i).morphMeshes.empty()) {
      continue;
    }
    drawInstanced(renderData, i, instanceCount, selectionModeActive, true);
  }
}

void AssimpModel::drawInstanced(VkRenderData &renderData, int bufferIndex,
    uint32_t instanceCount, bool selectionModeActive, bool drawMorphMeshes) {
  VkMesh& mesh = mModelMeshes.at(bufferIndex);
  // find diffuse texture by name
  VkTextureData diffuseTex{};
  auto diffuseTexName = mesh.textures.find(aiTextureType_DIFFUSE);
  if (diffuseTexName != mesh.textures.end()) {
    auto diffuseTexture = mTextures.find(diffuseTexName->second);
    if (diffuseTexture != mTextures.end()) {
      diffuseTex = diffuseTexture->second;
    }
  }

  /* switch between animated and non-animated pipeline layout */
  VkPipelineLayout renderLayout;
  if (hasAnimations()) {
    if (drawMorphMeshes) {
      if (selectionModeActive) {
        renderLayout = renderData.rdAssimpSkinningMorphSelectionPipelineLayout;
      } else {
        renderLayout = renderData.rdAssimpSkinningMorphPipelineLayout;
      }
    } else {
      if (selectionModeActive) {
        renderLayout = renderData.rdAssimpSkinningSelectionPipelineLayout;
      } else {
        renderLayout = renderData.rdAssimpSkinningPipelineLayout;
      }
    }
  } else {
    if (selectionModeActive) {
      renderLayout = renderData.rdAssimpSelectionPipelineLayout;
    } else {
      renderLayout = renderData.rdAssimpPipelineLayout;
    }
  }

  if (diffuseTex.image != VK_NULL_HANDLE) {
    vkCmdBindDescriptorSets(renderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      renderLayout, 0, 1, &diffuseTex.descriptorSet, 0, nullptr);
  } else {
    if (mesh.usesPBRColors) {
      vkCmdBindDescriptorSets(renderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        renderLayout, 0, 1, &mWhiteTexture.descriptorSet, 0, nullptr);
    } else {
      vkCmdBindDescriptorSets(renderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        renderLayout, 0, 1, &mPlaceholderTexture.descriptorSet, 0, nullptr);
    }
  }
  if (drawMorphMeshes) {
    vkCmdBindDescriptorSets(renderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      renderLayout, 2, 1, &mMorphAnimPerModelDescriptorSet, 0, nullptr);
  }

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(renderData.rdCommandBuffer, 0, 1, &mVertexBuffers.at(bufferIndex).buffer, &offset);
  vkCmdBindIndexBuffer(renderData.rdCommandBuffer, mIndexBuffers.at(bufferIndex).buffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(renderData.rdCommandBuffer, static_cast<uint32_t>(mesh.indices.size()), instanceCount, 0, 0, 0);
}

unsigned int AssimpModel::getTriangleCount() {
  return mTriangleCount;
}

void AssimpModel::cleanup(VkRenderData &renderData) {
  vkFreeDescriptorSets(renderData.rdVkbDevice.device, renderData.rdDescriptorPool, 1, &mTransformPerModelDescriptorSet);
  vkFreeDescriptorSets(renderData.rdVkbDevice.device, renderData.rdDescriptorPool, 1, &mMatrixMultPerModelDescriptorSet);
  vkFreeDescriptorSets(renderData.rdVkbDevice.device, renderData.rdDescriptorPool, 1, &mMatrixMultPerModelEmptyOffsetDescriptorSet);
  vkFreeDescriptorSets(renderData.rdVkbDevice.device, renderData.rdDescriptorPool, 1, &mBoundingSphereAdjustmentPerModelDescriptorSet);
  vkFreeDescriptorSets(renderData.rdVkbDevice.device, renderData.rdDescriptorPool, 1, &mMorphAnimPerModelDescriptorSet);

  for (auto buffer : mVertexBuffers) {
    VertexBuffer::cleanup(renderData, buffer);
  }
  for (auto buffer : mIndexBuffers) {
    IndexBuffer::cleanup(renderData, buffer);
  }

  ShaderStorageBuffer::cleanup(renderData, mShaderBoneMatrixOffsetBuffer);
  ShaderStorageBuffer::cleanup(renderData, mShaderBoneParentBuffer);
  ShaderStorageBuffer::cleanup(renderData, mAnimLookupBuffer);
  ShaderStorageBuffer::cleanup(renderData, mEmptyBoneOffsetBuffer);
  ShaderStorageBuffer::cleanup(renderData, mBoundingSphereAdjustmentBuffer);
  ShaderStorageBuffer::cleanup(renderData, mAnimMeshVerticesBuffer);
  ShaderStorageBuffer::cleanup(renderData, mInverseBoneMatrixOffsetBuffer);

  for (auto& tex : mTextures) {
    Texture::cleanup(renderData, tex.second);
  }

  Texture::cleanup(renderData, mPlaceholderTexture);
  Texture::cleanup(renderData, mWhiteTexture);
}

std::string AssimpModel::getModelFileName() {
  return mModelSettings.msModelFilename;
}

std::string AssimpModel::getModelFileNamePath() {
  return mModelSettings.msModelFilenamePath;
}

const std::vector<std::shared_ptr<AssimpNode>>& AssimpModel::getNodeList() {
  return mNodeList;
}

const std::unordered_map<std::string, std::shared_ptr<AssimpNode>>& AssimpModel::getNodeMap() {
  return mNodeMap;
}

const std::vector<std::shared_ptr<AssimpBone>>& AssimpModel::getBoneList() {
  return mBoneList;
}

const std::vector<std::string> AssimpModel::getBoneNameList() {
  return mBoneNameList;
}

const std::vector<std::shared_ptr<AssimpAnimClip>>& AssimpModel::getAnimClips() {
  return mAnimClips;
}

bool AssimpModel::hasAnimations() {
  return !mAnimClips.empty();
}

VkShaderStorageBufferData& AssimpModel::getBoneMatrixOffsetBuffer() {
  return mShaderBoneMatrixOffsetBuffer;
}

VkShaderStorageBufferData& AssimpModel::getBoneParentBuffer() {
  return mShaderBoneParentBuffer;
}

VkShaderStorageBufferData& AssimpModel::getAnimLookupBuffer() {
  return mAnimLookupBuffer;
}

VkDescriptorSet& AssimpModel::getTransformDescriptorSet() {
  return mTransformPerModelDescriptorSet;
}

VkDescriptorSet& AssimpModel::getMatrixMultDescriptorSet() {
  return mMatrixMultPerModelDescriptorSet;
}

VkDescriptorSet& AssimpModel::getMatrixMultEmptyOffsetDescriptorSet() {
  return mMatrixMultPerModelEmptyOffsetDescriptorSet;
}

VkDescriptorSet& AssimpModel::getBoundingSphereDescriptorSet() {
  return mBoundingSphereAdjustmentPerModelDescriptorSet;
}

std::vector<int32_t> AssimpModel::getBoneParentIndexList() {
  return mBoneParentIndexList;
}

void AssimpModel::setModelSettings(ModelSettings settings) {
  mModelSettings = settings;
}

ModelSettings AssimpModel::getModelSettings() {
  return mModelSettings;
}

float AssimpModel::getMaxClipDuration() {
  return mMaxClipDuration;
}

void AssimpModel::setAABBLookup(std::vector<std::vector<AABB>> lookupData) {
  mAabbLookups = lookupData;
}

AABB AssimpModel::getAABB(InstanceSettings instSettings) {
  if (hasAnimations()) {
    return getAnimatedAABB(instSettings);
  } else {
    return getNonAnimatedAABB(instSettings);
  }
}

AABB AssimpModel::getAnimatedAABB(InstanceSettings instSettings) {
  const int LOOKUP_SIZE = 1023;

  float timeScaleFactor = mMaxClipDuration / static_cast<float>(LOOKUP_SIZE);
  float invTimeScaleFactor = 1.0f / timeScaleFactor;

  /* get the AABBs of the two clips */
  int firstLookup = std::clamp(static_cast<int>(instSettings.isFirstClipAnimPlayTimePos * invTimeScaleFactor), 0, LOOKUP_SIZE - 1);
  AABB firstAabb =  mAabbLookups.at(instSettings.isFirstAnimClipNr).at(firstLookup);

  int secondLookup = std::clamp(static_cast<int>(instSettings.isSecondClipAnimPlayTimePos * invTimeScaleFactor), 0, LOOKUP_SIZE - 1);
  AABB secondAabb =  mAabbLookups.at(instSettings.isSecondAnimClipNr).at(secondLookup);

  /* interpolate between the two AABBs */
  AABB interpAabb;
  glm::vec3 pt1 = glm::mix(firstAabb.getMinPos(), secondAabb.getMinPos(), instSettings.isAnimBlendFactor);
  glm::vec3 pt2 = glm::mix(firstAabb.getMaxPos(), secondAabb.getMaxPos(), instSettings.isAnimBlendFactor);

  interpAabb.create(pt1);
  interpAabb.addPoint(glm::vec3(pt2.x, pt1.y, pt1.z));
  interpAabb.addPoint(glm::vec3(pt1.x, pt2.y, pt1.z));
  interpAabb.addPoint(glm::vec3(pt2.x, pt2.y, pt1.z));

  interpAabb.addPoint(glm::vec3(pt1.x, pt1.y, pt2.z));
  interpAabb.addPoint(glm::vec3(pt2.x, pt1.y, pt2.z));
  interpAabb.addPoint(glm::vec3(pt1.x, pt2.y, pt2.z));
  interpAabb.addPoint(pt2);

  /* scale AABB */
  interpAabb.setMinPos(interpAabb.getMinPos() * instSettings.isScale);
  interpAabb.setMaxPos(interpAabb.getMaxPos() * instSettings.isScale);

  /* honour swap axis */
  glm::quat swapAxisQuat = glm::identity<glm::quat>();
  if (instSettings.isSwapYZAxis) {
    glm::mat4 flipMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    swapAxisQuat = glm::quat_cast(glm::rotate(flipMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
  }

  /* rotate and re-create AABB from min and max positions */
  AABB rotatedAabb;
  glm::vec3 interpMinPos = interpAabb.getMinPos();
  glm::vec3 interpMaxPos = interpAabb.getMaxPos();
  glm::vec3 p1 = glm::quat(glm::radians(instSettings.isWorldRotation)) * swapAxisQuat * interpMinPos;
  glm::vec3 p2 = glm::quat(glm::radians(instSettings.isWorldRotation)) * swapAxisQuat *
  glm::vec3(interpMaxPos.x, interpMinPos.y, interpMinPos.z);
  glm::vec3 p3 = glm::quat(glm::radians(instSettings.isWorldRotation)) * swapAxisQuat *
  glm::vec3(interpMinPos.x, interpMaxPos.y, interpMinPos.z);
  glm::vec3 p4 = glm::quat(glm::radians(instSettings.isWorldRotation)) * swapAxisQuat *
  glm::vec3(interpMaxPos.x, interpMaxPos.y, interpMinPos.z);

  glm::vec3 p5 = glm::quat(glm::radians(instSettings.isWorldRotation)) * swapAxisQuat *
  glm::vec3(interpMinPos.x, interpMinPos.y, interpMaxPos.z);
  glm::vec3 p6 = glm::quat(glm::radians(instSettings.isWorldRotation)) * swapAxisQuat *
  glm::vec3(interpMaxPos.x, interpMinPos.y, interpMaxPos.z);
  glm::vec3 p7 = glm::quat(glm::radians(instSettings.isWorldRotation)) * swapAxisQuat *
  glm::vec3(interpMinPos.x, interpMaxPos.y, interpMaxPos.z);
  glm::vec3 p8 = glm::quat(glm::radians(instSettings.isWorldRotation)) * swapAxisQuat * interpMaxPos;

  rotatedAabb.create(p1);
  rotatedAabb.addPoint(p2);
  rotatedAabb.addPoint(p3);
  rotatedAabb.addPoint(p4);
  rotatedAabb.addPoint(p5);
  rotatedAabb.addPoint(p6);
  rotatedAabb.addPoint(p7);
  rotatedAabb.addPoint(p8);

  /* translate */
  AABB translatedAabb;
  translatedAabb.setMinPos(rotatedAabb.getMinPos() += instSettings.isWorldPosition);
  translatedAabb.setMaxPos(rotatedAabb.getMaxPos() += instSettings.isWorldPosition);

  return translatedAabb;
}

AABB AssimpModel::getNonAnimatedAABB(InstanceSettings instSettings) {
  glm::mat4 localScaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(instSettings.isScale));

  glm::mat4 localSwapAxisMatrix;
  if (instSettings.isSwapYZAxis) {
    glm::mat4 flipMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    localSwapAxisMatrix = glm::rotate(flipMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  } else {
    localSwapAxisMatrix = glm::mat4(1.0f);
  }

  glm::mat4 localRotationMatrix = glm::mat4_cast(glm::quat(glm::radians(instSettings.isWorldRotation)));

  glm::mat4 localTranslationMatrix = glm::translate(glm::mat4(1.0f), instSettings.isWorldPosition);

  glm::mat4 localTransformMatrix = localTranslationMatrix * localRotationMatrix *
    localSwapAxisMatrix * localScaleMatrix * mRootTransformMatrix;

  AABB modelAABB{};
  for (const auto& mesh : mModelMeshes) {
    for (const auto& vertex : mesh.vertices) {
      /* we use position.w for UV coordinates, set to 1.0f */
      modelAABB.addPoint(localTransformMatrix * glm::vec4(glm::vec3(vertex.position), 1.0f));
    }
  }

  return modelAABB;
}

bool AssimpModel::hasAnimMeshes() {
  return mNumAnimatedMeshes > 0;
}

unsigned int AssimpModel::getAnimMeshVertexSize() {
  return mAnimatedMeshVertexSize;
}

bool AssimpModel::hasHeadMovementAnimationsMapped() {
  if (mModelSettings.msHeadMoveClipMappings.size() < 4) {
    return false;
  }

  bool hasAllMappings = true;
  for (const auto& clip : mModelSettings.msHeadMoveClipMappings) {
    if (clip.second < 0) {
      hasAllMappings = false;
    }
  }
  return hasAllMappings;
}

glm::mat4 AssimpModel::getInverseBoneOffsetMatrix(int boneId) {
  if (boneId > mInverseBoneOffsetMatricesList.size()) {
    Logger::log(1, "%s error: inverse bone index out of range (want: %i, size: %i)\n", __FUNCTION__, boneId, mInverseBoneOffsetMatricesList.size());
    return glm::mat4(1.0f);
  }
  return mInverseBoneOffsetMatricesList.at(boneId);
}

glm::mat4 AssimpModel::getBoneOffsetMatrix(int boneId) {
  if (boneId > mBoneOffsetMatricesList.size()) {
    Logger::log(1, "%s error: bone index out of range (want: %i, size: %i)\n", __FUNCTION__, boneId, mBoneOffsetMatricesList.size());
    return glm::mat4(1.0f);
  }

  return mBoneOffsetMatricesList.at(boneId);
}

void AssimpModel::setIkNodeChain(int footId, int effektorNode, int targetNode) {
  /* root node as effector node... return */
  if (effektorNode == 0) {
    return;
  }

  std::vector<int> nodeList{};
  int currentNodeId = effektorNode;
  do {
    nodeList.emplace_back(currentNodeId);
    currentNodeId = mBoneParentIndexList.at(currentNodeId);
  } while (currentNodeId != targetNode && currentNodeId != -1);

  if (currentNodeId == -1) {
    Logger::log(1, "%s warning: root node hit, not adding target node\n", __FUNCTION__);
  } else {
    nodeList.emplace_back(targetNode);
  }

  mModelSettings.msFootIKChainNodes.at(footId) = nodeList;

  Logger::log(1, "%s: foot %i node chain (effector to target)\n", __FUNCTION__, footId);
  for (int node : nodeList) {
    Logger::log(1, "%s: -- node %i\n", __FUNCTION__, node);
  }
}

void AssimpModel::setAsNavigationTarget(bool value) {
  mModelSettings.msUseAsNavigationTarget = value;
}

bool AssimpModel::isNavigationTarget() {
  return mModelSettings.msUseAsNavigationTarget;
}
