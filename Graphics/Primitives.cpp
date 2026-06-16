#include "Graphics/Primitives.h"
#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

// ��������������� ������� ��� ���������� ����������� �� ������ ������ ������������
static void ComputeTangents(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    const size_t vertexCount = vertices.size();
    std::vector<glm::vec3> tan1(vertexCount, glm::vec3(0.0f));
    std::vector<glm::vec3> tan2(vertexCount, glm::vec3(0.0f));

    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i1 = indices[i];
        unsigned int i2 = indices[i + 1];
        unsigned int i3 = indices[i + 2];

        const Vertex& v1 = vertices[i1];
        const Vertex& v2 = vertices[i2];
        const Vertex& v3 = vertices[i3];

        glm::vec3 p1(v1.Position[0], v1.Position[1], v1.Position[2]);
        glm::vec3 p2(v2.Position[0], v2.Position[1], v2.Position[2]);
        glm::vec3 p3(v3.Position[0], v3.Position[1], v3.Position[2]);

        glm::vec2 uv1(v1.TexCoords[0], v1.TexCoords[1]);
        glm::vec2 uv2(v2.TexCoords[0], v2.TexCoords[1]);
        glm::vec2 uv3(v3.TexCoords[0], v3.TexCoords[1]);

        glm::vec3 edge1 = p2 - p1;
        glm::vec3 edge2 = p3 - p1;
        glm::vec2 duv1 = uv2 - uv1;
        glm::vec2 duv2 = uv3 - uv1;

        float det = duv1.x * duv2.y - duv2.x * duv1.y;
        if (fabs(det) < 1e-6) continue;

        float invDet = 1.0f / det;
        glm::vec3 t = (edge1 * duv2.y - edge2 * duv1.y) * invDet;
        glm::vec3 b = (edge2 * duv1.x - edge1 * duv2.x) * invDet; // ��������� (���� �� ���������)

        tan1[i1] += t;
        tan1[i2] += t;
        tan1[i3] += t;

        tan2[i1] += b;
        tan2[i2] += b;
        tan2[i3] += b;
    }

    for (size_t i = 0; i < vertexCount; ++i) {
        const glm::vec3 n(vertices[i].Normal[0], vertices[i].Normal[1], vertices[i].Normal[2]);
        const glm::vec3 t = tan1[i];

        // ��������������� �����-������
        glm::vec3 tangent = glm::normalize(t - n * glm::dot(n, t));
        vertices[i].Tangent[0] = tangent.x;
        vertices[i].Tangent[1] = tangent.y;
        vertices[i].Tangent[2] = tangent.z;
    }
}

// ========== ��� ==========
std::shared_ptr<Mesh> Primitives::CreateCube() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float cubeVertices[] = {
        // positions          // normals           // texcoords
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  1.0f,  0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f
    };

    unsigned int cubeIndices[] = {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };

    for (int i = 0; i < 24 * 8; i += 8) {
        Vertex vertex;
        vertex.Position[0] = cubeVertices[i];
        vertex.Position[1] = cubeVertices[i + 1];
        vertex.Position[2] = cubeVertices[i + 2];
        vertex.Normal[0] = cubeVertices[i + 3];
        vertex.Normal[1] = cubeVertices[i + 4];
        vertex.Normal[2] = cubeVertices[i + 5];
        vertex.TexCoords[0] = cubeVertices[i + 6];
        vertex.TexCoords[1] = cubeVertices[i + 7];
        vertex.Tangent[0] = 0.0f; vertex.Tangent[1] = 0.0f; vertex.Tangent[2] = 0.0f;
        vertices.push_back(vertex);
    }

    for (int i = 0; i < 36; i++) {
        indices.push_back(cubeIndices[i]);
    }

    ComputeTangents(vertices, indices);
    return std::make_shared<Mesh>(vertices, indices);
}

