#include "Graphics/Skybox.h"
#include "Graphics/Primitives.h"
#include <iostream>
#include <stb_image.h>

Skybox::Skybox() {}

Skybox::~Skybox() {
    if (m_TextureID) glDeleteTextures(1, &m_TextureID);
}

bool Skybox::Load(const std::string& right, const std::string& left,
                  const std::string& top, const std::string& bottom,
                  const std::string& front, const std::string& back) {
    if (m_TextureID) {
        glDeleteTextures(1, &m_TextureID);
        m_TextureID = 0;
    }

    glGenTextures(1, &m_TextureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureID);

    const char* faces[6] = { right.c_str(), left.c_str(), top.c_str(), bottom.c_str(), front.c_str(), back.c_str() };
    GLenum targets[6] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    int width, height, nrChannels;
    for (int i = 0; i < 6; i++) {
        unsigned char* data = stbi_load(faces[i], &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(targets[i], 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cerr << "Failed to load skybox texture: " << faces[i] << std::endl;
            return false;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Создаём сферу (64 сегмента достаточно)
    m_Mesh = Primitives::CreateSkyboxSphere(64);
    return true;
}

void Skybox::Draw() const {
    if (!m_Mesh) return;
    glDepthFunc(GL_LEQUAL);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureID);
    m_Mesh->Draw();
    glDepthFunc(GL_LESS);
}
