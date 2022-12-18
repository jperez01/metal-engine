#pragma once

#include <iostream>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"

class Model {
public:
    Model();
    Model(std::string path, MTL::Device* device);
    void draw(MTL::RenderCommandEncoder* encoder);
    void setupMeshBuffers(MTL::Device* device, MTL::Function* function);
    
private:
    std::vector<Texture> m_textures_loaded;
    std::vector<Mesh> m_meshes;
    std::string m_directory;
    MTL::Device* m_device;
    
    void loadModel(std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* material, aiTextureType type, std::string& typeName);
};
