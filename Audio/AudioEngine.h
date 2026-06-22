#pragma once
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "External/miniaudio.h"

class AudioEngine {
public:
    static AudioEngine& Get();

    bool Initialize();
    void Shutdown();
    void Update(const glm::vec3& listenerPos,
                const glm::vec3& listenerForward,
                const glm::vec3& listenerUp);

    // Загрузка звука из файла, возвращает ID
    uint64_t LoadSound(const std::string& filePath);
    void UnloadSound(uint64_t id);

    // 2D воспроизведение
    void Play2D(uint64_t id, float volume = 1.0f, bool loop = false);
    // 3D воспроизведение
    void Play3D(uint64_t id, const glm::vec3& position,
                float volume = 1.0f, bool loop = false,
                float minDistance = 1.0f, float maxDistance = 50.0f);
    void Stop(uint64_t id);
    void SetMasterVolume(float volume);
    bool IsPlaying(uint64_t id) const;
    // Для обновления позиции движущегося звука (3D)
    void UpdateSoundPosition(uint64_t id, const glm::vec3& position);
    void UpdateVolume(uint64_t id, float volume);
    void UpdateMinDistance(uint64_t id, float minDist);
    void UpdateMaxDistance(uint64_t id, float maxDist);

private:
    AudioEngine() = default;
    ~AudioEngine() { Shutdown(); }

    ma_engine m_Engine;
    struct SoundSlot {
        ma_sound sound;
        uint64_t id;
        bool active;
    };
    SoundSlot m_Slots[256];          // до 256 одновременных звуков
    uint64_t m_NextId = 1;
    bool m_Initialized = false;
    std::unordered_map<std::string, uint64_t> m_SoundCache;
};
