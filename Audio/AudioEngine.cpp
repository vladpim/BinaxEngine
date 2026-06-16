#define MINIAUDIO_IMPLEMENTATION
#include "AudioEngine.h"
#include <iostream>

AudioEngine& AudioEngine::Get() {
    static AudioEngine instance;
    return instance;
}

bool AudioEngine::Initialize() {
    if (m_Initialized) return true;
    if (ma_engine_init(nullptr, &m_Engine) != MA_SUCCESS) {
        std::cerr << "[Audio] Failed to initialize engine" << std::endl;
        return false;
    }
    for (int i = 0; i < 256; ++i) {
        m_Slots[i].active = false;
        m_Slots[i].id = 0;
    }
    m_Initialized = true;
    std::cout << "[Audio] AudioEngine initialized" << std::endl;
    return true;
}

void AudioEngine::Shutdown() {
    if (!m_Initialized) return;
    for (int i = 0; i < 256; ++i) {
        if (m_Slots[i].active) {
            ma_sound_uninit(&m_Slots[i].sound);
            m_Slots[i].active = false;
        }
    }
    ma_engine_uninit(&m_Engine);
    m_Initialized = false;
}

void AudioEngine::Update(const glm::vec3& listenerPos,
                         const glm::vec3& listenerForward,
                         const glm::vec3& listenerUp) {
    if (!m_Initialized) return;
    ma_engine_listener_set_position(&m_Engine, 0, listenerPos.x, listenerPos.y, listenerPos.z);
    ma_engine_listener_set_direction(&m_Engine, 0, listenerForward.x, listenerForward.y, listenerForward.z);
    ma_engine_listener_set_world_up(&m_Engine, 0, listenerUp.x, listenerUp.y, listenerUp.z);
}

uint64_t AudioEngine::LoadSound(const std::string& filePath) {
    if (!m_Initialized) return 0;

    // Проверяем, не загружен ли уже этот файл
    auto it = m_SoundCache.find(filePath);
    if (it != m_SoundCache.end()) {
        // Найден существующий звук – возвращаем его ID
        return it->second;
    }

    // Ищем свободный слот для нового звука
    for (int i = 0; i < 256; ++i) {
        if (!m_Slots[i].active) {
            if (ma_sound_init_from_file(&m_Engine, filePath.c_str(), 0, nullptr, nullptr, &m_Slots[i].sound) == MA_SUCCESS) {
                m_Slots[i].active = true;
                m_Slots[i].id = m_NextId++;
                // Сохраняем в кэш
                m_SoundCache[filePath] = m_Slots[i].id;
                return m_Slots[i].id;
            } else {
                std::cerr << "[Audio] Failed to load sound: " << filePath << std::endl;
                return 0;
            }
        }
    }
    std::cerr << "[Audio] No free sound slot" << std::endl;
    return 0;
}

void AudioEngine::UnloadSound(uint64_t id) {
    for (int i = 0; i < 256; ++i) {
        if (m_Slots[i].active && m_Slots[i].id == id) {
            ma_sound_uninit(&m_Slots[i].sound);
            m_Slots[i].active = false;
            // Удаляем из кэша (перебор всех – неэффективно, но допустимо)
            for (auto it = m_SoundCache.begin(); it != m_SoundCache.end(); ) {
                if (it->second == id) {
                    it = m_SoundCache.erase(it);
                } else {
                    ++it;
                }
            }
            break;
        }
    }
}

void AudioEngine::Play2D(uint64_t id, float volume, bool loop) {
    for (int i = 0; i < 256; ++i) {
        if (m_Slots[i].active && m_Slots[i].id == id) {
            ma_sound_set_looping(&m_Slots[i].sound, loop);
            ma_sound_set_volume(&m_Slots[i].sound, volume);
            ma_sound_set_spatialization_enabled(&m_Slots[i].sound, 0);   // 0 = отключить пространственность
            ma_sound_start(&m_Slots[i].sound);
            break;
        }
    }
}

void AudioEngine::Play3D(uint64_t id, const glm::vec3& position, float volume, bool loop,
                         float minDistance, float maxDistance) {
    for (int i = 0; i < 256; ++i) {
        if (m_Slots[i].active && m_Slots[i].id == id) {
            ma_sound_set_looping(&m_Slots[i].sound, loop);
            ma_sound_set_volume(&m_Slots[i].sound, volume);
            // ВКЛЮЧАЕМ 3D (было отключено в Play2D)
            ma_sound_set_spatialization_enabled(&m_Slots[i].sound, 1);   // <-- ДОБАВИТЬ ЭТУ СТРОКУ
            ma_sound_set_positioning(&m_Slots[i].sound, static_cast<ma_positioning>(2));
            ma_sound_set_position(&m_Slots[i].sound, position.x, position.y, position.z);
            ma_sound_set_min_distance(&m_Slots[i].sound, minDistance);
            ma_sound_set_max_distance(&m_Slots[i].sound, maxDistance);
            ma_sound_set_attenuation_model(&m_Slots[i].sound, ma_attenuation_model_inverse);
            ma_sound_start(&m_Slots[i].sound);
            break;
        }
    }
}

void AudioEngine::Stop(uint64_t id) {
    for (int i = 0; i < 256; ++i) {
        if (m_Slots[i].active && m_Slots[i].id == id) {
            if (ma_sound_is_playing(&m_Slots[i].sound)) {
                ma_sound_stop(&m_Slots[i].sound);
            }
            break;
        }
    }
}

bool AudioEngine::IsPlaying(uint64_t id) const {
    for (int i = 0; i < 256; ++i) {
        if (m_Slots[i].active && m_Slots[i].id == id) {
            return ma_sound_is_playing(&m_Slots[i].sound);
        }
    }
    return false;
}

void AudioEngine::UpdateSoundPosition(uint64_t id, const glm::vec3& position) {
    for (int i = 0; i < 256; ++i) {
        if (m_Slots[i].active && m_Slots[i].id == id && ma_sound_is_playing(&m_Slots[i].sound)) {
            ma_sound_set_position(&m_Slots[i].sound, position.x, position.y, position.z);
            break;
        }
    }
}

void AudioEngine::SetMasterVolume(float volume) {
    if (m_Initialized)
        ma_engine_set_volume(&m_Engine, volume);
}

void AudioEngine::UpdateVolume(uint64_t id, float volume) {
    for (int i = 0; i < 256; ++i) {
        if (m_Slots[i].active && m_Slots[i].id == id) {
            ma_sound_set_volume(&m_Slots[i].sound, volume);
            break;
        }
    }
}

void AudioEngine::UpdateMinDistance(uint64_t id, float minDist) {
    for (int i = 0; i < 256; ++i) {
        if (m_Slots[i].active && m_Slots[i].id == id) {
            ma_sound_set_min_distance(&m_Slots[i].sound, minDist);
            break;
        }
    }
}

void AudioEngine::UpdateMaxDistance(uint64_t id, float maxDist) {
    for (int i = 0; i < 256; ++i) {
        if (m_Slots[i].active && m_Slots[i].id == id) {
            ma_sound_set_max_distance(&m_Slots[i].sound, maxDist);
            break;
        }
    }
}