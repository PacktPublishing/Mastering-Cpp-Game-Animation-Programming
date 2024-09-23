#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <assimp/scene.h>

#include "OGLRenderData.h"
#include "Texture.h"
#include "AssimpBone.h"

class AssimpMesh {
  public:
    bool processMesh(aiMesh* mesh, const aiScene* scene, std::string assetDirectory);

    std::string getMeshName();
    unsigned int getTriangleCount();
    unsigned int getVertexCount();

    OGLMesh getMesh();
    std::vector<uint32_t> getIndices();
    std::unordered_map<std::string, std::shared_ptr<Texture>> getTextures();
    std::vector<std::shared_ptr<AssimpBone>> getBoneList();

  private:
    std::string mMeshName;
    unsigned int mTriangleCount = 0;
    unsigned int mVertexCount = 0;

    unsigned int mAnimMeshCount = 0;

    OGLMesh mMesh{};

    std::unordered_map<std::string, std::shared_ptr<Texture>> mTextures;
    std::vector<std::shared_ptr<AssimpBone>> mBoneList;
};
