#include "Scene/GameObject.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "Physics/PhysicsWorld.h"
#include <btBulletDynamicsCommon.h>
#include "Audio/AudioEngine.h"
#include "Graphics/Primitives.h"
#include "Graphics/Model.h"
#include "Script/ScriptManager.h"

GameObject::GameObject(const std::string& name)
    : m_Name(name) {
}

GameObject::~GameObject() {
    // 1. Открепляемся от родителя, если он ещё существует
    if (auto parent = m_Parent.lock()) {
        parent->RemoveChild(this);
    }
    
    // 2. Удаляем физическое тело, если оно есть
    if (m_rigidBody) {
        PhysicsWorld::GetInstance().RemoveRigidBody(m_rigidBody);
        delete m_rigidBody->getMotionState();
        delete m_rigidBody;
        m_rigidBody = nullptr;
    }
    
    // 3. Удаляем коллизионную форму
    if (m_collisionShape) {
        delete m_collisionShape;
        m_collisionShape = nullptr;
    }
    
    // 4. Обнуляем родителя у детей (они всё равно будут удалены через shared_ptr, но для безопасности)
    for (auto& child : m_Children) {
        child->m_Parent.reset();
    }
}

void GameObject::AddChild(std::shared_ptr<GameObject> child) {
    if (!child) return;
    if (child.get() == this) return;
    if (child->m_Parent.lock().get() == this) return;

    // Если у child уже есть родитель, открепляем его
    if (auto oldParent = child->m_Parent.lock()) {
        oldParent->RemoveChild(child.get());
    }

    child->m_Parent = shared_from_this();
    m_Children.push_back(child);
}

void GameObject::RemoveChild(GameObject* child) {
    auto it = std::find_if(m_Children.begin(), m_Children.end(),
        [child](const std::shared_ptr<GameObject>& ptr) { return ptr.get() == child; });
    if (it != m_Children.end()) {
        (*it)->m_Parent.reset();
        m_Children.erase(it);
    }
}

glm::vec3 GameObject::GetWorldPosition() const {
    if (auto parent = m_Parent.lock()) {
        return parent->GetWorldPosition() + m_Position;
    }
    return m_Position;
}

glm::mat4 GameObject::GetTransformMatrix() const {
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, m_Position);
    glm::mat4 rotation = glm::eulerAngleYXZ(
        glm::radians(m_Rotation.y),
        glm::radians(m_Rotation.x),
        glm::radians(m_Rotation.z)
    );
    transform = transform * rotation;
    transform = glm::scale(transform, m_Scale);
    if (auto parent = m_Parent.lock()) {
        transform = parent->GetTransformMatrix() * transform;
    }
    return transform;
}

void GameObject::SetPosition(const glm::vec3& position) {
    m_Position = position;
}

void GameObject::SetRotation(const glm::vec3& rotation) {
    m_PreviousRotation = m_Rotation;
    m_Rotation = rotation;
    auto normalize = [](float angle) {
        while (angle > 180.0f) angle -= 360.0f;
        while (angle < -180.0f) angle += 360.0f;
        return angle;
    };
    m_Rotation.x = normalize(m_Rotation.x);
    m_Rotation.y = normalize(m_Rotation.y);
    m_Rotation.z = normalize(m_Rotation.z);
}

void GameObject::SetScale(const glm::vec3& scale) {
    m_Scale = scale;
    if (m_Scale.x == 0.0f) m_Scale.x = 0.001f;
    if (m_Scale.y == 0.0f) m_Scale.y = 0.001f;
    if (m_Scale.z == 0.0f) m_Scale.z = 0.001f;
    
    // Если есть коллайдер, пересоздаём его с новым масштабом
    if (m_colliderType != COLLIDER_NONE) {
        SetColliderType(m_colliderType);  // пересоздаст коллайдер и вызовет UpdatePhysicsBody
    }
}

