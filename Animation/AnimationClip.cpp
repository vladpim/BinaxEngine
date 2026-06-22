#include "AnimationClip.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <glm/gtx/quaternion.hpp>

void AnimationClip::AddKeyframe(const AnimationKeyframe& key) {
    m_Keyframes.push_back(key);
    SortKeyframes();
    // Обновляем длительность
    if (!m_Keyframes.empty())
        m_Duration = m_Keyframes.back().time;
}

void AnimationClip::SortKeyframes() {
    std::sort(m_Keyframes.begin(), m_Keyframes.end(),
        [](const AnimationKeyframe& a, const AnimationKeyframe& b) { return a.time < b.time; });
}

void AnimationClip::Clear() {
    m_Keyframes.clear();
    m_Duration = 0.0f;
}

AnimationKeyframe AnimationClip::Interpolate(float time) const {
    if (m_Keyframes.empty()) return AnimationKeyframe();
    if (m_Keyframes.size() == 1) return m_Keyframes[0];
    if (time <= m_Keyframes.front().time) return m_Keyframes.front();
    if (time >= m_Keyframes.back().time) return m_Keyframes.back();

    for (size_t i = 0; i < m_Keyframes.size() - 1; ++i) {
        const auto& a = m_Keyframes[i];
        const auto& b = m_Keyframes[i + 1];
        if (time >= a.time && time <= b.time) {
            float t = (time - a.time) / (b.time - a.time);
            AnimationKeyframe result;
            result.time = time;
            result.position = glm::mix(a.position, b.position, t);
            result.rotation = glm::slerp(a.rotation, b.rotation, t);
            result.scale = glm::mix(a.scale, b.scale, t);
            return result;
        }
    }
    return m_Keyframes.back();
}

nlohmann::json AnimationClip::ToJson() const {
    nlohmann::json j;
    j["name"] = m_Name;
    j["duration"] = m_Duration;
    nlohmann::json keys = nlohmann::json::array();
    for (const auto& k : m_Keyframes) {
        keys.push_back(k.ToJson());
    }
    j["keyframes"] = keys;
    return j;
}

bool AnimationClip::FromJson(const nlohmann::json& j) {
    try {
        if (j.contains("name")) m_Name = j["name"];
        if (j.contains("duration")) m_Duration = j["duration"];
        if (j.contains("keyframes")) {
            m_Keyframes.clear();
            for (const auto& item : j["keyframes"]) {
                AnimationKeyframe k;
                if (k.FromJson(item)) {
                    m_Keyframes.push_back(k);
                }
            }
            SortKeyframes();
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool AnimationClip::SaveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << ToJson().dump(4);
    return true;
}

bool AnimationClip::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    nlohmann::json j;
    file >> j;
    return FromJson(j);
}