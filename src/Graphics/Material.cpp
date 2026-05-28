#include "Graphics/Material.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
// ===== Сериализация glm::vec3 и glm::vec2 для nlohmann/json =====
#include <glm/glm.hpp>

namespace nlohmann {
    template <>
    struct adl_serializer<glm::vec3> {
        static void to_json(json& j, const glm::vec3& v) {
            j = { v.x, v.y, v.z };
        }
        static void from_json(const json& j, glm::vec3& v) {
            if (j.is_array() && j.size() >= 3) {
                v.x = j[0].get<float>();
                v.y = j[1].get<float>();
                v.z = j[2].get<float>();
            } else {
                v = glm::vec3(0.0f);
            }
        }
    };
    template <>
    struct adl_serializer<glm::vec2> {
        static void to_json(json& j, const glm::vec2& v) {
            j = { v.x, v.y };
        }
        static void from_json(const json& j, glm::vec2& v) {
            if (j.is_array() && j.size() >= 2) {
                v.x = j[0].get<float>();
                v.y = j[1].get<float>();
            } else {
                v = glm::vec2(1.0f);
            }
        }
    };
}

using json = nlohmann::json;

Material::Material() {}

Material::~Material() {
    if (m_DiffuseTexture) glDeleteTextures(1, &m_DiffuseTexture);
    if (m_NormalTexture) glDeleteTextures(1, &m_NormalTexture);
    if (m_RoughnessTexture) glDeleteTextures(1, &m_RoughnessTexture);
    if (m_MetallicTexture) glDeleteTextures(1, &m_MetallicTexture);
    if (m_AOTexture) glDeleteTextures(1, &m_AOTexture);
}

// Модифицируйте методы загрузки текстур, чтобы сохранять пути
bool Material::LoadDiffuseTexture(const std::string& path) {
    if (m_DiffuseTexture) glDeleteTextures(1, &m_DiffuseTexture);
    m_DiffuseTexture = LoadTexture(path);
    if (m_DiffuseTexture) {
        m_DiffusePath = path;
        return true;
    }
    return false;
}

bool Material::LoadNormalTexture(const std::string& path) {
    if (m_NormalTexture) glDeleteTextures(1, &m_NormalTexture);
    m_NormalTexture = LoadTexture(path);
    if (m_NormalTexture) {
        m_NormalPath = path;
        return true;
    }
    return false;
}

bool Material::LoadRoughnessTexture(const std::string& path) {
    if (m_RoughnessTexture) glDeleteTextures(1, &m_RoughnessTexture);
    m_RoughnessTexture = LoadTexture(path);
    if (m_RoughnessTexture) {
        m_RoughnessPath = path;
        return true;
    }
    return false;
}

bool Material::LoadMetallicTexture(const std::string& path) {
    if (m_MetallicTexture) glDeleteTextures(1, &m_MetallicTexture);
    m_MetallicTexture = LoadTexture(path);
    if (m_MetallicTexture) {
        m_MetallicPath = path;
        return true;
    }
    return false;
}

bool Material::LoadAOTexture(const std::string& path) {
    if (m_AOTexture) glDeleteTextures(1, &m_AOTexture);
    m_AOTexture = LoadTexture(path);
    if (m_AOTexture) {
        m_AOPath = path;
        return true;
    }
    return false;
}

// Сериализация в JSON
json Material::ToJson() const {
    json j;
    j["albedo"] = { albedo.x, albedo.y, albedo.z };
    j["metallic"] = metallic;
    j["roughness"] = roughness;
    j["ao"] = ao;
    j["uv_scale"] = { uvScale.x, uvScale.y };
    j["normal_strength"] = normalStrength;
    j["use_world_uv"] = useWorldUV;
    j["emission_color"] = { emissionColor.x, emissionColor.y, emissionColor.z };
    j["emission_intensity"] = emissionIntensity;
    j["enable_reflections"] = enableReflections;

    if (!m_DiffusePath.empty()) j["diffuse_path"] = m_DiffusePath;
    if (!m_NormalPath.empty()) j["normal_path"] = m_NormalPath;
    if (!m_RoughnessPath.empty()) j["roughness_path"] = m_RoughnessPath;
    if (!m_MetallicPath.empty()) j["metallic_path"] = m_MetallicPath;
    if (!m_AOPath.empty()) j["ao_path"] = m_AOPath;

    return j;
}

bool Material::FromJson(const json& j) {
    try {
        if (j.contains("albedo")) albedo = j["albedo"].get<glm::vec3>();
        if (j.contains("metallic")) metallic = j["metallic"];
        if (j.contains("roughness")) roughness = j["roughness"];
        if (j.contains("ao")) ao = j["ao"];
        if (j.contains("uv_scale")) uvScale = j["uv_scale"].get<glm::vec2>();
        if (j.contains("normal_strength")) normalStrength = j["normal_strength"];
        if (j.contains("use_world_uv")) useWorldUV = j["use_world_uv"];
        if (j.contains("emission_color")) emissionColor = j["emission_color"].get<glm::vec3>();
        if (j.contains("emission_intensity")) emissionIntensity = j["emission_intensity"];
        if (j.contains("enable_reflections")) enableReflections = j["enable_reflections"];

        if (j.contains("diffuse_path")) LoadDiffuseTexture(j["diffuse_path"]);
        if (j.contains("normal_path")) LoadNormalTexture(j["normal_path"]);
        if (j.contains("roughness_path")) LoadRoughnessTexture(j["roughness_path"]);
        if (j.contains("metallic_path")) LoadMetallicTexture(j["metallic_path"]);
        if (j.contains("ao_path")) LoadAOTexture(j["ao_path"]);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Material::FromJson error: " << e.what() << std::endl;
        return false;
    }
}

bool Material::SaveToFile(const std::string& path) {
    json j = ToJson();
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << j.dump(4);
    return true;
}

bool Material::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    json j;
    file >> j;
    return FromJson(j);
}

void Material::BindTextures() const {
    if (m_DiffuseTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_DiffuseTexture);
    }
    if (m_NormalTexture) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_NormalTexture);
    }
    if (m_RoughnessTexture) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, m_RoughnessTexture);
    }
    if (m_MetallicTexture) {
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, m_MetallicTexture);
    }
    if (m_AOTexture) {
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, m_AOTexture);
    }
}

void Material::UnbindTextures() const {
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint Material::LoadTexture(const std::string& path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    if (textureID == 0) {
        std::cerr << "[Material] Failed to generate texture ID" << std::endl;
        return 0;
    }

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
        return textureID;
    } else {
        std::cerr << "[Material] Failed to load texture: " << path << " - " << stbi_failure_reason() << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }
}