void GameObject::Draw(Shader& shader) const {
    if (!m_Visible || !m_Mesh) return;

    glm::mat4 transform = GetTransformMatrix();
    shader.SetMat4("model", glm::value_ptr(transform));
    shader.SetBool("receiveShadows", m_ReceiveShadows);
    shader.SetVec3("objectColor", m_Color.x, m_Color.y, m_Color.z);

    // Получаем материал (один раз)
    auto mat = GetMaterial() ? GetMaterial() : (m_Mesh ? m_Mesh->GetMaterial() : nullptr);

    // Настройка прозрачности и альфа-теста
    if (mat && mat->transparent && mat->alpha < 1.0f) {
        if (mat->alphaTest) {
            glDisable(GL_BLEND);               // для альфа-теста (листва)
        } else {
            glEnable(GL_BLEND);                // для альфа-блендинга (стекло)
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        shader.SetFloat("alpha", mat->alpha);
        shader.SetBool("alphaTest", mat->alphaTest);
        shader.SetFloat("alphaCutoff", mat->alphaCutoff);
    } else {
        glDisable(GL_BLEND);
        shader.SetFloat("alpha", 1.0f);
        shader.SetBool("alphaTest", false);
    }

    // Передаём параметры материала в шейдер
    if (mat) {
        shader.SetBool("enableReflections", mat->enableReflections);
        shader.SetBool("hasDiffuseTexture", mat->HasDiffuse());
        shader.SetBool("hasNormalMap", mat->HasNormal());
        shader.SetFloat("metallic", mat->metallic);
        shader.SetFloat("roughness", mat->roughness);
        shader.SetVec2("uvScale", mat->uvScale.x, mat->uvScale.y);
        shader.SetFloat("normalStrength", mat->normalStrength);
        shader.SetFloat("aoStrength", mat->aoStrength);
        shader.SetFloat("roughnessStrength", mat->roughnessStrength);
        shader.SetBool("useWorldUV", mat->useWorldUV);
        shader.SetVec3("emissionColor", mat->emissionColor.x, mat->emissionColor.y, mat->emissionColor.z);
        shader.SetFloat("emissionIntensity", mat->emissionIntensity);

        mat->BindTextures();
        shader.SetInt("diffuseTexture", 0);
        shader.SetInt("normalMap", 1);
        shader.SetBool("hasRoughnessTexture", mat->HasRoughness());
        shader.SetBool("hasMetallicTexture", mat->HasMetallic());
        shader.SetBool("hasAOTexture", mat->HasAO());
        shader.SetInt("roughnessTexture", 3);
        shader.SetInt("metallicTexture", 4);
        shader.SetInt("aoTexture", 5);
    } else {
        shader.SetBool("enableReflections", false);
        shader.SetBool("hasDiffuseTexture", false);
        shader.SetBool("hasNormalMap", false);
        shader.SetVec3("emissionColor", 0.0f, 0.0f, 0.0f);
        shader.SetFloat("emissionIntensity", 0.0f);
    }

    // Рисуем меш
    m_Mesh->Draw();

    // Отключаем текстуры
    if (mat) {
        mat->UnbindTextures();
    }
}

glm::mat4 GameObject::GetCameraViewMatrix() const {
    glm::vec3 pos = GetWorldPosition();
    glm::mat4 transform = GetTransformMatrix();
    // В OpenGL камера смотрит в направлении -Z (по стандарту)
    glm::vec3 forward = -glm::normalize(glm::vec3(transform[2])); // ось Z с минусом
    glm::vec3 up = glm::normalize(glm::vec3(transform[1]));
    return glm::lookAt(pos, pos + forward, up);
}

glm::mat4 GameObject::GetCameraProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(m_CameraFOV), aspectRatio, m_CameraNear, m_CameraFar);
}

void GameObject::AddRigidBody(float mass) {
    if (!CanHavePhysics()) {
    std::cerr << "[Physics] Cannot add physics to '" << m_Name << "' because it has parent or children!" << std::endl;
    return;
}
    if (mass <= 0.0f) mass = 1.0f;
    m_mass = mass;
    UpdatePhysicsBody();   // создаст динамическое тело
    std::cout << "[Physics] " << m_Name << " became dynamic, mass=" << m_mass << std::endl;
}

void GameObject::RemoveRigidBody() {
    m_mass = 0.0f;          // статическое тело
    UpdatePhysicsBody();    // пересоздаст тело как статическое
    std::cout << "[Physics] " << m_Name << " became static" << std::endl;
}

void GameObject::SetColliderType(ColliderType type) {
    if (!CanHavePhysics()) {
        std::cerr << "[Physics] Cannot set collider on '" << m_Name << "' because it has parent or children!" << std::endl;
        return;
    }
    if (m_collisionShape) delete m_collisionShape;
    m_colliderType = type;

    // Создаем базовую форму в зависимости от типа
    btCollisionShape* childShape = nullptr;
    switch (type) {
        case COLLIDER_BOX:
            childShape = new btBoxShape(btVector3(m_ColliderSize.x, m_ColliderSize.y, m_ColliderSize.z));
            break;
        case COLLIDER_SPHERE:
            childShape = new btSphereShape(m_ColliderSize.x);
            break;
        case COLLIDER_CAPSULE:
            childShape = new btCapsuleShape(m_ColliderSize.x, m_ColliderSize.y);
            break;
        default:
            childShape = nullptr;
            break;
    }

    // Если есть смещение, оборачиваем в составную форму
    if (childShape) {
        if (glm::length(m_ColliderOffset) > 0.001f) {
            btCompoundShape* compound = new btCompoundShape();
            btTransform offsetTransform;
            offsetTransform.setIdentity();
            offsetTransform.setOrigin(btVector3(m_ColliderOffset.x, m_ColliderOffset.y, m_ColliderOffset.z));
            compound->addChildShape(offsetTransform, childShape);
            m_collisionShape = compound;
        } else {
            m_collisionShape = childShape;
        }
    } else {
        m_collisionShape = nullptr;
    }

    // Обновляем физическое тело
    UpdatePhysicsBody();
}

void GameObject::SetFriction(float friction) {
    m_friction = friction;
    if (m_rigidBody) {
        m_rigidBody->setFriction(m_friction);
    }
}

void GameObject::SetRestitution(float restitution) {
    m_restitution = restitution;
    if (m_rigidBody) {
        m_rigidBody->setRestitution(m_restitution);
    }
}