// ========== ����� (Grid) ==========
std::shared_ptr<Mesh> Primitives::CreateGrid(int size) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfSize = size / 2.0f;

    float gridVertices[] = {
        -halfSize, 0.0f, -halfSize, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
         halfSize, 0.0f, -halfSize, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
         halfSize, 0.0f,  halfSize, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        -halfSize, 0.0f,  halfSize, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
    };

    unsigned int gridIndices[] = { 0, 1, 2, 0, 2, 3 };

    for (int i = 0; i < 4 * 8; i += 8) {
        Vertex vertex;
        vertex.Position[0] = gridVertices[i];
        vertex.Position[1] = gridVertices[i + 1];
        vertex.Position[2] = gridVertices[i + 2];
        vertex.Normal[0] = gridVertices[i + 3];
        vertex.Normal[1] = gridVertices[i + 4];
        vertex.Normal[2] = gridVertices[i + 5];
        vertex.TexCoords[0] = gridVertices[i + 6];
        vertex.TexCoords[1] = gridVertices[i + 7];
        vertex.Tangent[0] = 0.0f; vertex.Tangent[1] = 0.0f; vertex.Tangent[2] = 0.0f;
        vertices.push_back(vertex);
    }

    for (int i = 0; i < 6; i++) {
        indices.push_back(gridIndices[i]);
    }

    ComputeTangents(vertices, indices);
    return std::make_shared<Mesh>(vertices, indices);
}

