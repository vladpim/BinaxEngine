#pragma once
#include <vector>
#include <string>
#include "AnimationKeyframe.h"
#include <nlohmann/json.hpp>

class AnimationClip {
public:
    AnimationClip() = default;
    explicit AnimationClip(const std::string& name) : m_Name(name) {}

    void AddKeyframe(const AnimationKeyframe& key);
    void SortKeyframes(); // по времени
    void Clear();

    // Интерполяция между двумя ключами (линейная)
    AnimationKeyframe Interpolate(float time) const;

    float GetDuration() const { return m_Duration; }
    const std::string& GetName() const { return m_Name; }
    const std::vector<AnimationKeyframe>& GetKeyframes() const { return m_Keyframes; }

    // Сохранение/загрузка
    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);

    // Сериализация JSON
    nlohmann::json ToJson() const;
    bool FromJson(const nlohmann::json& j);

private:
    std::string m_Name = "Untitled";
    std::vector<AnimationKeyframe> m_Keyframes;
    float m_Duration = 0.0f;
};