void GameObject::SetRollingFriction(float rollingFriction) {
    m_rollingFriction = rollingFriction;
    if (m_rigidBody) {
        m_rigidBody->setRollingFriction(m_rollingFriction);
    }
}

void GameObject::SetLinearDamping(float damping) {
    m_linearDamping = damping;
    if (m_rigidBody) {
        m_rigidBody->setDamping(m_linearDamping, m_angularDamping);
    }
}

void GameObject::SetAngularDamping(float damping) {
    m_angularDamping = damping;
    if (m_rigidBody) {
        m_rigidBody->setDamping(m_linearDamping, m_angularDamping);
    }
}

void GameObject::SyncTransformToPhysics() {
    if (!m_rigidBody) return;
    btTransform trans;
    m_rigidBody->getMotionState()->getWorldTransform(trans);
    btVector3 pos = trans.getOrigin();
    SetPosition(glm::vec3(pos.x(), pos.y(), pos.z()));
    std::cout << "SyncTransformToPhysics: y = " << pos.y() << std::endl;
}

void GameObject::SyncPhysicsToTransform() {
    if (!m_rigidBody) return;
    btTransform trans;
    trans.setIdentity();
    glm::vec3 pos = GetWorldPosition();
    trans.setOrigin(btVector3(pos.x, pos.y, pos.z));
    glm::vec3 rot = GetRotation();
    trans.setRotation(btQuaternion(glm::radians(rot.y), glm::radians(rot.x), glm::radians(rot.z)));
    m_rigidBody->getMotionState()->setWorldTransform(trans);
    m_rigidBody->setCenterOfMassTransform(trans);
}

void GameObject::UpdatePhysicsBody() {
    if (!CanHavePhysics()) {
    if (m_rigidBody) {
        PhysicsWorld::GetInstance().RemoveRigidBody(m_rigidBody);
        delete m_rigidBody->getMotionState();
        delete m_rigidBody;
        m_rigidBody = nullptr;
    }
    return;
}
    if (!m_collisionShape) {
        // Нет коллайдера — удаляем тело из мира, если оно было
        if (m_rigidBody) {
            PhysicsWorld::GetInstance().RemoveRigidBody(m_rigidBody);
            delete m_rigidBody->getMotionState();
            delete m_rigidBody;
            m_rigidBody = nullptr;
        }
        return;
    }

    // Удаляем существующее тело, если есть
    if (m_rigidBody) {
        PhysicsWorld::GetInstance().RemoveRigidBody(m_rigidBody);
        delete m_rigidBody->getMotionState();
        delete m_rigidBody;
        m_rigidBody = nullptr;
    }

    // Создаём новое тело (статическое или динамическое)
    btTransform startTransform;
    startTransform.setIdentity();
    glm::vec3 pos = GetWorldPosition();
    startTransform.setOrigin(btVector3(pos.x, pos.y, pos.z));
    glm::vec3 rot = GetRotation();
    startTransform.setRotation(btQuaternion(glm::radians(rot.y), glm::radians(rot.x), glm::radians(rot.z)));

    btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);
    btVector3 inertia(0,0,0);
    if (m_mass != 0.0f) {
        m_collisionShape->calculateLocalInertia(m_mass, inertia);
    }
    btRigidBody::btRigidBodyConstructionInfo rbInfo(m_mass, motionState, m_collisionShape, inertia);
    m_rigidBody = new btRigidBody(rbInfo);
    PhysicsWorld::GetInstance().AddRigidBody(m_rigidBody);
    
    std::cout << "[Physics] UpdatePhysicsBody for " << m_Name 
              << ", mass=" << m_mass 
              << ", collider=" << (m_collisionShape ? "yes" : "no") 
              << std::endl;

    m_rigidBody->setFriction(m_friction);
    m_rigidBody->setRestitution(m_restitution);
    m_rigidBody->setRollingFriction(m_rollingFriction);
    m_rigidBody->setDamping(m_linearDamping, m_angularDamping);     
    SyncPhysicsToTransform();     
}

void GameObject::SetParent(std::shared_ptr<GameObject> newParent, bool keepWorldPosition) {
    if (newParent.get() == this) return;
    if (newParent && newParent->m_Parent.lock().get() == this) return;

    glm::mat4 worldMat = GetTransformMatrix();

    // Открепляемся от старого родителя
    if (auto oldParent = m_Parent.lock()) {
        oldParent->RemoveChild(this);
        m_Parent.reset();
    }

    // Прикрепляемся к новому
    if (newParent) {
        m_Parent = newParent;
        newParent->m_Children.push_back(shared_from_this());
    }

    // Если нужно сохранить мировую позицию, пересчитываем локальную
    if (keepWorldPosition && newParent) {
        glm::mat4 parentInv = glm::inverse(newParent->GetTransformMatrix());
        glm::mat4 localMat = parentInv * worldMat;
        glm::vec3 scale, pos, skew;
        glm::quat rot;
        glm::vec4 persp;
        glm::decompose(localMat, scale, rot, pos, skew, persp);
        SetPosition(pos);
        SetRotation(glm::degrees(glm::eulerAngles(rot)));
        SetScale(scale);
    }
}