// ========== ����� (���������, ��� �����������) ==========
std::shared_ptr<Mesh> Primitives::CreateSphere(int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;
    float radius = 0.5f;

    for (int y = 0; y <= segments; ++y) {
        for (int x = 0; x <= segments; ++x) {
            float xSegment = (float)x / (float)segments;
            float ySegment = (float)y / (float)segments;

            float xPos = radius * std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = radius * std::cos(ySegment * PI);
            float zPos = radius * std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            Vertex vertex;
            vertex.Position[0] = xPos;
            vertex.Position[1] = yPos;
            vertex.Position[2] = zPos;

            vertex.Normal[0] = xPos / radius;
            vertex.Normal[1] = yPos / radius;
            vertex.Normal[2] = zPos / radius;

            vertex.TexCoords[0] = xSegment;
            vertex.TexCoords[1] = ySegment;

            vertex.Tangent[0] = 1.0f; vertex.Tangent[1] = 0.0f; vertex.Tangent[2] = 0.0f; // ��������

            vertices.push_back(vertex);
        }
    }

    for (int y = 0; y < segments; ++y) {
        for (int x = 0; x < segments; ++x) {
            int first = (y * (segments + 1)) + x;
            int second = first + segments + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    // ��� ����� ����������� �� ��������� (����� �������� �����)
    return std::make_shared<Mesh>(vertices, indices);
}

// ========== ������� (���������, ��� �����������) ==========
std::shared_ptr<Mesh> Primitives::CreateCylinder(int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;
    float radius = 0.5f;
    float height = 1.0f;
    float halfHeight = height * 0.5f;

    // ������� �����������
    for (int i = 0; i <= segments; ++i) {
        float theta = (float)i / (float)segments * 2.0f * PI;
        float x = radius * cos(theta);
        float z = radius * sin(theta);

        Vertex topVertex;
        topVertex.Position[0] = x;
        topVertex.Position[1] = halfHeight;
        topVertex.Position[2] = z;
        topVertex.Normal[0] = cos(theta);
        topVertex.Normal[1] = 0.0f;
        topVertex.Normal[2] = sin(theta);
        topVertex.TexCoords[0] = (float)i / (float)segments;
        topVertex.TexCoords[1] = 1.0f;
        topVertex.Tangent[0] = -sin(theta); topVertex.Tangent[1] = 0.0f; topVertex.Tangent[2] = cos(theta); // ����������
        vertices.push_back(topVertex);

        Vertex bottomVertex;
        bottomVertex.Position[0] = x;
        bottomVertex.Position[1] = -halfHeight;
        bottomVertex.Position[2] = z;
        bottomVertex.Normal[0] = cos(theta);
        bottomVertex.Normal[1] = 0.0f;
        bottomVertex.Normal[2] = sin(theta);
        bottomVertex.TexCoords[0] = (float)i / (float)segments;
        bottomVertex.TexCoords[1] = 0.0f;
        bottomVertex.Tangent[0] = -sin(theta); bottomVertex.Tangent[1] = 0.0f; bottomVertex.Tangent[2] = cos(theta);
        vertices.push_back(bottomVertex);
    }

    // ������� ������� �����������
    for (int i = 0; i < segments; ++i) {
        int topLeft = i * 2;
        int topRight = topLeft + 2;
        int bottomLeft = topLeft + 1;
        int bottomRight = topRight + 1;

        indices.push_back(topLeft);
        indices.push_back(bottomLeft);
        indices.push_back(topRight);
        indices.push_back(topRight);
        indices.push_back(bottomLeft);
        indices.push_back(bottomRight);
    }

    // ������� ������ (����� + ����)
    int centerTop = (int)vertices.size();
    Vertex centerTopVertex;
    centerTopVertex.Position[0] = 0.0f;
    centerTopVertex.Position[1] = halfHeight;
    centerTopVertex.Position[2] = 0.0f;
    centerTopVertex.Normal[0] = 0.0f;
    centerTopVertex.Normal[1] = 1.0f;
    centerTopVertex.Normal[2] = 0.0f;
    centerTopVertex.TexCoords[0] = 0.5f;
    centerTopVertex.TexCoords[1] = 0.5f;
    centerTopVertex.Tangent[0] = 1.0f; centerTopVertex.Tangent[1] = 0.0f; centerTopVertex.Tangent[2] = 0.0f;
    vertices.push_back(centerTopVertex);

    int centerBottom = (int)vertices.size();
    Vertex centerBottomVertex;
    centerBottomVertex.Position[0] = 0.0f;
    centerBottomVertex.Position[1] = -halfHeight;
    centerBottomVertex.Position[2] = 0.0f;
    centerBottomVertex.Normal[0] = 0.0f;
    centerBottomVertex.Normal[1] = -1.0f;
    centerBottomVertex.Normal[2] = 0.0f;
    centerBottomVertex.TexCoords[0] = 0.5f;
    centerBottomVertex.TexCoords[1] = 0.5f;
    centerBottomVertex.Tangent[0] = 1.0f; centerBottomVertex.Tangent[1] = 0.0f; centerBottomVertex.Tangent[2] = 0.0f;
    vertices.push_back(centerBottomVertex);

    // ������� ��� ������
    for (int i = 0; i <= segments; ++i) {
        float theta = (float)i / (float)segments * 2.0f * PI;
        float x = radius * cos(theta);
        float z = radius * sin(theta);

        Vertex topCap;
        topCap.Position[0] = x;
        topCap.Position[1] = halfHeight;
        topCap.Position[2] = z;
        topCap.Normal[0] = 0.0f;
        topCap.Normal[1] = 1.0f;
        topCap.Normal[2] = 0.0f;
        topCap.TexCoords[0] = (cos(theta) + 1.0f) * 0.5f;
        topCap.TexCoords[1] = (sin(theta) + 1.0f) * 0.5f;
        topCap.Tangent[0] = 1.0f; topCap.Tangent[1] = 0.0f; topCap.Tangent[2] = 0.0f;
        vertices.push_back(topCap);

        Vertex bottomCap;
        bottomCap.Position[0] = x;
        bottomCap.Position[1] = -halfHeight;
        bottomCap.Position[2] = z;
        bottomCap.Normal[0] = 0.0f;
        bottomCap.Normal[1] = -1.0f;
        bottomCap.Normal[2] = 0.0f;
        bottomCap.TexCoords[0] = (cos(theta) + 1.0f) * 0.5f;
        bottomCap.TexCoords[1] = (sin(theta) + 1.0f) * 0.5f;
        bottomCap.Tangent[0] = 1.0f; bottomCap.Tangent[1] = 0.0f; bottomCap.Tangent[2] = 0.0f;
        vertices.push_back(bottomCap);
    }

    // ������� ������
    for (int i = 0; i < segments; ++i) {
        int topStart = centerTop + 1;
        int bottomStart = centerBottom + 1;

        indices.push_back(centerTop);
        indices.push_back(topStart + i + 1);
        indices.push_back(topStart + i);

        indices.push_back(centerBottom);
        indices.push_back(bottomStart + i);
        indices.push_back(bottomStart + i + 1);
    }

    return std::make_shared<Mesh>(vertices, indices);
}

// ========== ����� (���� ��� �������) ==========
std::shared_ptr<Mesh> Primitives::CreateCone(int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    const float PI = 3.14159265359f;
    float radius = 0.5f;
    float height = 1.0f;
    float halfHeight = height * 0.5f;
    
    // Вершина конуса
    Vertex tip;
    tip.Position[0] = 0.0f; tip.Position[1] = halfHeight; tip.Position[2] = 0.0f;
    tip.Normal[0] = 0.0f; tip.Normal[1] = 1.0f; tip.Normal[2] = 0.0f;
    tip.TexCoords[0] = 0.5f; tip.TexCoords[1] = 1.0f;
    tip.Tangent[0] = 1.0f; tip.Tangent[1] = 0.0f; tip.Tangent[2] = 0.0f;
    vertices.push_back(tip);
    
    // Боковая поверхность (кольцо основания)
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * PI;
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        float u = (float)i / segments;
        
        Vertex v;
        v.Position[0] = x; v.Position[1] = -halfHeight; v.Position[2] = z;
        // Нормаль для боковой поверхности (направлена наружу и немного вверх)
        glm::vec3 normal = glm::normalize(glm::vec3(x, radius, z));
        v.Normal[0] = normal.x; v.Normal[1] = normal.y; v.Normal[2] = normal.z;
        v.TexCoords[0] = u; v.TexCoords[1] = 0.0f;
        v.Tangent[0] = -sin(angle); v.Tangent[1] = 0.0f; v.Tangent[2] = cos(angle);
        vertices.push_back(v);
    }
    
    // Индексы для боковых граней
    for (int i = 0; i < segments; i++) {
        indices.push_back(0);         // tip
        indices.push_back(i + 1);     // base i
        indices.push_back(i + 2);     // base i+1
    }
    
    // Основание (круг)
    int centerIndex = (int)vertices.size();
    Vertex center;
    center.Position[0] = 0.0f; center.Position[1] = -halfHeight; center.Position[2] = 0.0f;
    center.Normal[0] = 0.0f; center.Normal[1] = -1.0f; center.Normal[2] = 0.0f;
    center.TexCoords[0] = 0.5f; center.TexCoords[1] = 0.5f;
    center.Tangent[0] = 1.0f; center.Tangent[1] = 0.0f; center.Tangent[2] = 0.0f;
    vertices.push_back(center);
    
    int firstRing = (int)vertices.size();
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * PI;
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        Vertex v;
        v.Position[0] = x; v.Position[1] = -halfHeight; v.Position[2] = z;
        v.Normal[0] = 0.0f; v.Normal[1] = -1.0f; v.Normal[2] = 0.0f;
        v.TexCoords[0] = (cos(angle) + 1.0f) * 0.5f;
        v.TexCoords[1] = (sin(angle) + 1.0f) * 0.5f;
        v.Tangent[0] = 1.0f; v.Tangent[1] = 0.0f; v.Tangent[2] = 0.0f;
        vertices.push_back(v);
    }
    
    for (int i = 0; i < segments; i++) {
        indices.push_back(centerIndex);
        indices.push_back(firstRing + i);
        indices.push_back(firstRing + i + 1);
    }
    
    return std::make_shared<Mesh>(vertices, indices);
}

