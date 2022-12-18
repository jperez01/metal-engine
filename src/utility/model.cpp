//
//  Model.cpp
//  metal_engine
//
//  Created by Juan Perez on 11/26/22.
//

#include "model.hpp"
#include "importUtils.hpp"

Model::Model() = default;

Model::Model(std::string path, MTL::Device* device) {
    m_device = device;
    std::cout << "Starting model loading" << std::endl;
    loadModel(path);
    std::cout << "Ending model loading" << std::endl;
}

void Model::loadModel(std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "Error::Assimp::" << importer.GetErrorString() << std::endl;
    }
    m_directory = path.substr(0, path.find_last_of('/'));
    
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.push_back(processMesh(mesh, scene));
    }
    
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    TypeEndpoints endpoints = {
        -1, -1, -1, -1
    };
    
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        simd::float3 vector = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };
        vertex.position = vector;
        
        if (mesh->HasNormals()) {
            vector = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            };
            vertex.normal = vector;
        }
        
        if (mesh->mTextureCoords[0]) {
            simd::float2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.texCoords = vec;
        } else {
            vertex.texCoords = (simd::float2) {
                0, 0
            };
        }
        
        vertices.push_back(vertex);
    }
    
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }
    
    int textureAmount = 0;
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        std::string materialType = "diffuse";
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, materialType);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        if (diffuseMaps.size() > 0) {
            endpoints.diffuse = textureAmount;
            textureAmount += diffuseMaps.size();
        }
        
        materialType = "specular";
        std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, materialType);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        if (specularMaps.size() > 0) {
            endpoints.specular = textureAmount;
            textureAmount += specularMaps.size();
        }
    }
    
    return Mesh(vertices, indices, textures, endpoints);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* material, aiTextureType type, std::string& typeName) {
    std::vector<Texture> textures;
    
    unsigned int count = material->GetTextureCount(type);
    
    for (unsigned int i = 0; i < count; i++) {
        aiString str;
        material->GetTexture(type, i, &str);
        bool skip = false;
        
        for (unsigned int j = 0; j < m_textures_loaded.size(); j++) {
            if (std::strcmp(m_textures_loaded[j].path.c_str(), str.C_Str()) == 0) {
                textures.push_back(m_textures_loaded[j]);
                skip = true;
                break;
            }
        }
        
        if (!skip) {
            Texture texture;
            std::string path = std::string(str.C_Str());
            texture.actualTexture = importUtils::textureFromFile(path, this->m_directory, m_device);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            m_textures_loaded.push_back(texture);
        }
    }
    
    return textures;
}

void Model::setupMeshBuffers(MTL::Device* device, MTL::Function* function) {
    for (Mesh& mesh: m_meshes) {
        mesh.setupMesh(device, function);
    }
}

void Model::draw(MTL::RenderCommandEncoder* encoder) {
    for (Texture texture : m_textures_loaded) {
        encoder->useResource(texture.actualTexture, MTL::ResourceUsageSample, MTL::RenderStageFragment);
    }
    for (Mesh& mesh : m_meshes) {
        mesh.draw(encoder);
    }
}