bool GameObject::CanHavePhysics() const {
    return m_Parent.lock() == nullptr && m_Children.empty();
}

void GameObject::CalculateAABB(glm::vec3& outMin, glm::vec3& outMax) const {
    if (!m_Mesh || m_Mesh->GetVertices().empty()) {
        // дефолтный box размером 1x1x1
        glm::mat4 m = GetTransformMatrix();
        glm::vec3 corners[8] = {
            glm::vec3(-0.5f,-0.5f,-0.5f), glm::vec3( 0.5f,-0.5f,-0.5f),
            glm::vec3( 0.5f, 0.5f,-0.5f), glm::vec3(-0.5f, 0.5f,-0.5f),
            glm::vec3(-0.5f,-0.5f, 0.5f), glm::vec3( 0.5f,-0.5f, 0.5f),
            glm::vec3( 0.5f, 0.5f, 0.5f), glm::vec3(-0.5f, 0.5f, 0.5f)
        };
        outMin = glm::vec3( FLT_MAX);
        outMax = glm::vec3(-FLT_MAX);
        for (int i = 0; i < 8; ++i) {
            glm::vec3 world = glm::vec3(m * glm::vec4(corners[i], 1.0f));
            // Ручное сравнение вместо glm::min / glm::max
            if (world.x < outMin.x) outMin.x = world.x;
            if (world.y < outMin.y) outMin.y = world.y;
            if (world.z < outMin.z) outMin.z = world.z;
            if (world.x > outMax.x) outMax.x = world.x;
            if (world.y > outMax.y) outMax.y = world.y;
            if (world.z > outMax.z) outMax.z = world.z;
        }
        return;
    }
    // Вычисляем AABB по вершинам меша
    const auto& vertices = m_Mesh->GetVertices();
    glm::vec3 localMin( FLT_MAX), localMax(-FLT_MAX);
    for (const auto& v : vertices) {
        glm::vec3 pos(v.Position[0], v.Position[1], v.Position[2]);
        if (pos.x < localMin.x) localMin.x = pos.x;
        if (pos.y < localMin.y) localMin.y = pos.y;
        if (pos.z < localMin.z) localMin.z = pos.z;
        if (pos.x > localMax.x) localMax.x = pos.x;
        if (pos.y > localMax.y) localMax.y = pos.y;
        if (pos.z > localMax.z) localMax.z = pos.z;
    }
    glm::mat4 m = GetTransformMatrix();
    outMin = glm::vec3( FLT_MAX);
    outMax = glm::vec3(-FLT_MAX);
    // 8 углов локального AABB
    glm::vec3 corners[8] = {
        localMin, glm::vec3(localMax.x, localMin.y, localMin.z),
        glm::vec3(localMax.x, localMax.y, localMin.z), glm::vec3(localMin.x, localMax.y, localMin.z),
        glm::vec3(localMin.x, localMin.y, localMax.z), glm::vec3(localMax.x, localMin.y, localMax.z),
        localMax, glm::vec3(localMin.x, localMax.y, localMax.z)
    };
    for (int i = 0; i < 8; ++i) {
        glm::vec3 world = glm::vec3(m * glm::vec4(corners[i], 1.0f));
        if (world.x < outMin.x) outMin.x = world.x;
        if (world.y < outMin.y) outMin.y = world.y;
        if (world.z < outMin.z) outMin.z = world.z;
        if (world.x > outMax.x) outMax.x = world.x;
        if (world.y > outMax.y) outMax.y = world.y;
        if (world.z > outMax.z) outMax.z = world.z;
    }
}

void GameObject::Unparent() {
    if (!m_Parent.lock()) return;

    // Сохраняем мировую позицию
    glm::mat4 worldMat = GetTransformMatrix();
    glm::vec3 scale, pos, skew;
    glm::quat rot;
    glm::vec4 persp;
    glm::decompose(worldMat, scale, rot, pos, skew, persp);

    // Открепляемся
    if (auto parent = m_Parent.lock()) {
        parent->RemoveChild(this);
    }
    m_Parent.reset();

    // Устанавливаем абсолютную трансформацию
    SetPosition(pos);
    SetRotation(glm::degrees(glm::eulerAngles(rot)));
    SetScale(scale);

    // Если была физика – удаляем
    if (!CanHavePhysics() && m_rigidBody) {
        RemoveRigidBody();
    }
}

void GameObject::SaveInitialTransform() {
    m_initialPosition = GetPosition();
    m_initialRotation = GetRotation();
    m_initialScale = GetScale();
}

void GameObject::ResetToInitialTransform() {
    SetPosition(m_initialPosition);
    SetRotation(m_initialRotation);
    SetScale(m_initialScale);
    if (m_rigidBody) {
        SyncPhysicsToTransform();
        m_rigidBody->setLinearVelocity(btVector3(0,0,0));
        m_rigidBody->setAngularVelocity(btVector3(0,0,0));
    }
}

void GameObject::SetShaftEnabled(bool enabled) {
    if (m_ShaftEnabled == enabled) return;
    m_ShaftEnabled = enabled;
    if (enabled) UpdateShaftMesh();
}
bool GameObject::GetShaftEnabled() const { return m_ShaftEnabled; }

