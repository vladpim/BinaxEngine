#include "Scene/SceneManager.h"
#include "Graphics/Primitives.h"
#include "Graphics/Material.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Physics/PhysicsWorld.h"
#include "Graphics/Shader.h"
#include <glm/gtx/quaternion.hpp>
#include <nlohmann/json.hpp>
#include <GLFW/glfw3.h>
#include "Editor/EditorUI.h"

extern EditorUI g_EditorUI;

void SceneManager::SaveScene(const std::string& filename) {
    nlohmann::json sceneJson;
    sceneJson["version"] = 1;
    nlohmann::json objectsJson = nlohmann::json::array();
    for (const auto& obj : m_Objects) {
        auto objJson = obj->ToJson();
        // Находим индекс родителя
        int parentIdx = -1;
        if (auto parent = obj->GetParent()) {
            auto it = std::find(m_Objects.begin(), m_Objects.end(), parent);
            if (it != m_Objects.end()) {
                parentIdx = static_cast<int>(std::distance(m_Objects.begin(), it));
            }
        }
        objJson["parentIndex"] = parentIdx;
        objectsJson.push_back(objJson);
    }
    sceneJson["objects"] = objectsJson;
    // Активная камера
    int activeCamIdx = -1;
    if (m_ActiveCamera) {
        auto it = std::find(m_Objects.begin(), m_Objects.end(), m_ActiveCamera);
        if (it != m_Objects.end()) {
            activeCamIdx = static_cast<int>(std::distance(m_Objects.begin(), it));
        }
    }
    sceneJson["activeCameraIndex"] = activeCamIdx;
    sceneJson["editorSettings"] = g_EditorUI.SettingsToJson();
    // Сохраняем в файл
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to save scene to " << filename << std::endl;
        return;
    }
    file << sceneJson.dump(4);
    std::cout << "Scene saved to " << filename << std::endl;
}

void SceneManager::LoadScene(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to load scene from " << filename << std::endl;
        return;
    }

    nlohmann::json sceneJson;
    file >> sceneJson;

    // Очищаем текущую сцену
    // Сначала удаляем физику, чтобы избежать утечек
    for (auto& obj : m_Objects) {
        if (obj->HasRigidBody()) {
            obj->RemoveRigidBody();
        }
    }
    m_Objects.clear();
    m_SelectedObject.reset();
    m_ActiveCamera.reset();

    // Временный вектор для хранения индексов родителей
    std::vector<int> parentIndices;
    if (sceneJson.contains("editorSettings")) {
    g_EditorUI.SettingsFromJson(sceneJson["editorSettings"]);
    // Применить настройки, требующие немедленного действия
    glfwSwapInterval(g_EditorUI.GetSettings().vsync ? 1 : 0);
    if (g_EditorUI.GetSettings().skyboxSeamless)
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    else
        glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

    // Загружаем объекты
    if (sceneJson.contains("objects")) {
        for (const auto& objJson : sceneJson["objects"]) {
            auto obj = std::make_shared<GameObject>("");
            if (obj->FromJson(objJson)) {
                m_Objects.push_back(obj);
                int parentIdx = objJson.value("parentIndex", -1);
                parentIndices.push_back(parentIdx);
            }
        }
    }

    // Восстанавливаем иерархию
    for (size_t i = 0; i < m_Objects.size(); ++i) {
        int parentIdx = parentIndices[i];
        if (parentIdx >= 0 && parentIdx < static_cast<int>(m_Objects.size())) {
            m_Objects[i]->SetParent(m_Objects[parentIdx], true);
        }
    }

    // Восстанавливаем активную камеру
    if (sceneJson.contains("activeCameraIndex")) {
        int idx = sceneJson["activeCameraIndex"];
        if (idx >= 0 && idx < static_cast<int>(m_Objects.size())) {
            SetActiveCamera(m_Objects[idx]);
        }
    }

    // Обновляем физику для объектов с коллайдерами
    for (auto& obj : m_Objects) {
        if (obj->GetColliderType() != COLLIDER_NONE) {
            obj->UpdatePhysicsBody();
        }
    }

    std::cout << "Scene loaded from " << filename << std::endl;
}

SceneManager::SceneManager() {
    m_Initialized = false;
}

