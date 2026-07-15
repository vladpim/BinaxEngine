#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "imgui.h"
#include "ImGuizmo.h"
#include "EditorTheme.h"
#include <nlohmann/json.hpp>
#include "Animation/Animation.h"

class SceneManager;
class GameObject;
class Skybox;

struct EditorSettings {
    bool show_gizmo = true;
    bool wireframe_mode = false;
    bool grid_enabled = true;
    bool snap_to_grid = false;
    bool skyboxSeamless = true;
    float grid_size = 1.0f;
    float bg_color[3] = {0.1f, 0.1f, 0.1f};
    float shininess = 32.0f;
    float metallic = 0.1f;
    float roughness = 0.3f;
    float light_intensity = 1.0f;
    glm::vec3 light_pos = glm::vec3(2.0f, 4.0f, 2.0f);
    glm::vec3 light_color = glm::vec3(1.0f, 1.0f, 1.0f);
    bool shadows_enabled = true;
    float shadow_bias = 0.005f;
    bool useSnap = false;
    float snapTranslation = 0.1f;
    float snapRotation = 5.0f;
    float snapScale = 0.1f;
    bool vsync = true;
    int shadowMapSize = 4096;
    float shadowSoftness = 2.0f;
    int shadowSamples = 9;
    float shadowNearQualityRadius = 30.0f;
    float shadowFarClip = 100.0f;
    bool shadowHighQualityMode = true;
    float ambientStrength = 0.05f;
};
class EditorUI {
public:
    EditorUI();
    ~EditorUI();
    bool Initialize(GLFWwindow* window, SceneManager* sceneManager);
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    void Render();
    const EditorSettings& GetSettings() const { return m_Settings; }
    EditorSettings& GetSettings() { return m_Settings; }
    std::shared_ptr<GameObject> GetSelectedObject() const;
    void SetSelectedObject(std::shared_ptr<GameObject> obj);
    void HandleShortcuts();
    void SetViewProjection(const glm::mat4& view, const glm::mat4& projection);
    bool IsViewportHovered() const { return m_ViewportHovered; }
    GLFWwindow* GetWindow() const { return m_Window; }
    void DrawThemeEditor();
    void SetSkybox(Skybox* skybox) { m_Skybox = skybox; }
    bool IsGizmoActive() const { return m_GizmoActive; }
    std::string SaveFileDialog(const char* filter, const char* defaultExt = "binaxmat");
    void DrawAudioSourceUI(std::shared_ptr<GameObject> selected);
    std::string m_CurrentScenePath = "scene.bxlvl";
    nlohmann::json SettingsToJson() const;
    bool SettingsFromJson(const nlohmann::json& j);
    void TogglePlayMode();
    bool IsPlayMode() const { return m_IsPlayMode; }
        void StartRecording(std::shared_ptr<GameObject> target);
    void StopRecording();
    void AddKeyframe();
    bool IsRecording() const { return m_IsRecording; }

private:
    void SetupImGuiStyle();
    void DrawMainMenuBar();
    void DrawHierarchy();
    void DrawInspector();
    void DrawSceneView();
    void DrawSceneSettings();
    void DrawContentBrowser();
    void DrawMaterialSettings();
    void DrawTransformControls(std::shared_ptr<GameObject> obj);
    void DrawObjectTreeNode(std::shared_ptr<GameObject> obj, int& id);
    void DrawGizmoToolbar();
    void DrawMaterialControls(std::shared_ptr<GameObject> obj);
    void DrawSkyboxSettings();
    void DrawShadowsSettings();
    void DrawPhysicsComponents(std::shared_ptr<GameObject> obj);
    std::string OpenFileDialog(const char* filter);
    GLFWwindow* m_Window = nullptr;
    SceneManager* m_SceneManager = nullptr;
    ImGuiContext* m_ImGuiContext = nullptr;
    EditorSettings m_Settings;
    bool m_ShowDemoWindow = false;
    bool m_ShowMetricsWindow = false;
    bool m_ShowAboutPopup = false;
    bool m_ShowSkyboxSettings = false;
    bool m_ShowShadowsSettings = false;
    bool m_ShowThemeEditor = false;
    bool m_FirstLaunch = false;
    bool m_GizmoActive = false;
    float m_MenuBarHeight = 0.0f;
    ImVec2 m_ViewportSize;
    ImVec2 m_ViewportPos;
    bool m_ViewportHovered = false;
    bool m_ViewportFocused = false;
    glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
    glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
    ImGuizmo::OPERATION m_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE m_CurrentGizmoMode = ImGuizmo::WORLD;
    EditorTheme m_Theme;
    bool m_IsPlayMode = false;
        bool m_IsRecording = false;
    std::shared_ptr<GameObject> m_RecordingTarget;
    std::shared_ptr<AnimationComponent> m_RecordingComponent;
    float m_RecordingTime = 0.0f;
    std::shared_ptr<AnimationClip> m_RecordingClip;
    Skybox* m_Skybox = nullptr;
    std::string m_SkyboxPaths[6] = {
    "resources/embedded_assets/skybox/right.png",
    "resources/embedded_assets/skybox/left.png",
    "resources/embedded_assets/skybox/top.png",
    "resources/embedded_assets/skybox/bottom.png",
    "resources/embedded_assets/skybox/front.png",
    "resources/embedded_assets/skybox/back.png"
    };
};