// ========== �������� ==========
std::shared_ptr<Mesh> Primitives::CreatePyramid() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float pyramidVertices[] = {
        // �������           // �������           // ���������� ����������
        -0.5f, 0.0f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         0.5f, 0.0f, -0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         0.5f, 0.0f,  0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
        -0.5f, 0.0f,  0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,

        0.0f, 1.0f,  0.0f,  0.0f, 0.8f, 0.0f,   0.5f, 0.5f
    };

    unsigned int pyramidIndices[] = {
        0, 1, 2, 0, 2, 3,
        0, 1, 4,
        1, 2, 4,
        2, 3, 4,
        3, 0, 4
    };

    for (int i = 0; i < 5 * 8; i += 8) {
        Vertex vertex;
        vertex.Position[0] = pyramidVertices[i];
        vertex.Position[1] = pyramidVertices[i + 1];
        vertex.Position[2] = pyramidVertices[i + 2];
        vertex.Normal[0] = pyramidVertices[i + 3];
        vertex.Normal[1] = pyramidVertices[i + 4];
        vertex.Normal[2] = pyramidVertices[i + 5];
        vertex.TexCoords[0] = pyramidVertices[i + 6];
        vertex.TexCoords[1] = pyramidVertices[i + 7];
        vertex.Tangent[0] = 0.0f; vertex.Tangent[1] = 0.0f; vertex.Tangent[2] = 0.0f;
        vertices.push_back(vertex);
    }

    for (int i = 0; i < 18; i++) {
        indices.push_back(pyramidIndices[i]);
    }

    ComputeTangents(vertices, indices);
    return std::make_shared<Mesh>(vertices, indices);
}

