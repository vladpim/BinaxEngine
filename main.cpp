#define NOMINMAX
#include <stb_image.h>
#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Graphics/Shader.h"
#include "Graphics/Skybox.h"
#include "Graphics/Primitives.h"
#include "Scene/SceneManager.h"
#include "Editor/EditorUI.h"
#include "Scene/Frustum.h"
#include "Physics/PhysicsWorld.h"
#include "Audio/AudioEngine.h"
#include <windows.h>
#include <shellapi.h>

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

SceneManager g_SceneManager;
EditorUI g_EditorUI;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool firstMouse = true;
bool mouseCaptured = false;

Shader shader;
Shader gridShader;
Shader gizmoShader;
Shader skyboxShader;
Shader depthShader;
Shader screenFogShader;
Skybox skybox;

unsigned int depthMapFBO;
unsigned int depthMap;
unsigned int SHADOW_WIDTH = 4096;
unsigned int SHADOW_HEIGHT = 4096;

unsigned int framebuffer;
unsigned int sceneTexture;
unsigned int depthTexture;
unsigned int quadVAO, quadVBO;

// Прототипы
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
bool initShaders();
bool initShadowMap();
void initPostProcessing(int width, int height);
void renderFullScreenQuad();
glm::mat4 calculateLightSpaceMatrix(const glm::vec3& lightDir, const glm::mat4& viewMatrix,
                                    const glm::vec3& lightPos, float nearRadius, float farClip);

// Реализация initPostProcessing и renderFullScreenQuad (как у вас, но без ошибок)
void initPostProcessing(int width, int height) {
    // Создаём FBO
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    
    // Текстура для цвета
    glGenTextures(1, &sceneTexture);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture, 0);
    
    // Текстура глубины
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Framebuffer not complete!" << std::endl;
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Данные для полноэкранного квада
    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void renderFullScreenQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// Загрузка cubemap (должна быть вне main)
unsigned int LoadCubemap(const std::vector<std::string>& faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cerr << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    return textureID;
}

