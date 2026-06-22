#pragma once
#include <memory>
#include <string>
#include "AnimationClip.h"

class GameObject;

class AnimationComponent {
public:
    AnimationComponent() = default;
    ~AnimationComponent() = default;

    void SetClip(std::shared_ptr<AnimationClip> clip);
    std::shared_ptr<AnimationClip> GetClip() const { return m_Clip; }
    std::string GetClipPath() const { return m_ClipPath; }

    void Play();
    void Stop();
    void Pause();
    void SetTime(float time);
    float GetTime() const { return m_CurrentTime; }
    void SetSpeed(float speed) { m_Speed = speed; }
    float GetSpeed() const { return m_Speed; }
    void SetLooping(bool loop) { m_Looping = loop; }
    bool IsLooping() const { return m_Looping; }
    bool IsPlaying() const { return m_Playing && !m_Paused; }
    bool IsPaused() const { return m_Paused; }

    void Update(float deltaTime, GameObject* owner);

    // Загрузка из файла (устанавливает clip)
    bool LoadFromFile(const std::string& path);

    // Сериализация (сохраняем только путь к файлу и параметры)
    nlohmann::json ToJson() const;
    bool FromJson(const nlohmann::json& j);

private:
    std::shared_ptr<AnimationClip> m_Clip;
    std::string m_ClipPath;           // путь к .bxanim файлу
    float m_CurrentTime = 0.0f;
    float m_Speed = 1.0f;
    bool m_Looping = false;
    bool m_Playing = false;
    bool m_Paused = false;
};