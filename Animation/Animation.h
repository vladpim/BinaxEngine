#pragma once

#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>

// Forward declaration
class GameObject;

// ============================================================================
// AnimationKeyframe
// ============================================================================
struct AnimationKeyframe {
    float time = 0.0f;
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    nlohmann::json ToJson() const;
    bool FromJson(const nlohmann::json& j);
};

// ============================================================================
// AnimationClip
// ============================================================================
class AnimationClip {
public:
    AnimationClip() = default;
    explicit AnimationClip(const std::string& name) : m_Name(name) {}

    void AddKeyframe(const AnimationKeyframe& key);
    void SortKeyframes();
    void Clear();

    AnimationKeyframe Interpolate(float time) const;

    float GetDuration() const { return m_Duration; }
    const std::string& GetName() const { return m_Name; }
    const std::vector<AnimationKeyframe>& GetKeyframes() const { return m_Keyframes; }

    nlohmann::json ToJson() const;
    bool FromJson(const nlohmann::json& j);

    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);

private:
    std::string m_Name;
    std::vector<AnimationKeyframe> m_Keyframes;
    float m_Duration = 0.0f;
};

// ============================================================================
// AnimationComponent
// ============================================================================
class AnimationComponent {
public:
    AnimationComponent() = default;
    ~AnimationComponent() = default;

    void SetClip(std::shared_ptr<AnimationClip> clip);
    void Play();
    void Stop();
    void Pause();
    void SetTime(float time);
    void Update(float deltaTime, GameObject* owner);

    bool LoadFromFile(const std::string& path);

    // Геттеры / сеттеры
    std::shared_ptr<AnimationClip> GetClip() const { return m_Clip; }
    float GetTime() const { return m_CurrentTime; }
    float GetSpeed() const { return m_Speed; }
    void SetSpeed(float speed) { m_Speed = speed; }
    bool IsLooping() const { return m_Looping; }
    void SetLooping(bool loop) { m_Looping = loop; }
    bool IsPlaying() const { return m_Playing; }
    bool IsPaused() const { return m_Paused; }
    const std::string& GetClipPath() const { return m_ClipPath; }

    // Сериализация
    nlohmann::json ToJson() const;
    bool FromJson(const nlohmann::json& j);

private:
    std::shared_ptr<AnimationClip> m_Clip;
    std::string m_ClipPath;
    float m_CurrentTime = 0.0f;
    float m_Speed = 1.0f;
    bool m_Looping = true;
    bool m_Playing = false;
    bool m_Paused = false;
};