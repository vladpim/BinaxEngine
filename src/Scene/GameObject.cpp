#include "Scene/GameObject.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "Physics/PhysicsWorld.h"
#include <btBulletDynamicsCommon.h>

GameObject::GameObject(const std::string& name)
    : m_Name(name) {
}

GameObject::~GameObject() {
    // Удаляем себя из списка детей родителя
    if (m_Parent) {
        m_Parent->RemoveChild(this);
    }
    // Обнуляем родителя у своих детей
    for (auto& child : m_Children) {
        child->m_Parent = nullptr;
    }
}

void GameObject::AddChild(std::shared_ptr<GameObject> child) {
    if (!child) return;
    if (child.get() == this) return;          // нельзя добавить самого себя
    if (child->m_Parent == this) return;      // уже является прямым ребёнком

    // Если у child уже есть родитель, открепляем его от старого родителя
    if (child->m_Parent) {
        child->m_Parent->RemoveChild(child.get());
    }

    child->m_Parent = this;
    m_Children.push_back(child);
    std::cout << "AddChild SUCCESS: " << child->GetName() << " added to " << m_Name 
              << ", children count = " << m_Children.size() << std::endl;
    std::cout << "DEBUG: AddChild выполнен, теперь у " << m_Name 
    << " детей = " << m_Children.size() << std::endl;
}

void GameObject::RemoveChild(GameObject* child) {
    auto it = std::find_if(m_Children.begin(), m_Children.end(),
        [child](const std::shared_ptr<GameObject>& ptr) { return ptr.get() == child; });
    if (it != m_Children.end()) {
        (*it)->m_Parent = nullptr;
        m_Children.erase(it);
        std::cout << "Removed child: " << child->GetName() << " from " << m_Name << std::endl;
    }
}

glm::vec3 GameObject::GetWorldPosition() const {
    if (m_Parent) {
        return m_Parent->GetWorldPosition() + m_Position;
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
    if (m_Parent) {
        transform = m_Parent->GetTransformMatrix() * transform;
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

    // Выбираем материал: сначала свой, потом из меша
    std::shared_ptr<Material> mat = m_Material;
    if (!mat && m_Mesh) {
        mat = m_Mesh->GetMaterial();
    }

    if (mat) {
        shader.SetBool("hasDiffuseTexture", mat->HasDiffuse());
        shader.SetBool("hasNormalMap", mat->HasNormal());
        shader.SetFloat("metallic", mat->metallic);
        shader.SetFloat("roughness", mat->roughness);
        shader.SetVec2("uvScale", mat->uvScale.x, mat->uvScale.y);
        shader.SetFloat("normalStrength", mat->normalStrength);
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
        shader.SetBool("hasDiffuseTexture", false);
        shader.SetBool("hasNormalMap", false);
        shader.SetVec3("emissionColor", 0.0f, 0.0f, 0.0f);
        shader.SetFloat("emissionIntensity", 0.0f);
    }

    m_Mesh->Draw();

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
    glm::vec3 scale = GetScale();
    std::cout << "SetColliderType: " << type << " scale=(" << scale.x << "," << scale.y << "," << scale.z << ")" << std::endl;
    
    switch (type) {
        case COLLIDER_BOX:
            m_collisionShape = new btBoxShape(btVector3(scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f));
            break;
        case COLLIDER_SPHERE:
            m_collisionShape = new btSphereShape(scale.x * 0.5f);
            break;
        case COLLIDER_CAPSULE:
            m_collisionShape = new btCapsuleShape(scale.x * 0.5f, scale.y - scale.x);
            break;
        default:
            m_collisionShape = nullptr;
            break;
    }
    
    // Обновляем физическое тело (создаём/пересоздаём)
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
}

void GameObject::SetParent(std::shared_ptr<GameObject> newParent, bool keepWorldPosition) {
    if (newParent.get() == this) return;
    if (newParent && newParent.get() == m_Parent) return;

    glm::mat4 worldMat = GetTransformMatrix();

    // Открепляемся от старого родителя
    if (m_Parent) {
        m_Parent->RemoveChild(this);
        m_Parent = nullptr;
    }

    // Прикрепляемся к новому
    if (newParent) {
        m_Parent = newParent.get();
        // Прямое добавление в вектор (обход AddChild)
        newParent->m_Children.push_back(shared_from_this());
        std::cout << "DIRECT ADD: parent " << newParent->GetName() 
                  << " now has " << newParent->m_Children.size() << " children" << std::endl;
    }

    // Если нужно сохранить мировую позицию, пересчитываем локальную
    if (keepWorldPosition && m_Parent) {
        glm::mat4 parentInv = glm::inverse(m_Parent->GetTransformMatrix());
        glm::mat4 localMat = parentInv * worldMat;
        glm::vec3 scale, pos, skew;
        glm::quat rot;
        glm::vec4 persp;
        glm::decompose(localMat, scale, rot, pos, skew, persp);
        SetPosition(pos);
        SetRotation(glm::degrees(glm::eulerAngles(rot)));
        SetScale(scale);
    }

   std::cout << "SetParent: " << GetName() << " parent is " << (m_Parent ? m_Parent->GetName() : "null") << std::endl;
}

bool GameObject::CanHavePhysics() const {
    return m_Parent == nullptr && m_Children.empty();
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
            outMin = glm::min(outMin, world);
            outMax = glm::max(outMax, world);
        }
        return;
    }
    // Вычисляем AABB по вершинам меша (лучше закэшировать локальный AABB в Mesh)
    const auto& vertices = m_Mesh->GetVertices();
    glm::vec3 localMin( FLT_MAX), localMax(-FLT_MAX);
    for (const auto& v : vertices) {
        localMin = glm::min(localMin, glm::vec3(v.Position[0], v.Position[1], v.Position[2]));
        localMax = glm::max(localMax, glm::vec3(v.Position[0], v.Position[1], v.Position[2]));
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
        outMin = glm::min(outMin, world);
        outMax = glm::max(outMax, world);
    }
}

void GameObject::Unparent() {
    if (!m_Parent) return;

    // Сохраняем мировую позицию
    glm::mat4 worldMat = GetTransformMatrix();
    glm::vec3 scale, pos, skew;
    glm::quat rot;
    glm::vec4 persp;
    glm::decompose(worldMat, scale, rot, pos, skew, persp);

    // Открепляемся
    m_Parent->RemoveChild(this);
    m_Parent = nullptr;

    // Устанавливаем абсолютную трансформацию
    SetPosition(pos);
    SetRotation(glm::degrees(glm::eulerAngles(rot)));
    SetScale(scale);

    // Если была физика – удаляем (на всякий случай)
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


// В самый конец файла:

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
    
    // Индексы для боковых граней (треугольники: вершина – две соседние точки основания)
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
