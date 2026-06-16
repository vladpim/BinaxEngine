#include "PhysicsWorld.h"
#include "Scene/GameObject.h"
#include <iostream>

PhysicsWorld& PhysicsWorld::GetInstance() {
    static PhysicsWorld instance;
    return instance;
}

void PhysicsWorld::Initialize() {
    m_collisionConfig = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_collisionConfig);
    m_broadphase = new btDbvtBroadphase();
    m_solver = new btSequentialImpulseConstraintSolver();
    m_world = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfig);
    m_world->setGravity(btVector3(0, -9.81f, 0));
    std::cout << "PhysicsWorld initialized\n";
}

void PhysicsWorld::Update(float deltaTime) {
    if (!m_isSimulating) return;
    if (m_world) {
        std::cout << "Physics step, delta=" << deltaTime << ", bodies=" << m_world->getNumCollisionObjects() << std::endl;
        m_world->stepSimulation(deltaTime, 10);
        // Вывод позиции первого динамического тела
        for (int i = 0; i < m_world->getNumCollisionObjects(); i++) {
            btCollisionObject* obj = m_world->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && !body->isStaticObject()) {
                btVector3 pos = body->getCenterOfMassPosition();
                std::cout << "Physics pos y = " << pos.y() << std::endl;
                break;
            }
        }
    }
}

void PhysicsWorld::Shutdown() {
    for (auto body : m_bodies) {
        m_world->removeRigidBody(body);
        delete body->getMotionState();
        delete body;
    }
    delete m_world;
    delete m_solver;
    delete m_broadphase;
    delete m_dispatcher;
    delete m_collisionConfig;
}

void PhysicsWorld::AddRigidBody(btRigidBody* body) {
    if (m_world && body) {
        m_world->addRigidBody(body);
        m_bodies.push_back(body);
        std::cout << "[Physics] Added body, total now: " << m_world->getNumCollisionObjects() << std::endl;
    } else {
        std::cout << "[Physics] ERROR: Cannot add body (world=" << (m_world ? "ok" : "null")
                  << ", body=" << (body ? "ok" : "null") << ")" << std::endl;
    }
}

void PhysicsWorld::RemoveRigidBody(btRigidBody* body) {
    if (m_world && body) {
        m_world->removeRigidBody(body);
        auto it = std::find(m_bodies.begin(), m_bodies.end(), body);
        if (it != m_bodies.end()) m_bodies.erase(it);
    }
}

void PhysicsWorld::RegisterGameObject(GameObject* obj) {
    if (std::find(m_registeredObjects.begin(), m_registeredObjects.end(), obj) == m_registeredObjects.end())
        m_registeredObjects.push_back(obj);
}

void PhysicsWorld::ResetAllObjects() {
    for (auto obj : m_registeredObjects) {
        obj->ResetToInitialTransform();
    }
}