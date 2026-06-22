#include "AnimationComponent.h"
#include "Scene/GameObject.h"
#include <iostream>

void AnimationComponent::SetClip(std::shared_ptr<AnimationClip> clip) {
    m_Clip = clip;
    m_CurrentTime = 0.0f;
}

void AnimationComponent::Play() {
    if (!m_Clip || m_Clip->GetKeyframes().empty()) return;
    m_Playing = true;
    m_Paused = false;
    // Если достигли конца, начинаем сначала
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
            // Можно вызвать событие окончания (пока нет)
        }
    }

    // Интерполируем и применяем
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
            // Загружаем клип, если файл существует
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