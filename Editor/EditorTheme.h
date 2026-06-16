#pragma once
#include <glm/glm.hpp>
#include <imgui.h>

struct EditorTheme {
    // Основные цвета (как glm::vec3 для удобства)
    glm::vec3 accent = glm::vec3(0.95f, 0.55f, 0.15f); // Оранжево-золотой
    glm::vec3 background = glm::vec3(0.08f, 0.08f, 0.08f); // Тёмно-серый
    glm::vec3 text = glm::vec3(1.0f, 1.0f, 1.0f); // Белый
    glm::vec3 header = glm::vec3(0.15f, 0.15f, 0.15f); // Для заголовков окон
    glm::vec3 headerHovered = glm::vec3(0.25f, 0.25f, 0.25f);
    glm::vec3 headerActive = glm::vec3(0.35f, 0.35f, 0.35f);
    glm::vec3 button = glm::vec3(0.15f, 0.15f, 0.15f);
    glm::vec3 buttonHovered = glm::vec3(0.25f, 0.25f, 0.25f);
    glm::vec3 buttonActive = glm::vec3(0.35f, 0.35f, 0.35f);
    glm::vec3 frameBg = glm::vec3(0.12f, 0.12f, 0.12f); // Фон полей ввода
    glm::vec3 titleBg = glm::vec3(0.10f, 0.10f, 0.10f); // Заголовок окна

    void ApplyToImGui() const {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_WindowBg] = ImVec4(background.x, background.y, background.z, 1.00f);
        colors[ImGuiCol_Text] = ImVec4(text.x, text.y, text.z, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(header.x, header.y, header.z, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(headerHovered.x, headerHovered.y, headerHovered.z, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(headerActive.x, headerActive.y, headerActive.z, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(button.x, button.y, button.z, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 1.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(frameBg.x, frameBg.y, frameBg.z, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(titleBg.x, titleBg.y, titleBg.z, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(titleBg.x, titleBg.y, titleBg.z, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(titleBg.x, titleBg.y, titleBg.z, 0.75f);
    }
};