void GameObject::SetShaftIntensity(float intensity) {
    m_ShaftIntensity = glm::clamp(intensity, 0.0f, 2.0f);
}
float GameObject::GetShaftIntensity() const { return m_ShaftIntensity; }

void GameObject::SetShaftSoftness(float softness) {
    m_ShaftSoftness = glm::clamp(softness, 0.2f, 2.0f);
}
float GameObject::GetShaftSoftness() const { return m_ShaftSoftness; }

void GameObject::UpdateShaftMesh() {
    if (GetLightType() != LT_SPOT) return;
    float range = GetLightRange();
    float angleDeg = GetLightAngleDeg();
    float halfAngleRad = glm::radians(angleDeg) * 0.5f;
    float radius = range * tanf(halfAngleRad);
    int segments = 32;
    
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    // Вершина конуса (в локальных координатах – начало луча)
    Vertex tip;
    tip.Position[0] = 0.0f;
    tip.Position[1] = 0.0f;
    tip.Position[2] = 0.0f;
    tip.Normal[0] = 0.0f; tip.Normal[1] = 0.0f; tip.Normal[2] = -1.0f;
    tip.TexCoords[0] = 0.5f; tip.TexCoords[1] = 1.0f;
    tip.Tangent[0] = 1.0f; tip.Tangent[1] = 0.0f; tip.Tangent[2] = 0.0f;
    vertices.push_back(tip);
    
    // Основание конуса – окружность в плоскости XY, на расстоянии -range по Z
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14159265359f * i / segments;
        float x = radius * cosf(angle);
        float y = radius * sinf(angle);
        float u = (float)i / segments;
        Vertex v;
        v.Position[0] = x;
        v.Position[1] = y;
        v.Position[2] = -range;
        v.Normal[0] = x / radius;
        v.Normal[1] = y / radius;
        v.Normal[2] = 0.0f;
        v.TexCoords[0] = u;
        v.TexCoords[1] = 0.0f;
        v.Tangent[0] = -sinf(angle);
        v.Tangent[1] = cosf(angle);
        v.Tangent[2] = 0.0f;
        vertices.push_back(v);
    }
    
    for (int i = 0; i < segments; ++i) {
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i + 2);
    }
    
    m_ShaftMesh = std::make_shared<Mesh>(vertices, indices);
}

void GameObject::SetLightRange(float range) {
    m_LightRange = range;
    if (m_ShaftEnabled) UpdateShaftMesh();
}

void GameObject::SetLightAngle(float angleDeg) {
    m_LightAngle = glm::radians(angleDeg);
    if (m_ShaftEnabled) UpdateShaftMesh();
}

void GameObject::SetShaftDensity(float density) {
    m_ShaftDensity = glm::clamp(density, 0.0f, 1.0f);
}
float GameObject::GetShaftDensity() const { return m_ShaftDensity; }

void GameObject::SetAudioClip(const std::string& path) {
    if (m_AudioID != 0) {
        AudioEngine::Get().UnloadSound(m_AudioID);
        m_AudioID = 0;
    }
    m_AudioClipPath = path;
    if (!path.empty()) {
        m_AudioID = AudioEngine::Get().LoadSound(path);
        if (m_AudioID == 0)
            std::cerr << "[GameObject] Failed to load audio: " << path << std::endl;
    }
}

void GameObject::PlayAudio(bool loop, float volume) {
    if (m_AudioID == 0 || m_AudioClipPath.empty()) return;
    // Останавливаем предыдущий звук, если он играет
    if (IsAudioPlaying()) StopAudio();
    if (m_AudioSpatial) {
        glm::vec3 pos = GetWorldPosition();
        AudioEngine::Get().Play3D(m_AudioID, pos, volume, loop, m_AudioMinDistance, m_AudioMaxDistance);
    } else {
        AudioEngine::Get().Play2D(m_AudioID, volume, loop);
    }
}

void GameObject::StopAudio() {
    if (m_AudioID) AudioEngine::Get().Stop(m_AudioID);
}

bool GameObject::IsAudioPlaying() const {
    return m_AudioID ? AudioEngine::Get().IsPlaying(m_AudioID) : false;
}

void GameObject::UpdateAudioPosition(const glm::vec3& pos) {
    if (m_AudioSpatial && m_AudioID && IsAudioPlaying()) {
        AudioEngine::Get().UpdateSoundPosition(m_AudioID, pos);
    }
}

void GameObject::UpdateAudioVolume(float vol) {
    m_AudioVolume = vol;
    if (m_AudioID && IsAudioPlaying()) {
        AudioEngine::Get().UpdateVolume(m_AudioID, vol);
    }
}

void GameObject::UpdateAudioMinDistance(float dist) {
    m_AudioMinDistance = dist;
    if (m_AudioID && IsAudioPlaying() && m_AudioSpatial) {
        AudioEngine::Get().UpdateMinDistance(m_AudioID, dist);
    }
}

void GameObject::UpdateAudioMaxDistance(float dist) {
    m_AudioMaxDistance = dist;
    if (m_AudioID && IsAudioPlaying() && m_AudioSpatial) {
        AudioEngine::Get().UpdateMaxDistance(m_AudioID, dist);
    }
}

