#version 330 core
in vec2 TexCoords;

uniform sampler2D diffuseTexture;
uniform bool hasDiffuseTexture;
uniform bool alphaTestShadows;
uniform float alphaCutoff = 0.5;

void main() {
    if (alphaTestShadows && hasDiffuseTexture) {
        float alpha = texture(diffuseTexture, TexCoords).a;
        if (alpha < alphaCutoff) discard;
    }
    // Если не отброшен, просто пишем глубину (цвет не важен)
}
