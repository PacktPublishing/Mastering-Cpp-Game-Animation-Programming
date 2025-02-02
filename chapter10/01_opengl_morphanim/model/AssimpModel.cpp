#include <algorithm>
#include <filesystem>

#include <glm/gtx/quaternion.hpp>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "AssimpModel.h"
#include "Tools.h"
#include "Logger.h"

bool AssimpModel::loadModel(std::string modelFilename, unsigned int extraImportFlags) {
  Logger::log(1, "%s: loading model from file '%s'\n", __FUNCTION__, modelFilename.c_str());

  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(modelFilename, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_ValidateDataStructure | extraImportFlags);

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

      std::shared_ptr<Texture> newTex = std::make_shared<Texture>();
      if (!newTex->loadTexture(texName, data, width, height)) {
        return false;
      }

      std::string internalTexName = "*" + std::to_string(i);
      Logger::log(1, "%s: - added internal texture '%s'\n", __FUNCTION__, internalTexName.c_str());
      mTextures.insert({internalTexName, newTex});
    }

    Logger::log(1, "%s: scene has %i embedded textures\n", __FUNCTION__, numTextures);
  }

  /* add a placeholder texture in case there is no diffuse tex */
  mPlaceholderTexture = std::make_shared<Texture>();
  std::string placeholderTexName = "textures/missing_tex.png";
  if (!mPlaceholderTexture->loadTexture(placeholderTexName)) {
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

  processNode(mRootNode, rootNode, scene, assetDirectory);

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

  std::vector<glm::mat4> boneOffsetMatricesList{};

  for (const auto& bone : mBoneList) {
    boneOffsetMatricesList.emplace_back(bone->getOffsetMatrix());

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
    VertexIndexBuffer buffer;
    buffer.init();
    buffer.uploadData(mesh.vertices, mesh.indices);
    mVertexBuffers.emplace_back(buffer);
  }

  /* create a SSBOs containing all vertices for all morph animation of this mesh */
  for (const auto& mesh : mModelMeshes) {
    if (mesh.morphMeshes.size() == 0) {
      continue;
    }
    OGLMorphMesh animMesh;
    animMesh.morphVertices.resize(mesh.vertices.size() * mNumAnimatedMeshes);

    for (unsigned int i = 0; i < mNumAnimatedMeshes; ++i) {
      unsigned int vertexOffset = mesh.vertices.size() * i;
      std::copy(mesh.morphMeshes[i].morphVertices.begin(), mesh.morphMeshes[i].morphVertices.end(),
        animMesh.morphVertices.begin() + vertexOffset);
      mAnimatedMeshVertexSize = mesh.vertices.size();
    }

    mAnimMeshVerticesBuffer.uploadSsboData(animMesh.morphVertices);
    Logger::log(1, "%s: model has %i morphs, SSBO has %i vertices\n", __FUNCTION__, mNumAnimatedMeshes, mAnimatedMeshVertexSize);
  }

  mShaderBoneMatrixOffsetBuffer.uploadSsboData(boneOffsetMatricesList);
  mShaderBoneParentBuffer.uploadSsboData(mBoneParentIndexList);

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

  if (mAnimClips.size() > 0) {
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
    mAnimLookupBuffer.uploadSsboData(animLookupData);
  }

  mModelSettings.msModelFilenamePath = modelFilename;
  mModelSettings.msModelFilename = std::filesystem::path(modelFilename).filename().generic_string();

  /* get root transformation matrix from model's root node */
  mRootTransformMatrix = Tools::convertAiToGLM(rootNode->mTransformation);

  if (mBoneList.size() > 0) {
    for (const auto& bone: mBoneList) {
      mBoneNameList.emplace_back(bone->getBoneName());
    }
    mModelSettings.msBoundingSphereAdjustments = std::vector(mBoneList.size(), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
  }

  Logger::log(1, "%s: - model has a total of %i texture%s\n", __FUNCTION__, mTextures.size(), mTextures.size() == 1 ? "" : "s");
  Logger::log(1, "%s: - model has a total of %i bone%s\n", __FUNCTION__, mBoneList.size(), mBoneList.size() == 1 ? "" : "s");
  Logger::log(1, "%s: - model has a total of %i skeletal animation%s\n", __FUNCTION__, numAnims, numAnims == 1 ? "" : "s");
  Logger::log(1, "%s: - model has a total of %i morph animation%s\n", __FUNCTION__, mNumAnimatedMeshes, mNumAnimatedMeshes == 1 ? "" : "s");

  Logger::log(1, "%s: successfully loaded model '%s' (%s)\n", __FUNCTION__, modelFilename.c_str(), mModelSettings.msModelFilename.c_str());
  return true;
}

void AssimpModel::processNode(std::shared_ptr<AssimpNode> node, aiNode* aNode, const aiScene* scene, std::string assetDirectory) {
  std::string nodeName = aNode->mName.C_Str();
  Logger::log(1, "%s: node name: '%s'\n", __FUNCTION__, nodeName.c_str());

  unsigned int numMeshes = aNode->mNumMeshes;
  if (numMeshes > 0) {
    Logger::log(1, "%s: - node has %i meshes\n", __FUNCTION__, numMeshes);
    for (unsigned int i = 0; i < numMeshes; ++i) {
      aiMesh* modelMesh = scene->mMeshes[aNode->mMeshes[i]];

      AssimpMesh mesh;
      mesh.processMesh(modelMesh, scene, assetDirectory, mTextures);
      OGLMesh vertexMesh = mesh.getMesh();
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
    processNode(childNode, aNode->mChildren[i], scene, assetDirectory);
  }
}

glm::mat4 AssimpModel::getRootTranformationMatrix() {
  return mRootTransformMatrix;
}

void AssimpModel::draw() {
  for (unsigned int i = 0; i < mModelMeshes.size(); ++i) {
    OGLMesh& mesh = mModelMeshes.at(i);
    // find diffuse texture by name

    std::shared_ptr<Texture> diffuseTex = nullptr;
    auto diffuseTexName = mesh.textures.find(aiTextureType_DIFFUSE);
    if (diffuseTexName != mesh.textures.end()) {
      auto diffuseTexture = mTextures.find(diffuseTexName->second);
      if (diffuseTexture != mTextures.end()) {
        diffuseTex = diffuseTexture->second;
      }
    }

    glActiveTexture(GL_TEXTURE0);
    if (diffuseTex) {
      diffuseTex->bind();
    } else {
      mPlaceholderTexture->bind();
    }

    mVertexBuffers.at(i).bindAndDrawIndirect(GL_TRIANGLES, mesh.indices.size());

    if (diffuseTex) {
      diffuseTex->unbind();
    } else {
      mPlaceholderTexture->unbind();
    }
  }
}

void AssimpModel::drawInstanced(int instanceCount) {
  for (unsigned int i = 0; i < mModelMeshes.size(); ++i) {
    OGLMesh& mesh = mModelMeshes.at(i);
    drawInstanced(mesh, i, instanceCount);
  }
}

void AssimpModel::drawInstancedNoMorphAnims(int instanceCount) {
  for (unsigned int i = 0; i < mModelMeshes.size(); ++i) {
    /* skip meshes with morph animations */
    if (mModelMeshes.at(i).morphMeshes.size() > 0) {
      continue;
    }
    OGLMesh& mesh = mModelMeshes.at(i);
    drawInstanced(mesh, i, instanceCount);
  }
}

void AssimpModel::drawInstancedMorphAnims(int instanceCount) {
  for (unsigned int i = 0; i < mModelMeshes.size(); ++i) {
    /* draw only meshes with morph animations */
    if (mModelMeshes.at(i).morphMeshes.size() == 0) {
      continue;
    }
    OGLMesh& mesh = mModelMeshes.at(i);
    drawInstanced(mesh, i, instanceCount);
  }
}

void AssimpModel::drawInstanced(OGLMesh& mesh, unsigned int meshIndex, int instanceCount) {
  // find diffuse texture by name
  std::shared_ptr<Texture> diffuseTex = nullptr;
  auto diffuseTexName = mesh.textures.find(aiTextureType_DIFFUSE);
  if (diffuseTexName != mesh.textures.end()) {
    auto diffuseTexture = mTextures.find(diffuseTexName->second);
    if (diffuseTexture != mTextures.end()) {
      diffuseTex = diffuseTexture->second;
    }
  }

  glActiveTexture(GL_TEXTURE0);
  if (diffuseTex) {
    diffuseTex->bind();
  } else {
    mPlaceholderTexture->bind();
  }

  mVertexBuffers.at(meshIndex).bindAndDrawIndirectInstanced(GL_TRIANGLES, mesh.indices.size(), instanceCount);

  if (diffuseTex) {
    diffuseTex->unbind();
  } else {
    mPlaceholderTexture->unbind();
  }
}

unsigned int AssimpModel::getTriangleCount() {
  return mTriangleCount;
}

void AssimpModel::cleanup() {
  for (auto buffer : mVertexBuffers) {
    buffer.cleanup();
  }

  for (auto tex : mTextures) {
    tex.second->cleanup();
  }
  mPlaceholderTexture->cleanup();
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

const std::map<std::string, std::shared_ptr<AssimpNode>>& AssimpModel::getNodeMap() {
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
  return mAnimClips.size() > 0;
}

void AssimpModel::bindBoneMatrixOffsetBuffer(int bindingPoint) {
  mShaderBoneMatrixOffsetBuffer.bind(bindingPoint);
}

void AssimpModel::bindBoneParentBuffer(int bindingPoint) {
  mShaderBoneParentBuffer.bind(bindingPoint);
}

void AssimpModel::bindAnimLookupBuffer(int bindingPoint) {
  mAnimLookupBuffer.bind(bindingPoint);
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

void AssimpModel::bindMorphAnimBuffer(int bindingPoint) {
  mAnimMeshVerticesBuffer.bind(bindingPoint);
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
