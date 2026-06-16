#pragma once
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

class Material {
public:
    Material();
    ~Material();

    bool LoadDiffuseTexture(const std::string& path);
    bool LoadNormalTexture(const std::string& path);
    bool LoadRoughnessTexture(const std::string& path);
    bool LoadMetallicTexture(const std::string& path);
    bool LoadAOTexture(const std::string& path);

    void ClearDiffuse();
    void ClearNormal();
    void ClearRoughness();
    void ClearMetallic();
    void ClearAO();

    void BindTextures() const;
    void UnbindTextures() const;

    bool HasDiffuse() const { return m_DiffuseTexture != 0; }
    bool HasNormal() const  { return m_NormalTexture != 0; }
    bool HasRoughness() const { return m_RoughnessTexture != 0; }
    bool HasMetallic() const { return m_MetallicTexture != 0; }
    bool HasAO() const { return m_AOTexture != 0; }

    GLuint GetDiffuseTextureID() const { return m_DiffuseTexture; }

    // Параметры PBR
    glm::vec3 albedo = glm::vec3(1.0f);
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    glm::vec2 uvScale = glm::vec2(1.0f);
    float normalStrength = 1.0f;
    bool useWorldUV = false;
    glm::vec3 emissionColor = glm::vec3(0.0f);
    float emissionIntensity = 0.0f;
    bool enableReflections = false;

    // Прозрачность
    bool transparent = false;
    float alpha = 1.0f;
    bool alphaTest = false;        // true = discard (листва), false = blending (стекло)
    float alphaCutoff = 0.5f;      // порог отбрасывания

    // Тени от листвы
    bool alphaTestShadows = true;   // использовать альфа-тест при рендере карты теней

    void SetTransparent(bool enable, float a = 1.0f, bool test = false, float cutoff = 0.5f) {
        transparent = enable;
        alpha = glm::clamp(a, 0.0f, 1.0f);
        alphaTest = test;
        alphaCutoff = cutoff;
    }

    bool IsTransparent() const { return transparent && alpha < 1.0f; }

    // Сериализация
    nlohmann::json ToJson() const;
    bool FromJson(const nlohmann::json& j);
    bool SaveToFile(const std::string& path);
    bool LoadFromFile(const std::string& path);
    float aoStrength = 1.0f;          // сила AO (0 – отключена, 1 – полная)
    float roughnessStrength = 1.0f;   // сила roughness-текстуры (0 – используется только базовое значение, 1 – только текстура)

private:
    GLuint m_DiffuseTexture = 0;
    GLuint m_NormalTexture = 0;
    GLuint m_RoughnessTexture = 0;
    GLuint m_MetallicTexture = 0;
    GLuint m_AOTexture = 0;

    std::string m_DiffusePath;
    std::string m_NormalPath;
    std::string m_RoughnessPath;
    std::string m_MetallicPath;
    std::string m_AOPath;

    GLuint LoadTexture(const std::string& path);
};
