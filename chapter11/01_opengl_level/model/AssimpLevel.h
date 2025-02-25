#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <unordered_map>

#include <glm/glm.hpp>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "OGLRenderData.h"
#include "Texture.h"
#include "VertexIndexBuffer.h"
#include "AssimpNode.h"
#include "LevelSettings.h"
#include "AABB.h"

class AssimpLevel {
  public:
    bool loadLevel(std::string levelFilename, unsigned int extraImportFlags = 0);

    void draw();
    unsigned int getTriangleCount();

    void updateLevelRootMatrix();
    glm::mat4 getWorldTransformMatrix();

    std::string getLevelFileName();
    std::string getLevelFileNamePath();

    LevelSettings getLevelSettings();
    void setLevelSettings(LevelSettings settings);

    void generateAABB();
    AABB getAABB();

    void cleanup();

  private:
    void processNode(std::shared_ptr<AssimpNode> node, aiNode* aNode, const aiScene* scene, std::string assetDirectory);

    unsigned int mTriangleCount = 0;
    unsigned int mVertexCount = 0;

    glm::mat4 mLocalTranslationMatrix = glm::mat4(1.0f);
    glm::mat4 mLocalRotationMatrix = glm::mat4(1.0f);
    glm::mat4 mLocalScaleMatrix = glm::mat4(1.0f);
    glm::mat4 mLocalSwapAxisMatrix = glm::mat4(1.0f);

    glm::mat4 mLocalTransformMatrix = glm::mat4(1.0f);

    glm::mat4 mLevelRootMatrix = glm::mat4(1.0f);

    /* store the root node for direct access */
    std::shared_ptr<AssimpNode> mRootNode = nullptr;
    std::vector<std::shared_ptr<AssimpNode>> mNodeList{};

    glm::mat4 mRootTransformMatrix = glm::mat4(1.0f);
    LevelSettings mLevelSettings{};

    std::vector<OGLMesh> mLevelMeshes{};
    std::vector<VertexIndexBuffer> mVertexBuffers{};

    // map textures to external or internal texture names
    std::unordered_map<std::string, std::shared_ptr<Texture>> mTextures{};
    std::shared_ptr<Texture> mPlaceholderTexture = nullptr;

    AABB mLevelAABB{};
};
