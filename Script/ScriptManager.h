#pragma once
#include <sol/sol.hpp>
#include <memory>
#include <string>

struct GLFWwindow;   // <-- forward declaration

class SceneManager;

class ScriptManager {
public:
    static ScriptManager& GetInstance();

    bool Initialize(SceneManager* sceneManager, GLFWwindow* window);
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
    GLFWwindow* m_Window = nullptr;
    bool m_Initialized = false;
};