SceneManager::~SceneManager() {
    m_Objects.clear();
    m_SelectedObject.reset();
}

void SceneManager::Initialize() {
    
    if (m_Initialized) return;
    std::cout << "Initializing SceneManager..." << std::endl;

    m_GridMesh = Primitives::CreateGridLines(500, 1.0f);

    
    m_ShaftShader.Load("assets/shaders/shaft.vert", "assets/shaders/shaft.frag");

    auto light = CreateGameObject("DirectionalLight");
    light->SetLightType(LT_DIRECTIONAL);        
    light->SetLightColor(glm::vec3(1.0f, 1.0f, 1.0f));
    light->SetLightIntensity(1.0f);
    light->SetLightDirection(glm::vec3(-1.0f, -1.0f, 0.0f)); // направление света
    light->SetPosition(glm::vec3(2.0f, 4.0f, 2.0f)); // позиция не важна для directional, но для визуализации ок
    light->SetColor(glm::vec3(1.0f, 1.0f, 1.0f));
    light->SetScale(glm::vec3(0.3f));
    light->SetMesh(Primitives::CreateCube());
    light->SetLightDirection(glm::normalize(glm::vec3(-1.0f, -2.0f, -1.0f)));

    SetSelectedObject(light);
    m_Initialized = true;
    std::cout << "SceneManager initialized with " << m_Objects.size() << " objects" << std::endl;

    auto mainCamera = CreateGameObject("Scene Camera");
    mainCamera->SetIsCamera(true);
    mainCamera->SetPosition(glm::vec3(0.0f, 2.0f, 5.0f));
    mainCamera->SetRotation(glm::vec3(0.0f, -90.0f, 0.0f)); // важно: yaw = -90, pitch = 0
    mainCamera->SetCameraFOV(45.0f);
    mainCamera->SetCameraNear(0.1f);
    mainCamera->SetCameraFar(100.0f);
    SetActiveCamera(mainCamera);

    m_Initialized = true;
}

void SceneManager::SetSelectedObject(std::shared_ptr<GameObject> object) {
    m_SelectedObject = object;
}

std::shared_ptr<GameObject> SceneManager::CreateGameObject(const std::string& name) {
    auto obj = std::make_shared<GameObject>(name);
    m_Objects.push_back(obj);
    return obj;
}

void SceneManager::DeleteGameObject(GameObject* object) {
    if (!object) return;

    // Если удаляемый объект является выбранным, сбрасываем выделение
    if (m_SelectedObject.get() == object) {
        m_SelectedObject.reset();
    }

    // Если объект имеет родителя, открепляем его
    if (object->GetParent()) {
        object->GetParent()->RemoveChild(object);
    }

    // Рекурсивно удаляем всех детей (они уже будут удалены из списков родителей)
    for (auto& child : object->GetChildren()) {
        DeleteGameObject(child.get());
    }

    // Удаляем из общего списка
    auto it = std::find_if(m_Objects.begin(), m_Objects.end(),
        [object](const std::shared_ptr<GameObject>& ptr) { return ptr.get() == object; });
    if (it != m_Objects.end()) {
        m_Objects.erase(it);
    }
}

void SceneManager::DuplicateSelectedObject() {
    if (!m_SelectedObject) return;
    // Запрещаем дублировать DirectionalLight (можно и другие типы разрешить)
    if (m_SelectedObject->GetName() == "DirectionalLight") {
        std::cerr << "Cannot duplicate Directional Light" << std::endl;
        return;
    }
    auto newObj = CreateGameObject(m_SelectedObject->GetName() + " (Copy)");
    newObj->SetMesh(m_SelectedObject->GetMesh());
    newObj->SetPosition(m_SelectedObject->GetPosition() + glm::vec3(1.0f, 0.0f, 0.0f));
    newObj->SetRotation(m_SelectedObject->GetRotation());
    newObj->SetScale(m_SelectedObject->GetScale());
    newObj->SetColor(m_SelectedObject->GetColor());
    if (m_SelectedObject->GetMaterial()) {
        newObj->SetMaterial(m_SelectedObject->GetMaterial());
    }
    // Копируем параметры света
    newObj->SetLightType(m_SelectedObject->GetLightType());
    newObj->SetLightColor(m_SelectedObject->GetLightColor());
    newObj->SetLightIntensity(m_SelectedObject->GetLightIntensity());
    newObj->SetLightRange(m_SelectedObject->GetLightRange());
    newObj->SetLightAngle(m_SelectedObject->GetLightAngleDeg());
    newObj->SetLightDirection(m_SelectedObject->GetLightDirection());

    SetSelectedObject(newObj);
}

