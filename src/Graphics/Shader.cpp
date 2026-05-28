#include "Graphics/Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>

Shader::~Shader() {
    if (m_ID != 0)
        glDeleteProgram(m_ID);
}

bool Shader::Load(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexCode, fragmentCode;
    std::ifstream vShaderFile, fShaderFile;

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    } catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        return CreateDefaultShader();  // вместо return false
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    GLuint vertex, fragment;
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    if (!CheckCompileErrors(vertex, "VERTEX")) {
        glDeleteShader(vertex);
        return CreateDefaultShader();
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    if (!CheckCompileErrors(fragment, "FRAGMENT")) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return CreateDefaultShader();
    }

    m_ID = glCreateProgram();
    glAttachShader(m_ID, vertex);
    glAttachShader(m_ID, fragment);
    glLinkProgram(m_ID);
    GLint linked;
    glGetProgramiv(m_ID, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLchar infoLog[1024];
        glGetProgramInfoLog(m_ID, 1024, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM_LINKING_ERROR\n" << infoLog << std::endl;
        glDeleteProgram(m_ID);
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return CreateDefaultShader();
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    std::cout << "Shader loaded successfully: " << vertexPath << ", " << fragmentPath << std::endl;
    return true;
}

bool Shader::CreateDefaultShader() {
    const char* defaultVert = R"(
        #version 330 core
        layout(location=0) in vec3 aPos;
        void main() { gl_Position = vec4(aPos, 1.0); }
    )";
    const char* defaultFrag = R"(
        #version 330 core
        out vec4 FragColor;
        void main() { FragColor = vec4(1.0,0.0,1.0,1.0); }
    )";
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &defaultVert, nullptr);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &defaultFrag, nullptr);
    glCompileShader(fs);
    m_ID = glCreateProgram();
    glAttachShader(m_ID, vs);
    glAttachShader(m_ID, fs);
    glLinkProgram(m_ID);
    glDeleteShader(vs);
    glDeleteShader(fs);
    std::cerr << "Using default magenta shader as fallback." << std::endl;
    return true;
}

void Shader::Use() const {
    glUseProgram(m_ID);
}

void Shader::SetBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(m_ID, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(m_ID, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(m_ID, name.c_str()), value);
}

void Shader::SetVec2(const std::string& name, float x, float y) const {   // <-- реализация
    glUniform2f(glGetUniformLocation(m_ID, name.c_str()), x, y);
}

void Shader::SetVec3(const std::string& name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(m_ID, name.c_str()), x, y, z);
}

void Shader::SetMat4(const std::string& name, const float* matrix) const {
    glUniformMatrix4fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, matrix);
}

bool Shader::CheckCompileErrors(GLuint shader, const std::string& type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
            return false;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << std::endl;
            return false;
        }
    }
    return true;
}
