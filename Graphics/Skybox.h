#pragma once
#include <string>
#include <memory>
#include <GL/glew.h>
#include "Graphics/Mesh.h"

class Skybox {
public:
    Skybox();
    ~Skybox();

    bool Load(const std::string& right, const std::string& left,
              const std::string& top, const std::string& bottom,
              const std::string& front, const std::string& back);
    void Draw() const;

private:
    std::shared_ptr<Mesh> m_Mesh;
    GLuint m_TextureID = 0;
};
