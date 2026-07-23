#include "InputManager.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>

GLFWwindow* InputManager::s_Window = nullptr;
double InputManager::s_LastMouseX = 0.0;
double InputManager::s_LastMouseY = 0.0;
double InputManager::s_MouseDeltaX = 0.0;
double InputManager::s_MouseDeltaY = 0.0;
float InputManager::s_MouseWheelDelta = 0.0f;
bool InputManager::s_MouseCaptured = false;
bool InputManager::s_FirstMouse = true;
std::unordered_map<std::string, int> InputManager::s_KeyMap;
std::unordered_map<std::string, int> InputManager::s_MouseButtonMap;

void InputManager::Init(GLFWwindow* window) {
    s_Window = window;
    InitMaps();
    s_FirstMouse = true;
    s_MouseCaptured = false;
    s_MouseWheelDelta = 0.0f;
}

void InputManager::InitMaps() {
    // Клавиши (наиболее используемые)
    s_KeyMap = {
        {"A", GLFW_KEY_A}, {"B", GLFW_KEY_B}, {"C", GLFW_KEY_C}, {"D", GLFW_KEY_D},
        {"E", GLFW_KEY_E}, {"F", GLFW_KEY_F}, {"G", GLFW_KEY_G}, {"H", GLFW_KEY_H},
        {"I", GLFW_KEY_I}, {"J", GLFW_KEY_J}, {"K", GLFW_KEY_K}, {"L", GLFW_KEY_L},
        {"M", GLFW_KEY_M}, {"N", GLFW_KEY_N}, {"O", GLFW_KEY_O}, {"P", GLFW_KEY_P},
        {"Q", GLFW_KEY_Q}, {"R", GLFW_KEY_R}, {"S", GLFW_KEY_S}, {"T", GLFW_KEY_T},
        {"U", GLFW_KEY_U}, {"V", GLFW_KEY_V}, {"W", GLFW_KEY_W}, {"X", GLFW_KEY_X},
        {"Y", GLFW_KEY_Y}, {"Z", GLFW_KEY_Z},
        {"0", GLFW_KEY_0}, {"1", GLFW_KEY_1}, {"2", GLFW_KEY_2}, {"3", GLFW_KEY_3},
        {"4", GLFW_KEY_4}, {"5", GLFW_KEY_5}, {"6", GLFW_KEY_6}, {"7", GLFW_KEY_7},
        {"8", GLFW_KEY_8}, {"9", GLFW_KEY_9},
        {"SPACE", GLFW_KEY_SPACE}, {"SHIFT", GLFW_KEY_LEFT_SHIFT},
        {"CTRL", GLFW_KEY_LEFT_CONTROL}, {"ALT", GLFW_KEY_LEFT_ALT},
        {"ENTER", GLFW_KEY_ENTER}, {"ESCAPE", GLFW_KEY_ESCAPE},
        {"UP", GLFW_KEY_UP}, {"DOWN", GLFW_KEY_DOWN}, {"LEFT", GLFW_KEY_LEFT}, {"RIGHT", GLFW_KEY_RIGHT},
        {"TAB", GLFW_KEY_TAB}, {"BACKSPACE", GLFW_KEY_BACKSPACE},
        {"F1", GLFW_KEY_F1}, {"F2", GLFW_KEY_F2}, {"F3", GLFW_KEY_F3}, {"F4", GLFW_KEY_F4},
        {"F5", GLFW_KEY_F5}, {"F6", GLFW_KEY_F6}, {"F7", GLFW_KEY_F7}, {"F8", GLFW_KEY_F8},
        {"F9", GLFW_KEY_F9}, {"F10", GLFW_KEY_F10}, {"F11", GLFW_KEY_F11}, {"F12", GLFW_KEY_F12}
    };

    s_MouseButtonMap = {
        {"LEFT", GLFW_MOUSE_BUTTON_LEFT},
        {"RIGHT", GLFW_MOUSE_BUTTON_RIGHT},
        {"MIDDLE", GLFW_MOUSE_BUTTON_MIDDLE}
    };
}

void InputManager::Update() {
    if (!s_Window) return;

    double xpos, ypos;
    glfwGetCursorPos(s_Window, &xpos, &ypos);

    if (s_FirstMouse) {
        s_LastMouseX = xpos;
        s_LastMouseY = ypos;
        s_FirstMouse = false;
        s_MouseDeltaX = 0.0;
        s_MouseDeltaY = 0.0;
    } else {
        s_MouseDeltaX = xpos - s_LastMouseX;
        s_MouseDeltaY = ypos - s_LastMouseY;
        s_LastMouseX = xpos;
        s_LastMouseY = ypos;
    }

    // Сбрасываем колёсико после одного чтения (его будем забирать в GetMouseWheelDelta)
    // Но не сбрасываем здесь, чтобы можно было прочитать несколько раз в кадре, 
    // сбросим при вызове GetMouseWheelDelta.
}

bool InputManager::IsKeyPressed(const std::string& keyName) {
    if (!s_Window) return false;
    auto it = s_KeyMap.find(keyName);
    if (it == s_KeyMap.end()) {
        std::cerr << "[InputManager] Unknown key: " << keyName << std::endl;
        return false;
    }
    int state = glfwGetKey(s_Window, it->second);
    return state == GLFW_PRESS;
}

bool InputManager::IsMouseButtonPressed(const std::string& buttonName) {
    if (!s_Window) return false;
    auto it = s_MouseButtonMap.find(buttonName);
    if (it == s_MouseButtonMap.end()) {
        std::cerr << "[InputManager] Unknown mouse button: " << buttonName << std::endl;
        return false;
    }
    int state = glfwGetMouseButton(s_Window, it->second);
    return state == GLFW_PRESS;
}

glm::vec2 InputManager::GetMousePosition() {
    if (!s_Window) return glm::vec2(0.0f, 0.0f);
    double x, y;
    glfwGetCursorPos(s_Window, &x, &y);
    return glm::vec2((float)x, (float)y);
}

glm::vec2 InputManager::GetMouseDelta() {
    return glm::vec2((float)s_MouseDeltaX, (float)s_MouseDeltaY);
}

float InputManager::GetMouseWheelDelta() {
    float val = s_MouseWheelDelta;
    s_MouseWheelDelta = 0.0f; // сброс после чтения
    return val;
}

bool InputManager::IsMouseCaptured() {
    return s_MouseCaptured;
}

void InputManager::SetMouseCaptured(bool captured) {
    if (s_MouseCaptured == captured) return;
    s_MouseCaptured = captured;
    if (s_Window) {
        glfwSetInputMode(s_Window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        if (captured) {
            // Сбросить первый кадр, чтобы дельта не скакала
            s_FirstMouse = true;
        }
    }
}

void InputManager::AddMouseWheelDelta(float delta) {
    s_MouseWheelDelta += delta;
}