void SceneManager::Update(float deltaTime) {}

void SceneManager::Render(Shader& shader) {
    if (!m_Initialized) return;
    for (const auto& obj : m_Objects) {
        if (obj->IsVisible()) {
            obj->Draw(shader);
        }
    }
}

void SceneManager::RenderDepth(Shader& depthShader) {
    if (!m_Initialized) return;
    depthShader.Use();

    for (const auto& obj : m_Objects) {
        if (!obj->IsVisible() || !obj->GetMesh() || !obj->CastShadows()) continue;

        auto mat = obj->GetMaterial() ? obj->GetMaterial() : (obj->GetMesh()->GetMaterial());
        bool alphaTest = false;
        if (mat && mat->transparent && mat->alphaTestShadows && mat->HasDiffuse()) {
            alphaTest = true;
            depthShader.SetBool("alphaTestShadows", true);
            depthShader.SetBool("hasDiffuseTexture", true);
            depthShader.SetFloat("alphaCutoff", mat->alphaCutoff);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mat->GetDiffuseTextureID());
            depthShader.SetInt("diffuseTexture", 0);
        } else {
            depthShader.SetBool("alphaTestShadows", false);
            depthShader.SetBool("hasDiffuseTexture", false);
        }

        depthShader.SetMat4("model", glm::value_ptr(obj->GetTransformMatrix()));
        obj->GetMesh()->Draw();
    }
}

bool SceneManager::HasDirectionalLight() const {
    for (const auto& obj : m_Objects) {
        if (obj->GetName() == "DirectionalLight") {
            return true;
        }
    }
    return false;
}

void SceneManager::SetActiveCamera(std::shared_ptr<GameObject> camera) {
    if (camera && camera->IsCamera()) {
        m_ActiveCamera = camera;
        // Извлекаем текущий поворот камеры для управления от первого лица
        glm::vec3 rot = m_ActiveCamera->GetRotation();
        m_CameraYaw = rot.y;
        m_CameraPitch = rot.x;
        // Убедимся, что углы в допустимых пределах
        if (m_CameraPitch > 89.0f) m_CameraPitch = 89.0f;
        if (m_CameraPitch < -89.0f) m_CameraPitch = -89.0f;
    }
}

void SceneManager::MoveActiveCamera(float forwardBack, float leftRight, float upDown, float speed) {
    if (!m_ActiveCamera) return;

    // Получаем матрицу поворота камеры (без переноса)
    glm::mat4 transform = m_ActiveCamera->GetTransformMatrix();
    glm::vec3 forward = glm::normalize(glm::vec3(transform[2]));   // ось Z (куда смотрит камера)
    glm::vec3 right   = glm::normalize(glm::vec3(transform[0]));   // ось X
    glm::vec3 up      = glm::normalize(glm::vec3(transform[1]));   // ось Y
    glm::vec3 move = forward * forwardBack + right * leftRight + up * upDown;
    glm::vec3 newPos = m_ActiveCamera->GetPosition() + move * speed;
    m_ActiveCamera->SetPosition(newPos);
}

void SceneManager::RotateActiveCamera(float yawDelta, float pitchDelta) {
    if (!m_ActiveCamera) return;

    // Получаем текущий поворот в углах Эйлера (в градусах)
    glm::vec3 rot = m_ActiveCamera->GetRotation();
    float yaw = rot.y;
    float pitch = rot.x;

    // Применяем дельты (знаки уже скорректированы в mouse_callback)
    yaw += yawDelta;
    pitch += pitchDelta;

    // Ограничения
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Устанавливаем новый поворот
    m_ActiveCamera->SetRotation(glm::vec3(pitch, yaw, 0.0f));

    // Синхронизируем сохранённые углы для других нужд (если нужно)
    m_CameraYaw = yaw;
    m_CameraPitch = pitch;
}