int main() {
    std::cout << "=== Binax Engine Editor ===" << std::endl;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Binax Engine Editor", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // Инициализация аудиодвижка
if (!AudioEngine::Get().Initialize()) {
    std::cerr << "Warning: AudioEngine failed to initialize" << std::endl;
} else {
    std::cout << "AudioEngine ready" << std::endl;
}

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed!" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GPU: " << glGetString(GL_RENDERER) << std::endl;

    g_SceneManager.InitializePhysics();
    g_SceneManager.Initialize();
    if (!g_EditorUI.Initialize(window, &g_SceneManager)) {
        std::cerr << "Failed to initialize EditorUI" << std::endl;
        return -1;
    }
    g_EditorUI.SetSkybox(&skybox);

    if (!initShaders()) return -1;
    if (!initShadowMap()) return -1;

    initPostProcessing(SCR_WIDTH, SCR_HEIGHT);

    skybox.Load(
    "resources/embedded_assets/skybox/right.png",
    "resources/embedded_assets/skybox/left.png",
    "resources/embedded_assets/skybox/top.png",
    "resources/embedded_assets/skybox/bottom.png",
    "resources/embedded_assets/skybox/front.png",
    "resources/embedded_assets/skybox/back.png"
);

    std::vector<std::string> faces = {
    "resources/embedded_assets/skybox/right.png",
    "resources/embedded_assets/skybox/left.png",
    "resources/embedded_assets/skybox/top.png",
    "resources/embedded_assets/skybox/bottom.png",
    "resources/embedded_assets/skybox/front.png",
    "resources/embedded_assets/skybox/back.png"
};
unsigned int envCubemap = LoadCubemap(faces);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        g_EditorUI.HandleShortcuts();

        auto& settings = g_EditorUI.GetSettings();

        static unsigned int lastShadowMapSize = SHADOW_WIDTH;
        if (settings.shadowMapSize != lastShadowMapSize) {
            lastShadowMapSize = settings.shadowMapSize;
            SHADOW_WIDTH = settings.shadowMapSize;
            SHADOW_HEIGHT = settings.shadowMapSize;
            glDeleteFramebuffers(1, &depthMapFBO);
            glDeleteTextures(1, &depthMap);
            initShadowMap();
        }

        g_SceneManager.UpdatePhysics(deltaTime);

        auto activeCamera = g_SceneManager.GetActiveCamera();
        if (!activeCamera) {
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        float aspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;
        glm::mat4 projection = activeCamera->GetCameraProjectionMatrix(aspect);
        glm::mat4 view = activeCamera->GetCameraViewMatrix();

        Frustum frustum;
bool useCulling = activeCamera->GetFrustumCulling();
if (useCulling) {
    glm::mat4 viewProj = projection * view;
    frustum.Update(viewProj);
    // ========== АУДИО: обновляем слушателя ==========
    glm::vec3 camPos = activeCamera->GetWorldPosition();
    // Вектор forward – третья колонка матрицы трансформации (ось Z)
    glm::vec3 camForward = glm::normalize(glm::vec3(activeCamera->GetTransformMatrix()[2]));
    glm::vec3 camUp = glm::normalize(glm::vec3(activeCamera->GetTransformMatrix()[1]));
    AudioEngine::Get().Update(camPos, camForward, camUp);
}

        // --- Находим направленный свет для карты теней ---
        glm::vec3 directionalLightPos(2.0f, 4.0f, 2.0f);
        glm::vec3 directionalLightDir = glm::vec3(-1.0f, -1.0f, 0.0f);
        for (const auto& obj : g_SceneManager.GetObjects()) {
            if (obj->GetLightType() == LT_DIRECTIONAL) {
                directionalLightPos = obj->GetWorldPosition();
                directionalLightDir = obj->GetLightDirection();
                break;
            }
        }

        glm::mat4 lightSpaceMatrix = calculateLightSpaceMatrix(directionalLightDir, view,
                                                       directionalLightPos,
                                                       settings.shadowNearQualityRadius,
                                                       settings.shadowFarClip);

        // --- Рендер карты теней ---
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        depthShader.Use();
        depthShader.SetMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
        g_SceneManager.RenderDepth(depthShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --- Рендер сцены в текстуру (FBO) ---
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClearColor(settings.bg_color[0], settings.bg_color[1], settings.bg_color[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Скайбокс
        glDepthMask(GL_FALSE);
skyboxShader.Use();
skyboxShader.SetInt("skybox", 0);   // <--- добавьте эту строку
glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));
skyboxShader.SetMat4("view", glm::value_ptr(viewNoTranslation));
skyboxShader.SetMat4("projection", glm::value_ptr(projection));
skybox.Draw();
glDepthMask(GL_TRUE);

        // Сетка
        if (settings.grid_enabled) {
            gridShader.Use();
            gridShader.SetMat4("view", glm::value_ptr(view));
            gridShader.SetMat4("projection", glm::value_ptr(projection));
            gridShader.SetVec3("viewPos", activeCamera->GetWorldPosition().x,
                                         activeCamera->GetWorldPosition().y,
                                         activeCamera->GetWorldPosition().z);
            g_SceneManager.RenderGrid(gridShader, view, projection);
        }

        // Основные объекты
        shader.Use();
        shader.SetMat4("projection", glm::value_ptr(projection));
        shader.SetMat4("view", glm::value_ptr(view));
        shader.SetVec3("viewPos", activeCamera->GetWorldPosition().x,
                                 activeCamera->GetWorldPosition().y,
                                 activeCamera->GetWorldPosition().z);
        shader.SetFloat("ambientStrength", settings.ambientStrength);
        shader.SetBool("shadowsEnabled", settings.shadows_enabled);
        shader.SetFloat("shadowBias", settings.shadow_bias);
        shader.SetFloat("shadowSoftness", settings.shadowSoftness);
        shader.SetInt("shadowSamples", settings.shadowSamples);
        shader.SetMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        shader.SetInt("shadowMap", 2);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        shader.SetInt("environmentMap", 6);

        // Передача источников света
        struct LightUniform {
            int type;
            glm::vec3 position;
            glm::vec3 direction;
            glm::vec3 color;
            float intensity;
            float range;
            float angle;
        };
        std::vector<LightUniform> lightUniforms;
        for (const auto& obj : g_SceneManager.GetObjects()) {
            int type = obj->GetLightType();
            if (type == LT_NONE) continue;
            LightUniform lu;
            lu.type = type;
            lu.color = obj->GetLightColor();
            lu.intensity = obj->GetLightIntensity();
            if (type == LT_DIRECTIONAL) {
                lu.direction = obj->GetLightDirection();
                lu.position = glm::vec3(0.0f);
                lu.range = 0.0f;
                lu.angle = 0.0f;
            } else if (type == LT_POINT) {
                lu.position = obj->GetWorldPosition();
                lu.range = obj->GetLightRange();
                lu.direction = glm::vec3(0.0f);
                lu.angle = 0.0f;
            } else if (type == LT_SPOT) {
                lu.position = obj->GetWorldPosition();
                lu.direction = obj->GetLightDirection();
                lu.range = obj->GetLightRange();
                lu.angle = obj->GetLightAngleDeg() * 3.14159265f / 180.0f;
            }
            lightUniforms.push_back(lu);
            if (lightUniforms.size() >= 8) break;
        }
        shader.SetInt("numLights", (int)lightUniforms.size());
        for (size_t i = 0; i < lightUniforms.size(); ++i) {
            std::string prefix = "lights[" + std::to_string(i) + "].";
            shader.SetInt(prefix + "type", lightUniforms[i].type);
            shader.SetVec3(prefix + "position", lightUniforms[i].position.x, lightUniforms[i].position.y, lightUniforms[i].position.z);
            shader.SetVec3(prefix + "direction", lightUniforms[i].direction.x, lightUniforms[i].direction.y, lightUniforms[i].direction.z);
            shader.SetVec3(prefix + "color", lightUniforms[i].color.x, lightUniforms[i].color.y, lightUniforms[i].color.z);
            shader.SetFloat(prefix + "intensity", lightUniforms[i].intensity);
            shader.SetFloat(prefix + "range", lightUniforms[i].range);
            shader.SetFloat(prefix + "angle", lightUniforms[i].angle);
        }

if (settings.wireframe_mode)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
else
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

if (useCulling)
    g_SceneManager.RenderWithCulling(shader, frustum);
else
    g_SceneManager.Render(shader);

    g_SceneManager.RenderLightShafts(view, projection);

    gizmoShader.Use();
gizmoShader.SetMat4("view", glm::value_ptr(view));
gizmoShader.SetMat4("projection", glm::value_ptr(projection));
gizmoShader.SetVec3("color", 1.0f, 1.0f, 1.0f);
g_SceneManager.RenderFrustumGizmos(gizmoShader, view, projection, activeCamera.get());

// Рендер гизмо для Spot Light (белый конус)
gizmoShader.Use();
gizmoShader.SetMat4("view", glm::value_ptr(view));
gizmoShader.SetMat4("projection", glm::value_ptr(projection));
gizmoShader.SetVec3("color", 1.0f, 1.0f, 1.0f);
g_SceneManager.RenderLightGizmos(gizmoShader, view, projection);
// Аудио гизмо (дальность звука)
gizmoShader.Use();
g_SceneManager.RenderAudioGizmos(gizmoShader, view, projection);
// Коллайдеры
gizmoShader.Use();
g_SceneManager.RenderColliderGizmos(gizmoShader, view, projection);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // --- Пост-эффект тумана ---
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_DEPTH_BUFFER_BIT); // очищаем только глубину

        screenFogShader.Use();
        screenFogShader.SetInt("sceneTexture", 0);
        screenFogShader.SetInt("depthTexture", 1);
        screenFogShader.SetMat4("invProjection", glm::value_ptr(glm::inverse(projection)));
        screenFogShader.SetMat4("invView", glm::value_ptr(glm::inverse(view)));
        screenFogShader.SetVec3("viewPos", activeCamera->GetWorldPosition().x,
                                            activeCamera->GetWorldPosition().y,
                                            activeCamera->GetWorldPosition().z);

        // Параметры тумана из SceneManager
auto fogObj = g_SceneManager.GetActiveFog();
if (fogObj && fogObj->GetFogEnabled()) {
    screenFogShader.SetBool("fogEnabled", true);
    screenFogShader.SetVec3("fogColor", fogObj->GetFogColor().x, fogObj->GetFogColor().y, fogObj->GetFogColor().z);
    screenFogShader.SetInt("fogType", fogObj->GetFogType());
    screenFogShader.SetFloat("fogDensity", fogObj->GetFogDensity());
    screenFogShader.SetFloat("fogStart", fogObj->GetFogLinearStart());
    screenFogShader.SetFloat("fogEnd", fogObj->GetFogLinearEnd());
} else {
    screenFogShader.SetBool("fogEnabled", false);
}

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        renderFullScreenQuad();

        // --- ImGui ---
        g_EditorUI.SetViewProjection(view, projection);
        g_EditorUI.BeginFrame();
        g_EditorUI.Render();
        g_EditorUI.EndFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    g_EditorUI.Shutdown();
    AudioEngine::Get().Shutdown();
    PhysicsWorld::GetInstance().Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "Binax Engine shutdown successfully." << std::endl;
    return 0;
}

// ========== ФУНКЦИИ УПРАВЛЕНИЯ ==========
void processInput(GLFWwindow* window) {
    if (!mouseCaptured) return;
    if (g_EditorUI.IsGizmoActive()) return;

    float speed = 5.0f * deltaTime;
    float forward = 0.0f, right = 0.0f, up = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) forward -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) forward += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) right += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) right -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) up += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) up -= 1.0f;

    g_SceneManager.MoveActiveCamera(forward, right, up, speed);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!mouseCaptured) return;
    if (g_EditorUI.IsGizmoActive()) return;

    static float lastX = SCR_WIDTH / 2.0f;
    static float lastY = SCR_HEIGHT / 2.0f;
    static bool firstMouse = true;

    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
        return;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;

    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    g_SceneManager.RotateActiveCamera(-xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto activeCam = g_SceneManager.GetActiveCamera();
    if (activeCam) {
        float fov = activeCam->GetCameraFOV();
        fov -= (float)yoffset;
        if (fov < 1.0f) fov = 1.0f;
        if (fov > 120.0f) fov = 120.0f;
        activeCam->SetCameraFOV(fov);
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        mouseCaptured = false;
        firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    if (key == GLFW_KEY_DELETE && action == GLFW_PRESS) {
        auto selected = g_SceneManager.GetSelectedObject();
        if (selected) g_SceneManager.DeleteGameObject(selected.get());
    }
}

// ========== ИНИЦИАЛИЗАЦИЯ ШЕЙДЕРОВ ==========
bool initShaders() {
    std::cout << "Loading shaders..." << std::endl;
    bool shadersLoaded = shader.Load("assets/shaders/basic.vert", "assets/shaders/basic.frag");
    bool gridLoaded = gridShader.Load("assets/shaders/grid.vert", "assets/shaders/grid.frag");
    bool gizmoLoaded = gizmoShader.Load("assets/shaders/gizmo.vert", "assets/shaders/gizmo.frag");
    bool skyboxLoaded = skyboxShader.Load("assets/shaders/skybox.vert", "assets/shaders/skybox.frag");
    bool depthLoaded = depthShader.Load("assets/shaders/depth.vert", "assets/shaders/depth.frag");
    bool fogLoaded = screenFogShader.Load("assets/shaders/screen.vert", "assets/shaders/screen_fog.frag");
    
    if (!shadersLoaded || !gridLoaded || !gizmoLoaded || !skyboxLoaded || !depthLoaded || !fogLoaded) {
        std::cerr << "ERROR: Failed to load shaders!" << std::endl;
        return false;
    }
    std::cout << "All shaders loaded successfully!" << std::endl;
    return true;
}

bool initShadowMap() {
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer not complete!" << std::endl;
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

glm::mat4 calculateLightSpaceMatrix(const glm::vec3& lightDir, const glm::mat4& viewMatrix,
                                    const glm::vec3& /*lightPos*/, float nearRadius, float farClip) {
    glm::mat4 invView = glm::inverse(viewMatrix);
    glm::vec3 cameraPos = glm::vec3(invView[3]);

    // Расстояние от камеры до центра сцены (0,0,0) – можно заменить на любую другую точку
    float distToCenter = glm::length(cameraPos);
    
    // Выбираем радиус: если камера ближе nearRadius к центру – качественные тени, иначе – дальние (плохие)
    float currentRadius = (distToCenter < nearRadius) ? nearRadius : farClip;
    
    glm::vec3 center = cameraPos + lightDir * (currentRadius * 0.3f);
    glm::vec3 lightUp = glm::vec3(0.0f, 1.0f, 0.0f);
    if (fabs(glm::dot(lightDir, lightUp)) > 0.999f) lightUp = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::mat4 lightView = glm::lookAt(center - lightDir * currentRadius, center, lightUp);
    
    float half = currentRadius * 0.5f;
    glm::mat4 lightProjection = glm::ortho(-half, half, -half, half, 1.0f, farClip);
    return lightProjection * lightView;
}
