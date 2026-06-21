#pragma once
#include <string>
#include <sol/sol.hpp>
#include <memory>

class GameObject;

class LuaScriptComponent {
public:
    LuaScriptComponent() = default;
    ~LuaScriptComponent();

    bool LoadScript(const std::string& filePath, GameObject* owner);
    void Start();
    void Update(float deltaTime);
    void OnDestroy();
    bool Reload();

    std::string GetScriptPath() const { return m_ScriptPath; }
    bool IsLoaded() const { return m_Loaded; }

private:
    std::string m_ScriptPath;
    GameObject* m_Owner = nullptr;
    sol::environment m_ScriptEnv;
    sol::protected_function m_OnStart;
    sol::protected_function m_OnUpdate;
    sol::protected_function m_OnDestroy;
    bool m_Loaded = false;
};