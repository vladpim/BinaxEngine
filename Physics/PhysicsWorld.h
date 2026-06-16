#pragma once
#include <btBulletDynamicsCommon.h>
#include <memory>
#include <vector>

class GameObject;

class PhysicsWorld {
public:
    static PhysicsWorld& GetInstance();

    void Initialize();
    void Update(float deltaTime);
    void Shutdown();

    void AddRigidBody(btRigidBody* body);
    void RemoveRigidBody(btRigidBody* body);

    // Включение/выключение симуляции
    void SetSimulationActive(bool active) { m_isSimulating = active; }
    bool IsSimulating() const { return m_isSimulating; }

    // Сброс всех физических объектов (вернуть в начальные позиции)
    void ResetAllObjects();

    // Регистрация GameObject для сброса
    void RegisterGameObject(GameObject* obj);

private:
    PhysicsWorld() = default;
    ~PhysicsWorld() = default;

    btDefaultCollisionConfiguration* m_collisionConfig = nullptr;
    btCollisionDispatcher* m_dispatcher = nullptr;
    btBroadphaseInterface* m_broadphase = nullptr;
    btSequentialImpulseConstraintSolver* m_solver = nullptr;
    btDiscreteDynamicsWorld* m_world = nullptr;

    bool m_isSimulating = false;
    std::vector<GameObject*> m_registeredObjects;
    std::vector<btRigidBody*> m_bodies;
};