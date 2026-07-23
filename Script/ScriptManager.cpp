#include "ScriptManager.h"
#include "Scene/SceneManager.h"
#include "Scene/GameObject.h"
#include "Input/InputManager.h"
#include <iostream>
#include <filesystem>

ScriptManager& ScriptManager::GetInstance() {
    static ScriptManager instance;
    return instance;
}

bool ScriptManager::Initialize(SceneManager* sceneManager, GLFWwindow* window) {
    if (m_Initialized) return true;
    m_SceneManager = sceneManager;
    m_Window = window;

    try {
        m_LuaState.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table,
                                  sol::lib::string, sol::lib::os, sol::lib::io);

        RegisterBindings();

        m_Initialized = true;
        std::cout << "[ScriptManager] Initialized successfully." << std::endl;
        return true;
    } catch (const sol::error& e) {
        std::cerr << "[ScriptManager] Initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void ScriptManager::Shutdown() {
    if (m_Initialized) {
        m_LuaState.collect_garbage();
        m_Initialized = false;
        std::cout << "[ScriptManager] Shutdown." << std::endl;
    }
}

bool ScriptManager::LoadScript(const std::string& filePath) {
    if (!m_Initialized) return false;
    try {
        m_LuaState.script_file(filePath);
        std::cout << "[ScriptManager] Loaded script: " << filePath << std::endl;
        return true;
    } catch (const sol::error& e) {
        std::cerr << "[ScriptManager] Error loading script " << filePath << ": " << e.what() << std::endl;
        return false;
    }
}

bool ScriptManager::ExecuteString(const std::string& code) {
    if (!m_Initialized) return false;
    try {
        m_LuaState.script(code);
        return true;
    } catch (const sol::error& e) {
        std::cerr << "[ScriptManager] Error executing string: " << e.what() << std::endl;
        return false;
    }
}

void ScriptManager::RegisterBindings() {
    sol::state& lua = m_LuaState;

    // Регистрируем glm::vec3
    lua.new_usertype<glm::vec3>("Vec3",
        sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z,
        sol::meta_function::addition, [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
        sol::meta_function::subtraction, [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
        sol::meta_function::multiplication, [](const glm::vec3& a, float s) { return a * s; }
    );

        // Регистрируем glm::vec2
    lua.new_usertype<glm::vec2>("Vec2",
        sol::constructors<glm::vec2(), glm::vec2(float, float)>(),
        "x", &glm::vec2::x,
        "y", &glm::vec2::y,
        sol::meta_function::addition, [](const glm::vec2& a, const glm::vec2& b) { return a + b; },
        sol::meta_function::subtraction, [](const glm::vec2& a, const glm::vec2& b) { return a - b; },
        sol::meta_function::multiplication, [](const glm::vec2& a, float s) { return a * s; }
    );

    // Регистрируем GameObject
    lua.new_usertype<GameObject>("GameObject",
        "GetName", &GameObject::GetName,
        "SetName", &GameObject::SetName,
        "GetPosition", &GameObject::GetPosition,
        "SetPosition", [](GameObject& obj, const glm::vec3& pos) { obj.SetPosition(pos); },
        "GetWorldPosition", &GameObject::GetWorldPosition,
        "SetVisible", &GameObject::SetVisible,
        "IsVisible", &GameObject::IsVisible,
        "GetRotation", &GameObject::GetRotation,
        "SetRotation", [](GameObject& obj, const glm::vec3& rot) { obj.SetRotation(rot); },
        "GetScale", &GameObject::GetScale,
        "SetScale", [](GameObject& obj, const glm::vec3& scale) { obj.SetScale(scale); }
        // Добавьте другие методы по необходимости
    );

    // Глобальные функции
    lua.set_function("FindGameObject", [this](const std::string& name) -> std::shared_ptr<GameObject> {
        if (!m_SceneManager) return nullptr;
        for (const auto& obj : m_SceneManager->GetObjects()) {
            if (obj->GetName() == name) {
                return obj;
            }
        }
        return nullptr;
    });

    lua.set_function("GetAllGameObjects", [this]() -> std::vector<std::shared_ptr<GameObject>> {
        if (!m_SceneManager) return {};
        return m_SceneManager->GetObjects();
    });

    lua.set_function("Print", [](const std::string& msg) {
        std::cout << "[Lua] " << msg << std::endl;
    });

        // ===== ВВОД =====
    lua.set_function("IsKeyPressed", [](const std::string& key) -> bool {
        return InputManager::IsKeyPressed(key);
    });
    lua.set_function("IsMouseButtonPressed", [](const std::string& button) -> bool {
        return InputManager::IsMouseButtonPressed(button);
    });
    lua.set_function("GetMousePosition", []() -> glm::vec2 {
        return InputManager::GetMousePosition();
    });
    lua.set_function("GetMouseDelta", []() -> glm::vec2 {
        return InputManager::GetMouseDelta();
    });
    lua.set_function("GetMouseWheelDelta", []() -> float {
        return InputManager::GetMouseWheelDelta();
    });
    lua.set_function("IsMouseCaptured", []() -> bool {
        return InputManager::IsMouseCaptured();
    });
    lua.set_function("SetMouseCaptured", [](bool captured) {
        InputManager::SetMouseCaptured(captured);
    });

    // Пример доступа к настройкам сцены (ambient, etc.)
    // Можно добавить позже.
}
