#pragma once
#include <string>
#include <vector>
#include <memory>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Material.h"

class Model {
public:
    Model(const std::string& path);
    ~Model() = default;

    const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return m_Meshes; }
    bool IsLoaded() const { return !m_Meshes.empty(); }

private:
    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
    std::shared_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene);
    void processMaterial(aiMaterial* aiMat, const std::string& directory, std::shared_ptr<Material> material);

    std::vector<std::shared_ptr<Mesh>> m_Meshes;
    std::string m_Directory;
};