void GameObject::UpdateAudioSpatial(bool spatial) {
    if (m_AudioSpatial == spatial) return;
    bool wasPlaying = IsAudioPlaying();
    if (wasPlaying) StopAudio();
    m_AudioSpatial = spatial;
    if (wasPlaying) {
        PlayAudio(m_AudioLoop, m_AudioVolume);
    }
}

void GameObject::EnableAudioSource() {
    if (m_AudioSourceEnabled) return;
    m_AudioSourceEnabled = true;
    // Можно создать пустую заглушку, но пока ничего не загружаем
}

void GameObject::DisableAudioSource() {
    if (!m_AudioSourceEnabled) return;
    if (m_AudioID != 0) {
        StopAudio();
        AudioEngine::Get().UnloadSound(m_AudioID);
        m_AudioID = 0;
    }
    m_AudioClipPath.clear();
    m_AudioVolume = 1.0f;
    m_AudioLoop = false;
    m_AudioSpatial = true;
    m_AudioMinDistance = 1.0f;
    m_AudioMaxDistance = 20.0f;
    m_AudioSourceEnabled = false;
}


// ========== Сериализация ==========
nlohmann::json GameObject::ToJson() const {
    nlohmann::json j;
    j["name"] = m_Name;
    j["position"] = { m_Position.x, m_Position.y, m_Position.z };
    j["rotation"] = { m_Rotation.x, m_Rotation.y, m_Rotation.z };
    j["scale"] = { m_Scale.x, m_Scale.y, m_Scale.z };
    j["visible"] = m_Visible;
    j["color"] = { m_Color.x, m_Color.y, m_Color.z };
    j["castShadows"] = m_CastShadows;
    j["receiveShadows"] = m_ReceiveShadows;

    // Свет
    j["lightType"] = m_LightType;
    if (m_LightType != LT_NONE) {
        j["lightColor"] = { m_LightColor.x, m_LightColor.y, m_LightColor.z };
        j["lightIntensity"] = m_LightIntensity;
        j["lightRange"] = m_LightRange;
        j["lightAngle"] = m_LightAngle;
        j["lightDirection"] = { m_LightDirection.x, m_LightDirection.y, m_LightDirection.z };
        j["shaftEnabled"] = m_ShaftEnabled;
        j["shaftIntensity"] = m_ShaftIntensity;
        j["shaftSoftness"] = m_ShaftSoftness;
        j["shaftDensity"] = m_ShaftDensity;
    }

    // Камера
    if (m_IsCamera) {
        j["isCamera"] = true;
        j["cameraFOV"] = m_CameraFOV;
        j["cameraNear"] = m_CameraNear;
        j["cameraFar"] = m_CameraFar;
        j["frustumCulling"] = m_FrustumCulling;
        j["showFrustumGizmo"] = m_ShowFrustumGizmo;
    }

    // Меш
    if (m_Mesh) {
        j["meshSourceType"] = m_MeshSourceType;
        if (m_MeshSourceType == "primitive") {
            j["meshPrimitive"] = m_MeshPrimitiveType;
        } else if (m_MeshSourceType == "model") {
            j["meshPath"] = m_MeshPath;
        }
    }

    // Материал (сохраняем все параметры)
    if (m_Material) {
        j["material"] = m_Material->ToJson();
    }

    // Физика
    j["colliderType"] = (int)m_colliderType;
    j["mass"] = m_mass;
    j["friction"] = m_friction;
    j["restitution"] = m_restitution;
    j["rollingFriction"] = m_rollingFriction;
    j["linearDamping"] = m_linearDamping;
    j["angularDamping"] = m_angularDamping;
    j["hasRigidBody"] = (m_rigidBody != nullptr);

    // Аудио
    if (!m_AudioClipPath.empty()) {
        j["audioClipPath"] = m_AudioClipPath;
        j["audioVolume"] = m_AudioVolume;
        j["audioLoop"] = m_AudioLoop;
        j["audioSpatial"] = m_AudioSpatial;
        j["audioMinDistance"] = m_AudioMinDistance;
        j["audioMaxDistance"] = m_AudioMaxDistance;
        j["audioSourceEnabled"] = m_AudioSourceEnabled;
        j["showAudioGizmo"] = m_ShowAudioGizmo;
    }

    // Туман
    if (m_IsFog) {
        j["isFog"] = true;
        j["fogEnabled"] = m_FogEnabled;
        j["fogType"] = m_FogType;
        j["fogColor"] = { m_FogColor.x, m_FogColor.y, m_FogColor.z };
        j["fogDensity"] = m_FogDensity;
        j["fogLinearStart"] = m_FogLinearStart;
        j["fogLinearEnd"] = m_FogLinearEnd;
    }

        // Коллайдер
    j["colliderOffset"] = { m_ColliderOffset.x, m_ColliderOffset.y, m_ColliderOffset.z };
    j["colliderSize"] = { m_ColliderSize.x, m_ColliderSize.y, m_ColliderSize.z };
    j["showColliderGizmo"] = m_ShowColliderGizmo;

    // Скрипты
if (!m_ScriptComponents.empty()) {
    nlohmann::json scriptsJson = nlohmann::json::array();
    for (const auto& script : m_ScriptComponents) {
        if (script->IsLoaded()) {
            scriptsJson.push_back(script->GetScriptPath());
        }
    }
    j["luaScripts"] = scriptsJson;
}

    return j;
}

