#include "AnimationKeyframe.h"

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