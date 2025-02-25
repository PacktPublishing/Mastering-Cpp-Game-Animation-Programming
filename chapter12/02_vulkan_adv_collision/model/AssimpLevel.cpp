#include <algorithm>
#include <filesystem>

#include <glm/gtx/quaternion.hpp>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "AssimpLevel.h"
#include "Tools.h"
#include "Logger.h"

bool AssimpLevel::loadLevel(VkRenderData &renderData, std::string levelFilename, unsigned int extraImportFlags) {
  Logger::log(1, "%s: loading level from file '%s'\n", __FUNCTION__, levelFilename.c_str());

  Assimp::Importer importer;
  /* we need to flip texture coordinates for Vulkan */
  const aiScene *scene = importer.ReadFile(levelFilename,
    aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_ValidateDataStructure | aiProcess_FlipUVs | extraImportFlags);

  if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    Logger::log(1, "%s error: assimp error '%s' while loading file '%s'\n", __FUNCTION__, importer.GetErrorString(), levelFilename.c_str());
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

  /* add a placeholder texture in case there is no diffuse tex */
  std::string placeholderTexName = "textures/missing_tex.png";
  if (!Texture::loadTexture(renderData, mPlaceholderTexture, placeholderTexName)) {
    Logger::log(1, "%s error: could not load placeholder texture '%s'\n", __FUNCTION__, placeholderTexName.c_str());
    return false;
  }

  /* the textures are stored directly or relative to the model file */
  std::string assetDirectory = levelFilename.substr(0, levelFilename.find_last_of('/'));

  /* nodes */
  Logger::log(1, "%s: ... processing nodes...\n", __FUNCTION__);

  std::string rootNodeName = rootNode->mName.C_Str();
  mRootNode = AssimpNode::createNode(rootNodeName);
  Logger::log(2, "%s: root node name: '%s'\n", __FUNCTION__, rootNodeName.c_str());

  /* process all nodes in the level file */
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

  /* create vertex buffers for the meshes */
  for (const auto& mesh : mLevelMeshes) {
    VkVertexBufferData vertexBuffer;
    VertexBuffer::init(renderData, vertexBuffer, mesh.vertices.size() * sizeof(VkVertex));
    VertexBuffer::uploadData(renderData, vertexBuffer, mesh);
    mVertexBuffers.emplace_back(vertexBuffer);

    VkIndexBufferData indexBuffer;
    IndexBuffer::init(renderData, indexBuffer, mesh.indices.size() * sizeof(uint32_t));
    IndexBuffer::uploadData(renderData, indexBuffer, mesh);
    mIndexBuffers.emplace_back(indexBuffer);
  }

  mLevelSettings.lsLevelFilenamePath = levelFilename;
  mLevelSettings.lsLevelFilename = std::filesystem::path(levelFilename).filename().generic_string();

  /* get root transformation matrix from model's root node */
  mRootTransformMatrix = Tools::convertAiToGLM(rootNode->mTransformation);

  updateLevelRootMatrix();

  Logger::log(1, "%s: - level has a total of %i texture%s\n", __FUNCTION__, mTextures.size(), mTextures.size() == 1 ? "" : "s");

  Logger::log(1, "%s: successfully loaded level '%s' (%s)\n", __FUNCTION__, levelFilename.c_str(),
    mLevelSettings.lsLevelFilename.c_str());

  return true;
}

void AssimpLevel::processNode(VkRenderData &renderData, std::shared_ptr<AssimpNode> node, aiNode* aNode,
    const aiScene* scene, std::string assetDirectory) {
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

      mLevelMeshes.emplace_back(vertexMesh);
    }
  }

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

void AssimpLevel::draw(VkRenderData &renderData) {
  for (unsigned int i = 0; i < mLevelMeshes.size(); ++i) {
    VkMesh& mesh = mLevelMeshes.at(i);

    // find diffuse texture by name
    VkTextureData diffuseTex{};
    auto diffuseTexName = mesh.textures.find(aiTextureType_DIFFUSE);
    if (diffuseTexName != mesh.textures.end()) {
      auto diffuseTexture = mTextures.find(diffuseTexName->second);
      if (diffuseTexture != mTextures.end()) {
        diffuseTex = diffuseTexture->second;
      }
    }

    if (diffuseTex.image != VK_NULL_HANDLE) {
      vkCmdBindDescriptorSets(renderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        renderData.rdAssimpLevelPipelineLayout, 0, 1, &diffuseTex.descriptorSet, 0, nullptr);
    } else {
      vkCmdBindDescriptorSets(renderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        renderData.rdAssimpLevelPipelineLayout, 0, 1, &mPlaceholderTexture.descriptorSet, 0, nullptr);
    }

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(renderData.rdCommandBuffer, 0, 1, &mVertexBuffers.at(i).buffer, &offset);
    vkCmdBindIndexBuffer(renderData.rdCommandBuffer, mIndexBuffers.at(i).buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(renderData.rdCommandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
  }
}

void AssimpLevel::updateLevelRootMatrix() {
  mLocalScaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(mLevelSettings.lsScale));

  if (mLevelSettings.lsSwapYZAxis) {
    glm::mat4 flipMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    mLocalSwapAxisMatrix = glm::rotate(flipMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  } else {
    mLocalSwapAxisMatrix = glm::mat4(1.0f);
  }

  mLocalRotationMatrix = glm::mat4_cast(glm::quat(glm::radians(mLevelSettings.lsWorldRotation)));

  mLocalTranslationMatrix = glm::translate(glm::mat4(1.0f), mLevelSettings.lsWorldPosition);

  mLocalTransformMatrix = mLocalTranslationMatrix * mLocalRotationMatrix * mLocalSwapAxisMatrix * mLocalScaleMatrix;

  mLevelRootMatrix = mLocalTransformMatrix * mRootTransformMatrix;

  /* do NOT swap the axes for normals */
  mNormalTransformMatrix = glm::transpose(glm::inverse(mLevelRootMatrix));
}

unsigned int AssimpLevel::getTriangleCount() {
  return mTriangleCount;
}

void AssimpLevel::cleanup(VkRenderData &renderData) {
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
}

glm::mat4 AssimpLevel::getWorldTransformMatrix() {
  return mLevelRootMatrix;
}

glm::mat4 AssimpLevel::getNormalTransformMatrix() {
  return mNormalTransformMatrix;
}

LevelSettings AssimpLevel::getLevelSettings() {
  return mLevelSettings;
}

void AssimpLevel::setLevelSettings(LevelSettings settings) {
  mLevelSettings = settings;
  updateLevelRootMatrix();
}

std::string AssimpLevel::getLevelFileName() {
  return mLevelSettings.lsLevelFilename;
}

std::string AssimpLevel::getLevelFileNamePath() {
  return mLevelSettings.lsLevelFilenamePath;
}

void AssimpLevel::generateAABB() {
  updateLevelRootMatrix();
  mLevelAABB.clear();
  for (const auto& mesh : mLevelMeshes) {
    for (const auto& vertex : mesh.vertices) {
      /* we use position.w for UV coordinates, set to 1.0f */
      mLevelAABB.addPoint(mLevelRootMatrix * glm::vec4(glm::vec3(vertex.position), 1.0f));
    }
  }
}

AABB AssimpLevel::getAABB() {
  return mLevelAABB;
}

std::vector<VkMesh>& AssimpLevel::getLevelMeshes() {
  return mLevelMeshes;
}