void SceneManager::RenderGrid(Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
    if (!m_GridMesh) return;
    shader.Use();
    shader.SetMat4("view", glm::value_ptr(view));
    shader.SetMat4("projection", glm::value_ptr(projection));
    // Получаем позицию камеры из матрицы view (обратная матрица)
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 viewPos = glm::vec3(invView[3]);
    shader.SetVec3("viewPos", viewPos.x, viewPos.y, viewPos.z);   // <--- ДОБАВИТЬ
    glm::mat4 model = glm::mat4(1.0f);
    shader.SetMat4("model", glm::value_ptr(model));
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_GridMesh->Draw();
    glDisable(GL_BLEND);
}

void SceneManager::InitializePhysics() {
    PhysicsWorld::GetInstance().Initialize();
    for (auto& obj : m_Objects) {
        if (obj->GetColliderType() != COLLIDER_NONE) {
            obj->UpdatePhysicsBody();
            obj->SyncPhysicsToTransform();   // обязательно
        }
    }
}

void SceneManager::UpdatePhysics(float deltaTime) {
    PhysicsWorld::GetInstance().Update(deltaTime);
    for (auto& obj : m_Objects) {
        if (obj->HasRigidBody()) {
            obj->SyncTransformToPhysics();
            // Обновляем аудиопозицию после синхронизации трансформации
            if (!obj->GetAudioClipPath().empty() && obj->GetAudioSpatial()) {
                obj->UpdateAudioPosition(obj->GetWorldPosition());
            }
        }
    }
}

void SceneManager::SetPhysicsActive(bool active) {
    std::cout << "SceneManager::SetPhysicsActive(" << active << ")" << std::endl;
    PhysicsWorld::GetInstance().SetSimulationActive(active);
}

std::shared_ptr<GameObject> SceneManager::FindGameObjectByPtr(GameObject* ptr) {
    for (auto& obj : m_Objects) {
        if (obj.get() == ptr) return obj;
    }
    return nullptr;
}

std::shared_ptr<GameObject> SceneManager::GetActiveFog() const {
    for (const auto& obj : m_Objects) {
        if (obj->IsFog()) return obj;
    }
    return nullptr;
}

void SceneManager::SetFrustumCullingForActiveCamera(bool enabled) {
    if (m_ActiveCamera)
        m_ActiveCamera->SetFrustumCulling(enabled);
}

void SceneManager::RenderWithCulling(Shader& shader, const Frustum& frustum) {
    if (!m_Initialized) return;
    for (const auto& obj : m_Objects) {
        if (!obj->IsVisible()) continue;
        // Если объект — камера, всегда рисуем? нет, камера не рендерится
        if (obj->IsCamera()) continue;
        if (m_ActiveCamera && m_ActiveCamera->GetFrustumCulling()) {
            glm::vec3 minBB, maxBB;
            obj->CalculateAABB(minBB, maxBB);
            if (!frustum.IsAABBInside(minBB, maxBB))
                continue;
        }
        obj->Draw(shader);
    }
}

void SceneManager::ResetPhysics() {
    PhysicsWorld::GetInstance().ResetAllObjects();
}

void SceneManager::RegisterForPhysicsReset(GameObject* obj) {
    PhysicsWorld::GetInstance().RegisterGameObject(obj);
}

