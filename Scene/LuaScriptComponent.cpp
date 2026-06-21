#include "LuaScriptComponent.h"
#include "Script/ScriptManager.h"
#include "Scene/GameObject.h"
#include <iostream>
#include <filesystem>

LuaScriptComponent::~LuaScriptComponent() {
    OnDestroy();
}

bool LuaScriptComponent::LoadScript(const std::string& filePath, GameObject* owner) {
    m_Owner = owner;
    m_ScriptPath = filePath;
    std::cout << "[LuaScript] LoadScript called for " << filePath << std::endl;
    return Reload();
}

bool LuaScriptComponent::Reload() {
    if (m_ScriptPath.empty() || !m_Owner) {
        std::cerr << "[LuaScript] Reload: m_ScriptPath empty or m_Owner null" << std::endl;
        return false;
    }

    if (!std::filesystem::exists(m_ScriptPath)) {
        std::cerr << "[LuaScript] File does not exist: " << m_ScriptPath << std::endl;
        return false;
    }

    auto& scriptMgr = ScriptManager::GetInstance();
    sol::state& lua = scriptMgr.GetState();

    // ГАРАНТИРУЕМ РЕГИСТРАЦИЮ printToConsole
    lua.set_function("printToConsole", [](const std::string& msg) {
        std::cout << "[Lua] " << msg << std::endl;
    });

    try {
        sol::load_result loaded = lua.load_file(m_ScriptPath);
        if (!loaded.valid()) {
            sol::error err = loaded;
            std::cerr << "[LuaScript] Failed to load script: " << err.what() << std::endl;
            return false;
        }

        sol::protected_function scriptFunc = loaded;
        auto result = scriptFunc();
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[LuaScript] Execution error: " << err.what() << std::endl;
            return false;
        }

        m_OnStart = lua["on_start"];
        m_OnUpdate = lua["on_update"];
        m_OnDestroy = lua["on_destroy"];

        m_Loaded = true;
        std::cout << "[LuaScript] Script loaded successfully: " << m_ScriptPath << std::endl;
        std::cout << "[LuaScript] on_start valid: " << (m_OnStart.valid() ? "yes" : "no") << std::endl;
        std::cout << "[LuaScript] on_update valid: " << (m_OnUpdate.valid() ? "yes" : "no") << std::endl;
        return true;
    } catch (const sol::error& e) {
        std::cerr << "[LuaScript] Exception: " << e.what() << std::endl;
        m_Loaded = false;
        return false;
    }
}

void LuaScriptComponent::Start() {
    if (!m_Loaded || !m_OnStart.valid()) return;
    try {
        auto& scriptMgr = ScriptManager::GetInstance();
        sol::state& lua = scriptMgr.GetState();
        lua["self"] = m_Owner;   // передаём указатель на GameObject

        auto result = m_OnStart();
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[LuaScript] on_start error: " << err.what() << std::endl;
        }
    } catch (const sol::error& e) {
        std::cerr << "[LuaScript] on_start exception: " << e.what() << std::endl;
    }
}

void LuaScriptComponent::Update(float deltaTime) {
    if (!m_Loaded || !m_OnUpdate.valid()) return;
    try {
        auto& scriptMgr = ScriptManager::GetInstance();
        sol::state& lua = scriptMgr.GetState();
        lua["self"] = m_Owner;

        auto result = m_OnUpdate(deltaTime);
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[LuaScript] on_update error: " << err.what() << std::endl;
        }
    } catch (const sol::error& e) {
        std::cerr << "[LuaScript] on_update exception: " << e.what() << std::endl;
    }
}

void LuaScriptComponent::OnDestroy() {
    if (!m_Loaded || !m_OnDestroy.valid()) return;
    try {
        auto& scriptMgr = ScriptManager::GetInstance();
        sol::state& lua = scriptMgr.GetState();
        lua["self"] = m_Owner;

        auto result = m_OnDestroy();
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[LuaScript] on_destroy error: " << err.what() << std::endl;
        }
    } catch (const sol::error& e) {
        std::cerr << "[LuaScript] on_destroy exception: " << e.what() << std::endl;
    }
    m_Loaded = false;
}