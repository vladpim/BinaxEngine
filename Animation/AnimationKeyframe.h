#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>

struct AnimationKeyframe {
    float time = 0.0f;                     // время в секундах
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // единичный кватернион
    glm::vec3 scale = glm::vec3(1.0f);

    // Сериализация
    nlohmann::json ToJson() const;
    bool FromJson(const nlohmann::json& j);
};