void SceneManager::RenderLightShafts(const glm::mat4& view, const glm::mat4& projection) {
    if (!m_Initialized) return;
    m_ShaftShader.Use();
    m_ShaftShader.SetMat4("view", glm::value_ptr(view));
    m_ShaftShader.SetMat4("projection", glm::value_ptr(projection));
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // аддитивное смешивание
    glDepthMask(GL_FALSE);
    
    for (const auto& obj : m_Objects) {
        if (obj->GetLightType() != LT_SPOT) continue;
        if (!obj->GetShaftEnabled()) continue;
        if (!obj->GetShaftMesh()) obj->UpdateShaftMesh();
        auto mesh = obj->GetShaftMesh();
        if (!mesh) continue;
        
        // Матрица модели: источник света (позиция), направление света
        glm::vec3 pos = obj->GetWorldPosition();
        glm::vec3 dir = obj->GetLightDirection();
        glm::vec3 defaultDir(0.0f, 0.0f, -1.0f);
        glm::quat rot = glm::rotation(defaultDir, glm::normalize(dir));
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
        model = model * glm::mat4_cast(rot);
        
        m_ShaftShader.SetMat4("model", glm::value_ptr(model));
        m_ShaftShader.SetFloat("intensity", obj->GetShaftIntensity());
        m_ShaftShader.SetFloat("softness", obj->GetShaftSoftness());
        m_ShaftShader.SetFloat("density", obj->GetShaftDensity());
        m_ShaftShader.SetVec3("lightColor", obj->GetLightColor().x, obj->GetLightColor().y, obj->GetLightColor().z);
        
        mesh->Draw();
    }
    
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void SceneManager::RenderFrustumGizmos(Shader& shader, const glm::mat4& view, const glm::mat4& projection,
                                       const GameObject* activeCamera) const {
    if (!m_Initialized) return;
    
    for (const auto& obj : m_Objects) {
        if (!obj->IsCamera()) continue;
        if (obj.get() == activeCamera) continue;
        if (!obj->GetShowFrustumGizmo()) continue;
        
        // Позиция и направление
        glm::vec3 pos = obj->GetWorldPosition();
        glm::vec3 forward = glm::normalize(glm::vec3(obj->GetTransformMatrix()[2]));
        glm::vec3 right = glm::normalize(glm::vec3(obj->GetTransformMatrix()[0]));
        glm::vec3 up = glm::normalize(glm::vec3(obj->GetTransformMatrix()[1]));
        
        // Длина "луча" гимо – фиксированная, например 1.5 метра
        float gizmoLength = 1.5f;
        glm::vec3 tipPos = pos + forward * gizmoLength;
        
        // Размер основания пирамидки (ширина и высота в конце луча)
        float baseSize = 0.25f;
        float halfBase = baseSize * 0.5f;
        
        // Точки основания (квадрат, перпендикулярный направлению)
        glm::vec3 baseCenter = tipPos; // можно сместить назад для усечённого конуса, но пусть будет на конце
        glm::vec3 baseUp = up;
        glm::vec3 baseRight = right;
        
        glm::vec3 v1 = baseCenter + baseUp * halfBase + baseRight * halfBase; // верх-право
        glm::vec3 v2 = baseCenter + baseUp * halfBase - baseRight * halfBase; // верх-лево
        glm::vec3 v3 = baseCenter - baseUp * halfBase - baseRight * halfBase; // низ-лево
        glm::vec3 v4 = baseCenter - baseUp * halfBase + baseRight * halfBase; // низ-право
        
        // Собираем линии: от основания к вершине (позиция камеры) и рёбра основания
        std::vector<glm::vec3> lines;
        // 4 линии от основания к вершине
        lines.push_back(v1); lines.push_back(pos);
        lines.push_back(v2); lines.push_back(pos);
        lines.push_back(v3); lines.push_back(pos);
        lines.push_back(v4); lines.push_back(pos);
        // 4 ребра основания
        lines.push_back(v1); lines.push_back(v2);
        lines.push_back(v2); lines.push_back(v3);
        lines.push_back(v3); lines.push_back(v4);
        lines.push_back(v4); lines.push_back(v1);
        
        // Рисуем линии
        static GLuint lineVAO = 0, lineVBO = 0;
        if (lineVAO == 0) {
            glGenVertexArrays(1, &lineVAO);
            glGenBuffers(1, &lineVBO);
            glBindVertexArray(lineVAO);
            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
            glEnableVertexAttribArray(0);
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(glm::vec3), lines.data(), GL_DYNAMIC_DRAW);
        
        shader.Use();
        shader.SetMat4("model", glm::value_ptr(glm::mat4(1.0f)));
        shader.SetMat4("view", glm::value_ptr(view));
        shader.SetMat4("projection", glm::value_ptr(projection));
        shader.SetVec3("color", 1.0f, 1.0f, 1.0f); // белый
        
        glLineWidth(2.0f); // чуть толще для видимости
        glBindVertexArray(lineVAO);
        glDrawArrays(GL_LINES, 0, (GLsizei)lines.size());
        glBindVertexArray(0);
        glLineWidth(1.0f); // вернуть обратно
    }
}

void SceneManager::RenderLightGizmos(Shader& shader, const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_Initialized) return;

    // ========== 1. ПРЕДРАСЧЁТ ГЕОМЕТРИИ (ОДИН РАЗ) ==========
    static std::vector<glm::vec3> circleXY, circleXZ, circleYZ;
    static std::vector<glm::vec3> coneCircle;    // для основания конуса
    static std::vector<glm::vec2> coneUnit;      // единичные точки для конуса (16 сегментов)
    static bool init = false;
    if (!init) {
        const int pointSegments = 32;   // для сферы (3 окружности)
        const int coneSegments = 16;    // для конуса

        // Единичные точки для сферы (в плоскостях)
        for (int i = 0; i <= pointSegments; ++i) {
            float angle = 2.0f * 3.14159265359f * i / pointSegments;
            float x = cosf(angle);
            float y = sinf(angle);
            circleXY.emplace_back(x, y, 0.0f);
            circleXZ.emplace_back(x, 0.0f, y);
            circleYZ.emplace_back(0.0f, x, y);
        }
        // Единичные точки для конуса (2D, для умножения на right/up)
        for (int i = 0; i <= coneSegments; ++i) {
            float angle = 2.0f * 3.14159265359f * i / coneSegments;
            coneUnit.emplace_back(cosf(angle), sinf(angle));
        }
        init = true;
    }

    // ========== 2. СТАТИЧЕСКИЙ VBO (САМОРАСШИРЯЮЩИЙСЯ) ==========
    static GLuint lineVAO = 0, lineVBO = 0;
    static size_t vboCapacity = 0;  // в байтах

    if (lineVAO == 0) {
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // Собираем все линии в один вектор
    std::vector<glm::vec3> allLines;
    allLines.reserve(20000);   // хватит для большинства сцен

    for (const auto& obj : m_Objects) {
        int lightType = obj->GetLightType();
        if (lightType == LT_NONE) continue;
        if (!obj->GetShowLightGizmo()) continue;

        if (lightType == LT_POINT) {
            glm::vec3 center = obj->GetWorldPosition();
            float radius = obj->GetLightRange();
            if (radius <= 0.0f) radius = 0.1f;

            // Вспомогательная лямбда для добавления готовой окружности
            auto addCircle = [&](const std::vector<glm::vec3>& unitCircle) {
                for (size_t i = 0; i < unitCircle.size() - 1; ++i) {
                    glm::vec3 p1 = center + unitCircle[i] * radius;
                    glm::vec3 p2 = center + unitCircle[i+1] * radius;
                    allLines.push_back(p1);
                    allLines.push_back(p2);
                }
            };
            addCircle(circleXY);
            addCircle(circleXZ);
            addCircle(circleYZ);
        }
        else if (lightType == LT_SPOT) {
            glm::vec3 pos = obj->GetWorldPosition();
            glm::vec3 dir = glm::normalize(obj->GetLightDirection());
            float range = obj->GetLightRange();
            float angleDeg = obj->GetLightAngleDeg();
            float halfAngleRad = glm::radians(angleDeg) * 0.5f;
            float radius = range * tanf(halfAngleRad);

            // Базис для ориентации конуса
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            if (fabs(glm::dot(dir, up)) > 0.999f) up = glm::vec3(1.0f, 0.0f, 0.0f);
            glm::vec3 right = glm::normalize(glm::cross(up, dir));
            up = glm::normalize(glm::cross(dir, right));

            glm::vec3 baseCenter = pos + dir * range;

            // Генерируем точки основания из готовых единичных точек
            std::vector<glm::vec3> circlePoints;
            circlePoints.reserve(coneUnit.size());
            for (const auto& uv : coneUnit) {
                glm::vec3 offset = right * uv.x + up * uv.y;
                circlePoints.push_back(baseCenter + offset * radius);
            }

            // Линии от вершины к каждой точке окружности
            for (const auto& pt : circlePoints) {
                allLines.push_back(pos);
                allLines.push_back(pt);
            }
            // Линии окружности
            for (size_t i = 0; i < circlePoints.size() - 1; ++i) {
                allLines.push_back(circlePoints[i]);
                allLines.push_back(circlePoints[i+1]);
            }
        }
    }

    if (allLines.empty()) return;

    // ========== 3. ОБНОВЛЕНИЕ БУФЕРА ПО ТВОЕЙ ФИШКЕ ==========
    size_t vertexCount = allLines.size();
    size_t bufferSize = vertexCount * sizeof(glm::vec3);

    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

    if (bufferSize > vboCapacity) {
        vboCapacity = bufferSize + 100000 * sizeof(glm::vec3);   // запас 100k вершин
        glBufferData(GL_ARRAY_BUFFER, vboCapacity, nullptr, GL_DYNAMIC_DRAW);
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, allLines.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // ========== 4. РИСУЕМ ==========
    shader.Use();
    shader.SetMat4("model", glm::value_ptr(glm::mat4(1.0f)));
    shader.SetMat4("view", glm::value_ptr(view));
    shader.SetMat4("projection", glm::value_ptr(projection));
    shader.SetVec3("color", 1.0f, 1.0f, 1.0f);

    glLineWidth(1.5f);
    glBindVertexArray(lineVAO);
    glDrawArrays(GL_LINES, 0, (GLsizei)vertexCount);
    glBindVertexArray(0);
    glLineWidth(1.0f);
}

void SceneManager::RenderAudioGizmos(Shader& shader, const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_Initialized) return;
    
    // Статические предрасчитанные окружности (один раз)
    static std::vector<glm::vec3> circleXY, circleXZ, circleYZ;
    static bool initGeometry = false;
    if (!initGeometry) {
        const int segments = 32;
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * 3.14159265359f * i / segments;
            float x = cosf(angle), y = sinf(angle);
            circleXY.emplace_back(x, y, 0.0f);
            circleXZ.emplace_back(x, 0.0f, y);
            circleYZ.emplace_back(0.0f, x, y);
        }
        initGeometry = true;
    }
    
    // Статический VAO / VBO (один на все вызовы)
    static GLuint audioVAO = 0, audioVBO = 0;
    static size_t vboCapacity = 0;
    if (audioVAO == 0) {
        glGenVertexArrays(1, &audioVAO);
        glGenBuffers(1, &audioVBO);
        glBindVertexArray(audioVAO);
        glBindBuffer(GL_ARRAY_BUFFER, audioVBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    
    // Собираем линии для всех объектов с Audio
    std::vector<glm::vec3> allLines;
    allLines.reserve(5000);
    
    for (const auto& obj : m_Objects) {
        if (!obj->GetShowAudioGizmo()) continue;
        if (obj->GetAudioClipPath().empty()) continue;
        float radius = obj->GetAudioMaxDistance();
        if (radius <= 0.0f) continue;
        glm::vec3 center = obj->GetWorldPosition();
        
        // Лямбда для добавления окружности
        auto addCircle = [&](const std::vector<glm::vec3>& unitCircle) {
            for (size_t i = 0; i < unitCircle.size() - 1; ++i) {
                glm::vec3 p1 = center + unitCircle[i] * radius;
                glm::vec3 p2 = center + unitCircle[i+1] * radius;
                allLines.push_back(p1);
                allLines.push_back(p2);
            }
        };
        addCircle(circleXY);
        addCircle(circleXZ);
        addCircle(circleYZ);
    }
    
    if (allLines.empty()) return;
    
    size_t vertexCount = allLines.size();
    size_t bufferSize = vertexCount * sizeof(glm::vec3);
    
    glBindBuffer(GL_ARRAY_BUFFER, audioVBO);
    if (bufferSize > vboCapacity) {
        vboCapacity = bufferSize + 10000 * sizeof(glm::vec3); // запас
        glBufferData(GL_ARRAY_BUFFER, vboCapacity, nullptr, GL_DYNAMIC_DRAW);
    }
    glBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, allLines.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    shader.Use();
    shader.SetMat4("model", glm::value_ptr(glm::mat4(1.0f)));
    shader.SetMat4("view", glm::value_ptr(view));
    shader.SetMat4("projection", glm::value_ptr(projection));
    shader.SetVec3("color", 1.0f, 1.0f, 1.0f); // белый
    
    glLineWidth(1.5f);
    glBindVertexArray(audioVAO);
    glDrawArrays(GL_LINES, 0, (GLsizei)vertexCount);
    glBindVertexArray(0);
    glLineWidth(1.0f);
}