bool GameObject::FromJson(const nlohmann::json& j) {
    try {
        if (j.contains("name")) m_Name = j["name"];
        if (j.contains("position")) {
            auto pos = j["position"];
            m_Position = glm::vec3(pos[0], pos[1], pos[2]);
        }
        if (j.contains("rotation")) {
            auto rot = j["rotation"];
            m_Rotation = glm::vec3(rot[0], rot[1], rot[2]);
        }
        if (j.contains("scale")) {
            auto s = j["scale"];
            m_Scale = glm::vec3(s[0], s[1], s[2]);
        }
        if (j.contains("visible")) m_Visible = j["visible"];
        if (j.contains("color")) {
            auto col = j["color"];
            m_Color = glm::vec3(col[0], col[1], col[2]);
        }
        if (j.contains("castShadows")) m_CastShadows = j["castShadows"];
        if (j.contains("receiveShadows")) m_ReceiveShadows = j["receiveShadows"];

        // Свет
        if (j.contains("lightType")) {
            m_LightType = j["lightType"];
            if (m_LightType != LT_NONE) {
                if (j.contains("lightColor")) {
                    auto col = j["lightColor"];
                    m_LightColor = glm::vec3(col[0], col[1], col[2]);
                }
                if (j.contains("lightIntensity")) m_LightIntensity = j["lightIntensity"];
                if (j.contains("lightRange")) m_LightRange = j["lightRange"];
                if (j.contains("lightAngle")) m_LightAngle = j["lightAngle"];
                if (j.contains("lightDirection")) {
                    auto dir = j["lightDirection"];
                    m_LightDirection = glm::vec3(dir[0], dir[1], dir[2]);
                }
                if (j.contains("shaftEnabled")) m_ShaftEnabled = j["shaftEnabled"];
                if (j.contains("shaftIntensity")) m_ShaftIntensity = j["shaftIntensity"];
                if (j.contains("shaftSoftness")) m_ShaftSoftness = j["shaftSoftness"];
                if (j.contains("shaftDensity")) m_ShaftDensity = j["shaftDensity"];
            }
        }

        // Камера
        if (j.contains("isCamera") && j["isCamera"] == true) {
            m_IsCamera = true;
            if (j.contains("cameraFOV")) m_CameraFOV = j["cameraFOV"];
            if (j.contains("cameraNear")) m_CameraNear = j["cameraNear"];
            if (j.contains("cameraFar")) m_CameraFar = j["cameraFar"];
            if (j.contains("frustumCulling")) m_FrustumCulling = j["frustumCulling"];
            if (j.contains("showFrustumGizmo")) m_ShowFrustumGizmo = j["showFrustumGizmo"];
        }

        // Меш
        if (j.contains("meshSourceType")) {
            std::string sourceType = j["meshSourceType"];
            m_MeshSourceType = sourceType;
            if (sourceType == "primitive") {
                std::string primitive = j["meshPrimitive"];
                m_MeshPrimitiveType = primitive;
                // Создаём примитив
                if (primitive == "cube") m_Mesh = Primitives::CreateCube();
                else if (primitive == "sphere") m_Mesh = Primitives::CreateSphere();
                else if (primitive == "cylinder") m_Mesh = Primitives::CreateCylinder();
                else if (primitive == "cone") m_Mesh = Primitives::CreateCone();
                else if (primitive == "pyramid") m_Mesh = Primitives::CreatePyramid();
                else if (primitive == "plane") m_Mesh = Primitives::CreatePlane();
                else m_Mesh = Primitives::CreateCube(); // fallback
            } else if (sourceType == "model") {
                std::string path = j["meshPath"];
                m_MeshPath = path;
                auto model = std::make_shared<Model>(path);
                if (model->IsLoaded() && !model->GetMeshes().empty()) {
                    m_Mesh = model->GetMeshes()[0];
                    // Если несколько мешей, можно создать дочерние объекты, но для простоты берём первый
                }
            }
        }

        // Материал
        if (j.contains("material")) {
            auto matJson = j["material"];
            auto material = std::make_shared<Material>();
            if (material->FromJson(matJson)) {
                m_Material = material;
            }
        }

        // Физика
        if (j.contains("colliderType")) {
            m_colliderType = (ColliderType)j["colliderType"].get<int>();
        }
        if (j.contains("mass")) m_mass = j["mass"];
        if (j.contains("friction")) m_friction = j["friction"];
        if (j.contains("restitution")) m_restitution = j["restitution"];
        if (j.contains("rollingFriction")) m_rollingFriction = j["rollingFriction"];
        if (j.contains("linearDamping")) m_linearDamping = j["linearDamping"];
        if (j.contains("angularDamping")) m_angularDamping = j["angularDamping"];
        bool hasRigidBody = j.value("hasRigidBody", false);
        if (hasRigidBody && m_colliderType != COLLIDER_NONE) {
            // Создаём физическое тело
            UpdatePhysicsBody();
        }

        // Аудио
        if (j.contains("audioClipPath")) {
            m_AudioClipPath = j["audioClipPath"];
            m_AudioVolume = j.value("audioVolume", 1.0f);
            m_AudioLoop = j.value("audioLoop", false);
            m_AudioSpatial = j.value("audioSpatial", true);
            m_AudioMinDistance = j.value("audioMinDistance", 1.0f);
            m_AudioMaxDistance = j.value("audioMaxDistance", 20.0f);
            m_AudioSourceEnabled = j.value("audioSourceEnabled", false);
            m_ShowAudioGizmo = j.value("showAudioGizmo", true);
            if (m_AudioSourceEnabled) {
                EnableAudioSource();
                SetAudioClip(m_AudioClipPath);
            }
        }

        // Туман
        if (j.contains("isFog") && j["isFog"] == true) {
            m_IsFog = true;
            m_FogEnabled = j.value("fogEnabled", false);
            m_FogType = j.value("fogType", 1);
            if (j.contains("fogColor")) {
                auto col = j["fogColor"];
                m_FogColor = glm::vec3(col[0], col[1], col[2]);
            }
            m_FogDensity = j.value("fogDensity", 0.04f);
            m_FogLinearStart = j.value("fogLinearStart", 10.0f);
            m_FogLinearEnd = j.value("fogLinearEnd", 50.0f);
        }

                // Коллайдер
        if (j.contains("colliderOffset")) {
            auto off = j["colliderOffset"];
            m_ColliderOffset = glm::vec3(off[0], off[1], off[2]);
        }
        if (j.contains("colliderSize")) {
            auto sz = j["colliderSize"];
            m_ColliderSize = glm::vec3(sz[0], sz[1], sz[2]);
        }
        m_ShowColliderGizmo = j.value("showColliderGizmo", true);

        if (j.contains("luaScripts")) {
    for (const auto& path : j["luaScripts"]) {
        auto script = std::make_shared<LuaScriptComponent>();
        if (script->LoadScript(path.get<std::string>(), this)) {
            m_ScriptComponents.push_back(script);
            // Start будет вызван отдельно после загрузки всей сцены
        }
    }
}

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing GameObject: " << e.what() << std::endl;
        return false;
    }
}

