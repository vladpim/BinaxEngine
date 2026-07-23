#pragma once
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

struct GLFWwindow;   // <-- forward declaration

class InputManager {
public:
    static void Init(GLFWwindow* window);
    static void Update();
    static bool IsKeyPressed(const std::string& keyName);
    static bool IsMouseButtonPressed(const std::string& buttonName);
    static glm::vec2 GetMousePosition();
    static glm::vec2 GetMouseDelta();
    static float GetMouseWheelDelta();
    static bool IsMouseCaptured();
    static void SetMouseCaptured(bool captured);
    static void AddMouseWheelDelta(float delta);

private:
    static void InitMaps();
    static GLFWwindow* s_Window;
    static double s_LastMouseX, s_LastMouseY;
    static double s_MouseDeltaX, s_MouseDeltaY;
    static float s_MouseWheelDelta;
    static bool s_MouseCaptured;
    static bool s_FirstMouse;
    static std::unordered_map<std::string, int> s_KeyMap;
    static std::unordered_map<std::string, int> s_MouseButtonMap;
};