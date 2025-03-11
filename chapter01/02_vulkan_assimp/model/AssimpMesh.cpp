#include "AssimpMesh.h"

#include "Logger.h"
#include "Tools.h"

bool AssimpMesh::processMesh(VkRenderData &renderData, aiMesh* mesh, const aiScene* scene, std::string assetDirectory,
    std::unordered_map<std::string, VkTextureData> &textures) {
  mMeshName = mesh->mName.C_Str();

  mTriangleCount = mesh->mNumFaces;
  mVertexCount = mesh->mNumVertices;

  Logger::log(1, "%s: -- mesh '%s' has %i faces (%i vertices)\n", __FUNCTION__, mMeshName.c_str(), mTriangleCount, mVertexCount);
  for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i) {
    if (mesh->HasVertexColors(i)) {
      Logger::log(1, "%s: --- mesh has vertex colors in set %i\n", __FUNCTION__, i);
    }
  }
  if (mesh->HasNormals()) {
    Logger::log(1, "%s: --- mesh has normals\n", __FUNCTION__);
  }
  for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
    if (mesh->HasTextureCoords(i)) {
      Logger::log(1, "%s: --- mesh has texture cooords in set %i\n", __FUNCTION__, i);
    }
  }

  aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
  if (material) {
    aiString materialName = material->GetName();
    Logger::log(1, "%s: - material found, name '%s'\n", __FUNCTION__, materialName.C_Str());

    if (mesh->mMaterialIndex >= 0) {
      // scan only for diifuse and scalar textures for a start
      std::vector<aiTextureType> supportedTexTypes = { aiTextureType_DIFFUSE, aiTextureType_SPECULAR };
      for (const auto& texType : supportedTexTypes) {
        unsigned int textureCount = material->GetTextureCount(texType);
        if (textureCount > 0) {
          Logger::log(1, "%s: -- material '%s' has %i images of type %i\n", __FUNCTION__, materialName.C_Str(), textureCount, texType);
          for (unsigned int i = 0; i < textureCount; ++i) {
            aiString textureName;
            material->GetTexture(texType, i, &textureName);
            Logger::log(1, "%s: --- image %i has name '%s'\n", __FUNCTION__, i, textureName.C_Str());

            std::string texName = textureName.C_Str();
            mMesh.textures.insert({texType, texName});

            /* skip already loaded textures */
            if (textures.count(texName) > 0) {
              Logger::log(1, "%s: texture '%s' already loaded, skipping\n", __FUNCTION__, texName.c_str());
              continue;
            }

            // do not try to load internal textures
            if (!texName.empty() && texName.find("*") != 0) {
              VkTextureData newTex{};
              std::string texNameWithPath = assetDirectory + '/' + texName;
              if (!Texture::loadTexture(renderData, newTex, texNameWithPath)) {
                Logger::log(1, "%s error: could not load texture file '%s', skipping\n", __FUNCTION__, texNameWithPath.c_str());
                Texture::cleanup(renderData, newTex);
                continue;
              }

              textures.insert({texName, newTex});
            }
          }
        }
      }
    }

    aiColor4D baseColor(0.0f, 0.0f, 0.0f, 1.0f);
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == aiReturn_SUCCESS && textures.empty()) {
      mBaseColor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
      mMesh.usesPBRColors = true;
    }
  }

  for (unsigned int i = 0; i < mVertexCount; ++i) {
    VkVertex vertex;
    vertex.position.x = mesh->mVertices[i].x;
    vertex.position.y = mesh->mVertices[i].y;
    vertex.position.z = mesh->mVertices[i].z;

    if (mesh->HasVertexColors(0)) {
      vertex.color.r = mesh->mColors[0][i].r;
      vertex.color.g = mesh->mColors[0][i].g;
      vertex.color.b = mesh->mColors[0][i].b;
      vertex.color.a = mesh->mColors[0][i].a;
    } else {
      if (mMesh.usesPBRColors) {
        vertex.color = mBaseColor;
      } else {
        vertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
      }
    }

    if (mesh->HasNormals()) {
      vertex.normal.x = mesh->mNormals[i].x;
      vertex.normal.y = mesh->mNormals[i].y;
      vertex.normal.z = mesh->mNormals[i].z;
    } else {
      vertex.normal = glm::vec3(0.0f);
    }

    if (mesh->HasTextureCoords(0)) {
      vertex.uv.x = mesh->mTextureCoords[0][i].x;
      vertex.uv.y = mesh->mTextureCoords[0][i].y;
    } else {
      vertex.uv = glm::vec2(0.0f);
    }

    mMesh.vertices.emplace_back(vertex);
  }

  for (unsigned int i = 0; i < mTriangleCount; ++i) {
    aiFace face = mesh->mFaces[i];
    mMesh.indices.push_back(face.mIndices[0]);
    mMesh.indices.push_back(face.mIndices[1]);
    mMesh.indices.push_back(face.mIndices[2]);
  }

  if (mesh->HasBones()) {
    unsigned int numBones = mesh->mNumBones;
    Logger::log(1, "%s: -- mesh has information about %i bones\n", __FUNCTION__, numBones);
    for (unsigned int boneId = 0; boneId < numBones; ++boneId) {
      std::string boneName = mesh->mBones[boneId]->mName.C_Str();
      unsigned int numWeights = mesh->mBones[boneId]->mNumWeights;
      Logger::log(1, "%s: --- bone nr. %i has name %s, contains %i weights\n", __FUNCTION__, boneId, boneName.c_str(), numWeights);

      std::shared_ptr<AssimpBone> newBone = std::make_shared<AssimpBone>(boneId, boneName, Tools::convertAiToGLM(mesh->mBones[boneId]->mOffsetMatrix));
      mBoneList.push_back(newBone);

      for (unsigned int weight = 0; weight < numWeights; ++weight) {
        unsigned int vertexId = mesh->mBones[boneId]->mWeights[weight].mVertexId;
        float vertexWeight = mesh->mBones[boneId]->mWeights[weight].mWeight;

        glm::uvec4 currentIds = mMesh.vertices.at(vertexId).boneNumber;
        glm::vec4 currentWeights = mMesh.vertices.at(vertexId).boneWeight;

        /* insert weight and bone id into first free slot (weight => 0.0f) */
        for (unsigned int i = 0; i < 4; ++i) {
          if (currentWeights[i] == 0.0f) {
            currentIds[i] = boneId;
            currentWeights[i] = vertexWeight;

            /* skip to next weight */
            break;
          }
        }
        mMesh.vertices.at(vertexId).boneNumber = currentIds;
        mMesh.vertices.at(vertexId).boneWeight = currentWeights;

      }
    }
  }

  return true;
}

std::vector<uint32_t> AssimpMesh::getIndices() {
  return mMesh.indices;
}

VkMesh AssimpMesh::getMesh() {
  return mMesh;
}

std::string AssimpMesh::getMeshName() {
  return mMeshName;
}

unsigned int AssimpMesh::getTriangleCount() {
  return mTriangleCount;
}

unsigned int AssimpMesh::getVertexCount() {
  return mVertexCount;
}

std::vector<std::shared_ptr<AssimpBone>> AssimpMesh::getBoneList() {
  return mBoneList;
}
