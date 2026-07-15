#include "Animation/Animation.h"
#include "Scene/GameObject.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

// ============================================================================
// AnimationKeyframe
// ============================================================================
nlohmann::json AnimationKeyframe::ToJson() const {
    nlohmann::json j;
    j["time"] = time;
    j["position"] = { position.x, position.y, position.z };
    j["rotation"] = { rotation.x, rotation.y, rotation.z, rotation.w };
    j["scale"] = { scale.x, scale.y, scale.z };
    return j;
}

bool AnimationKeyframe::FromJson(const nlohmann::json& j) {
    try {
        if (j.contains("time")) time = j["time"];
        if (j.contains("position")) {
            auto p = j["position"];
            position = glm::vec3(p[0].get<float>(), p[1].get<float>(), p[2].get<float>());
        }
        if (j.contains("rotation")) {
            auto r = j["rotation"];
            rotation = glm::quat(r[3].get<float>(), r[0].get<float>(), r[1].get<float>(), r[2].get<float>());
        }
        if (j.contains("scale")) {
            auto s = j["scale"];
            scale = glm::vec3(s[0].get<float>(), s[1].get<float>(), s[2].get<float>());
        }
        return true;
    } catch (...) {
        return false;
    }
}

// ============================================================================
// AnimationClip
// ============================================================================
void AnimationClip::AddKeyframe(const AnimationKeyframe& key) {
    m_Keyframes.push_back(key);
    SortKeyframes();
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

// ============================================================================
// AnimationComponent
// ============================================================================
void AnimationComponent::SetClip(std::shared_ptr<AnimationClip> clip) {
    m_Clip = clip;
    m_CurrentTime = 0.0f;
}

void AnimationComponent::Play() {
    if (!m_Clip || m_Clip->GetKeyframes().empty()) return;
    m_Playing = true;
    m_Paused = false;
    if (m_CurrentTime >= m_Clip->GetDuration()) {
        m_CurrentTime = 0.0f;
    }
}

void AnimationComponent::Stop() {
    m_Playing = false;
    m_Paused = false;
    m_CurrentTime = 0.0f;
}

void AnimationComponent::Pause() {
    if (m_Playing) m_Paused = !m_Paused;
}

void AnimationComponent::SetTime(float time) {
    if (m_Clip) {
        m_CurrentTime = glm::clamp(time, 0.0f, m_Clip->GetDuration());
    }
}

void AnimationComponent::Update(float deltaTime, GameObject* owner) {
    if (!m_Playing || m_Paused || !m_Clip || !owner) return;
    if (m_Clip->GetKeyframes().empty()) return;

    m_CurrentTime += deltaTime * m_Speed;
    float duration = m_Clip->GetDuration();
    if (duration <= 0.0f) return;

    if (m_CurrentTime >= duration) {
        if (m_Looping) {
            m_CurrentTime = fmod(m_CurrentTime, duration);
        } else {
            m_CurrentTime = duration;
            m_Playing = false;
        }
    }

    AnimationKeyframe frame = m_Clip->Interpolate(m_CurrentTime);
    owner->SetPosition(frame.position);
    owner->SetRotation(glm::degrees(glm::eulerAngles(frame.rotation)));
    owner->SetScale(frame.scale);
}

bool AnimationComponent::LoadFromFile(const std::string& path) {
    auto clip = std::make_shared<AnimationClip>();
    if (clip->LoadFromFile(path)) {
        m_Clip = clip;
        m_ClipPath = path;
        m_CurrentTime = 0.0f;
        return true;
    }
    return false;
}

nlohmann::json AnimationComponent::ToJson() const {
    nlohmann::json j;
    j["clipPath"] = m_ClipPath;
    j["speed"] = m_Speed;
    j["looping"] = m_Looping;
    j["currentTime"] = m_CurrentTime;
    j["playing"] = m_Playing;
    j["paused"] = m_Paused;
    return j;
}

bool AnimationComponent::FromJson(const nlohmann::json& j) {
    try {
        if (j.contains("clipPath")) {
            m_ClipPath = j["clipPath"];
            if (!m_ClipPath.empty()) {
                LoadFromFile(m_ClipPath);
            }
        }
        if (j.contains("speed")) m_Speed = j["speed"];
        if (j.contains("looping")) m_Looping = j["looping"];
        if (j.contains("currentTime")) m_CurrentTime = j["currentTime"];
        if (j.contains("playing")) m_Playing = j["playing"];
        if (j.contains("paused")) m_Paused = j["paused"];
        return true;
    } catch (...) {
        return false;
    }
}