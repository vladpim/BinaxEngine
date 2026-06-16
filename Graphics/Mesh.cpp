#include "Graphics/Mesh.h"
#include <iostream>
#include <stb_image.h>
#include <glm/gtc/type_ptr.hpp>

Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<unsigned int>& indices,
           const std::string& diffusePath,
           const std::string& normalPath)
    : m_IndexCount(indices.size())
{
    SetupMesh(vertices, indices);
    m_Vertices = vertices;
    if (!diffusePath.empty())
        m_DiffuseTexture = LoadTexture(diffusePath);
    if (!normalPath.empty())
        m_NormalTexture = LoadTexture(normalPath);
}

Mesh::~Mesh() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    if (m_DiffuseTexture) glDeleteTextures(1, &m_DiffuseTexture);
    if (m_NormalTexture) glDeleteTextures(1, &m_NormalTexture);
}

void Mesh::SetupMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Позиция (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    // Нормаль (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);
    // Текстурные координаты (location = 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2);
    // Касательная (location = 3)
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

GLuint Mesh::LoadTexture(const std::string& path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    if (textureID == 0) {
        std::cerr << "Failed to generate texture ID" << std::endl;
        return 0;
    }

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "Texture loaded: " << path << std::endl;
        return textureID;
    } else {
        std::cerr << "Failed to load texture: " << path << " - " << stbi_failure_reason() << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }
}

void Mesh::Draw() const {
    if (VAO == 0 || m_IndexCount == 0) return;
    glBindVertexArray(VAO);
    glDrawElements(m_DrawMode, (GLsizei)m_IndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