// ========== Установка меша с сохранением источника ==========
void GameObject::SetMeshFromPrimitive(const std::string& primitiveType) {
    m_MeshSourceType = "primitive";
    m_MeshPrimitiveType = primitiveType;
    m_MeshPath.clear();

    if (primitiveType == "cube") m_Mesh = Primitives::CreateCube();
    else if (primitiveType == "sphere") m_Mesh = Primitives::CreateSphere();
    else if (primitiveType == "cylinder") m_Mesh = Primitives::CreateCylinder();
    else if (primitiveType == "cone") m_Mesh = Primitives::CreateCone();
    else if (primitiveType == "pyramid") m_Mesh = Primitives::CreatePyramid();
    else if (primitiveType == "plane") m_Mesh = Primitives::CreatePlane();
    else m_Mesh = Primitives::CreateCube();
}

void GameObject::SetMeshFromModel(const std::string& path) {
    m_MeshSourceType = "model";
    m_MeshPath = path;
    m_MeshPrimitiveType.clear();

    auto model = std::make_shared<Model>(path);
    if (model->IsLoaded() && !model->GetMeshes().empty()) {
        m_Mesh = model->GetMeshes()[0];
        // Если нужно несколько мешей, создавайте дочерние объекты отдельно
    } else {
        std::cerr << "Failed to load model: " << path << std::endl;
        m_Mesh.reset();
    }
}

// ========== COLLIDER PARAMETERS IMPLEMENTATION ==========
void GameObject::SetColliderOffset(const glm::vec3& offset) {
    m_ColliderOffset = offset;
    RecreateCollider();
}

void GameObject::SetColliderHalfExtents(const glm::vec3& halfExtents) {
    if (m_colliderType == COLLIDER_BOX) {
        m_ColliderSize = halfExtents;
        RecreateCollider();
    }
}

void GameObject::SetColliderRadius(float radius) {
    if (m_colliderType == COLLIDER_SPHERE || m_colliderType == COLLIDER_CAPSULE) {
        m_ColliderSize.x = radius;
        RecreateCollider();
    }
}

void GameObject::SetColliderHeight(float height) {
    if (m_colliderType == COLLIDER_CAPSULE) {
        m_ColliderSize.y = height;
        RecreateCollider();
    }
}

void GameObject::RecreateCollider() {
    if (m_colliderType == COLLIDER_NONE) return;
    SetColliderType(m_colliderType); // пересоздаст форму и тело
}

void GameObject::AddScriptComponent(std::shared_ptr<LuaScriptComponent> script) {
    m_ScriptComponents.push_back(script);
}

void GameObject::RemoveScriptComponent(size_t index) {
    if (index < m_ScriptComponents.size()) {
        // Вызываем OnDestroy перед удалением
        m_ScriptComponents[index]->OnDestroy();
        m_ScriptComponents.erase(m_ScriptComponents.begin() + index);
    }
}
