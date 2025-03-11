#include <algorithm>
#include <filesystem>

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

  aiNode* rootNode = scene->mRootNode;
  std::string rootNodeName = rootNode->mName.C_Str();
  mRootNode = AssimpNode::createNode(rootNodeName);
  Logger::log(1, "%s: root node name: '%s'\n", __FUNCTION__, rootNodeName.c_str());

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

  for (const auto& node : mNodeList) {
    std::string nodeName = node->getNodeName();
    const auto boneIter = std::find_if(mBoneList.begin(), mBoneList.end(), [nodeName](std::shared_ptr<AssimpBone>& bone) { return bone->getBoneName() == nodeName; });
     if (boneIter != mBoneList.end()) {
      mBoneOffsetMatrices.insert({nodeName, mBoneList.at(std::distance(mBoneList.begin(), boneIter))->getOffsetMatrix()});
    }
  }

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

  /* animations */
  unsigned int numAnims = scene->mNumAnimations;
  for (unsigned int i = 0; i < numAnims; ++i) {
    aiAnimation* animation = scene->mAnimations[i];

    Logger::log(1, "%s: -- animation clip %i has %i skeletal channels, %i mesh channels, and %i morph mesh channels\n",
      __FUNCTION__, i, animation->mNumChannels, animation->mNumMeshChannels, animation->mNumMorphMeshChannels);

    std::shared_ptr<AssimpAnimClip> animClip = std::make_shared<AssimpAnimClip>();
    animClip->addChannels(animation);
    if (animClip->getClipName().empty()) {
      animClip->setClipName(std::to_string(i));
    }
    mAnimClips.emplace_back(animClip);
  }

  mModelFilenamePath = modelFilename;
  mModelFilename = std::filesystem::path(modelFilename).filename().generic_string();

  /* get root transformation matrix from model's root node */
  mRootTransformMatrix = Tools::convertAiToGLM(rootNode->mTransformation);

  Logger::log(1, "%s: - model has a total of %i texture%s\n", __FUNCTION__, mTextures.size(), mTextures.size() == 1 ? "" : "s");
  Logger::log(1, "%s: - model has a total of %i bone%s\n", __FUNCTION__, mBoneList.size(), mBoneList.size() == 1 ? "" : "s");
  Logger::log(1, "%s: - model has a total of %i animation%s\n", __FUNCTION__, numAnims, numAnims == 1 ? "" : "s");

  Logger::log(1, "%s: successfully loaded model '%s' (%s)\n", __FUNCTION__, modelFilename.c_str(), mModelFilename.c_str());
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

      mModelMeshes.emplace_back(mesh.getMesh());

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

void AssimpModel::draw(VkRenderData &renderData) {
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
      renderLayout = renderData.rdAssimpSkinningPipelineLayout;
    } else {
      renderLayout = renderData.rdAssimpPipelineLayout;
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

void AssimpModel::drawInstanced(VkRenderData &renderData, uint32_t instanceCount) {
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
      renderLayout = renderData.rdAssimpSkinningPipelineLayout;
    } else {
      renderLayout = renderData.rdAssimpPipelineLayout;
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
    vkCmdDrawIndexed(renderData.rdCommandBuffer, static_cast<uint32_t>(mesh.indices.size()), instanceCount, 0, 0, 0);
  }
}

unsigned int AssimpModel::getTriangleCount() {
  return mTriangleCount;
}

void AssimpModel::cleanup(VkRenderData &renderData) {
  for (auto buffer : mVertexBuffers) {
    VertexBuffer::cleanup(renderData, buffer);
  }
  for (auto buffer : mIndexBuffers) {
    IndexBuffer::cleanup(renderData, buffer);
  }

  for (auto& tex : mTextures) {
    Texture::cleanup(renderData, tex.second);
  }

  Texture::cleanup(renderData, mPlaceholderTexture);
  Texture::cleanup(renderData, mWhiteTexture);
}

std::string AssimpModel::getModelFileName() {
  return mModelFilename;
}

std::string AssimpModel::getModelFileNamePath() {
  return mModelFilenamePath;
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

const std::unordered_map<std::string, glm::mat4>& AssimpModel::getBoneOffsetMatrices() {
  return mBoneOffsetMatrices;
}

const std::vector<std::shared_ptr<AssimpAnimClip>>& AssimpModel::getAnimClips() {
  return mAnimClips;
}

bool AssimpModel::hasAnimations() {
  return !mAnimClips.empty();
}

const std::shared_ptr<AssimpNode> AssimpModel::getRootNode() {
  return mRootNode;
}
