#include "Scene/Camera.h"
#include <GLFW/glfw3.h>

Camera::Camera(glm::vec3 position)
    : m_Position(position), m_WorldUp(glm::vec3(0.0f, 1.0f, 0.0f)) {
    UpdateCameraVectors();
}

void Camera::Update(float deltaTime) {
    // Можно добавить логику обновления
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset) {
    xoffset *= m_MouseSensitivity;
    yoffset *= m_MouseSensitivity;

    m_Yaw += xoffset;
    m_Pitch += yoffset;

    // Ограничение угла наклона
    if (m_Pitch > 89.0f) m_Pitch = 89.0f;
    if (m_Pitch < -89.0f) m_Pitch = -89.0f;

    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset) {
    m_Zoom -= yoffset;
    if (m_Zoom < 1.0f) m_Zoom = 1.0f;
    if (m_Zoom > 45.0f) m_Zoom = 45.0f;
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(m_Zoom), aspectRatio, 0.1f, 100.0f);
}

void Camera::MoveForward(float speed) {
    m_Position += m_Front * speed;
}

void Camera::MoveBackward(float speed) {
    m_Position -= m_Front * speed;
}

void Camera::MoveLeft(float speed) {
    m_Position -= m_Right * speed;
}

void Camera::MoveRight(float speed) {
    m_Position += m_Right * speed;
}

void Camera::MoveUp(float speed) {
    m_Position += m_WorldUp * speed;
}

void Camera::MoveDown(float speed) {
    m_Position -= m_WorldUp * speed;
}

void Camera::UpdateCameraVectors() {
    // Вычисляем новый вектор направления
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Front = glm::normalize(front);

    // Пересчитываем правый и верхний векторы
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}