// ========== ��������� ==========
std::shared_ptr<Mesh> Primitives::CreatePlane() {
    return CreateGrid(1);
}

std::shared_ptr<Mesh> Primitives::CreateSkyboxSphere(int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    const float radius = 1.0f;
    const float pi = 3.14159265359f;

    for (int y = 0; y <= segments; y++) {
        float v = (float)y / segments;
        float theta = v * pi;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        for (int x = 0; x <= segments; x++) {
            float u = (float)x / segments;
            float phi = u * 2.0f * pi;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            float px = cosPhi * sinTheta;
            float py = cosTheta;
            float pz = sinPhi * sinTheta;
            Vertex vert;
            vert.Position[0] = px * radius;
            vert.Position[1] = py * radius;
            vert.Position[2] = pz * radius;
            // Нормали внутрь (для скайбокса)
            vert.Normal[0] = -px;
            vert.Normal[1] = -py;
            vert.Normal[2] = -pz;
            // Текстурные координаты (для сферической карты, но не нужны для кубической)
            vert.TexCoords[0] = u;
            vert.TexCoords[1] = v;
            vert.Tangent[0] = 1.0f; vert.Tangent[1] = 0.0f; vert.Tangent[2] = 0.0f;
            vertices.push_back(vert);
        }
    }
    for (int y = 0; y < segments; y++) {
        for (int x = 0; x < segments; x++) {
            int first = y * (segments + 1) + x;
            int second = first + segments + 1;
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    auto mesh = std::make_shared<Mesh>(vertices, indices);
    mesh->SetName("SkyboxSphere");
    return mesh;
}

std::shared_ptr<Mesh> Primitives::CreateGridLines(int size, float step) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    int half = size / 2;
    
    // Генерируем линии вдоль оси X (Z = const)
    for (float z = -half; z <= half + 0.1f; z += step) {
        Vertex v1, v2;
        v1.Position[0] = -half; v1.Position[1] = 0; v1.Position[2] = z;
        v2.Position[0] =  half; v2.Position[1] = 0; v2.Position[2] = z;
        // Нормали и текстурные координаты не важны, заполним нулями
        for (int i = 0; i < 3; i++) { v1.Normal[i] = v2.Normal[i] = 0; v1.Tangent[i] = v2.Tangent[i] = 0; }
        v1.TexCoords[0] = v1.TexCoords[1] = v2.TexCoords[0] = v2.TexCoords[1] = 0;
        vertices.push_back(v1);
        vertices.push_back(v2);
    }
    // Линии вдоль оси Z (X = const)
    for (float x = -half; x <= half + 0.1f; x += step) {
        Vertex v1, v2;
        v1.Position[0] = x; v1.Position[1] = 0; v1.Position[2] = -half;
        v2.Position[0] = x; v2.Position[1] = 0; v2.Position[2] =  half;
        for (int i = 0; i < 3; i++) { v1.Normal[i] = v2.Normal[i] = 0; v1.Tangent[i] = v2.Tangent[i] = 0; }
        v1.TexCoords[0] = v1.TexCoords[1] = v2.TexCoords[0] = v2.TexCoords[1] = 0;
        vertices.push_back(v1);
        vertices.push_back(v2);
    }
    
    // Индексы: просто 0,1,2,3,... (каждая линия – два вертекса)
    for (unsigned int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }
    
    auto mesh = std::make_shared<Mesh>(vertices, indices);
    mesh->SetName("GridLines");
    mesh->SetDrawMode(GL_LINES);   // нужна доработка Mesh (см. ниже)
    return mesh;
}
