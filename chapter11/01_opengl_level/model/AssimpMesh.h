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
    bool processMesh(aiMesh* mesh, const aiScene* scene, std::string assetDirectory,
      std::unordered_map<std::string, std::shared_ptr<Texture>>& textures);

    std::string getMeshName();
    unsigned int getTriangleCount();
    unsigned int getVertexCount();

    OGLMesh getMesh();
    std::vector<uint32_t> getIndices();
    std::vector<std::shared_ptr<AssimpBone>> getBoneList();

  private:
    std::string mMeshName;
    unsigned int mTriangleCount = 0;
    unsigned int mVertexCount = 0;

    OGLMesh mMesh{};
    std::vector<std::shared_ptr<AssimpBone>> mBoneList{};
};
