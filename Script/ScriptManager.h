#pragma once
#include <sol/sol.hpp>
#include <memory>
#include <string>

class SceneManager;

class ScriptManager {
public:
    static ScriptManager& GetInstance();

    bool Initialize(SceneManager* sceneManager);
    void Shutdown();

    bool LoadScript(const std::string& filePath);
    bool ExecuteString(const std::string& code);

    sol::state& GetState() { return m_LuaState; }

private:
    ScriptManager() = default;
    ~ScriptManager() = default;
    ScriptManager(const ScriptManager&) = delete;
    ScriptManager& operator=(const ScriptManager&) = delete;

    void RegisterBindings();

    sol::state m_LuaState;
    SceneManager* m_SceneManager = nullptr;
    bool m_Initialized = false;
};