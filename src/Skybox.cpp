#include "Skybox.h"
#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>

static const float SKYBOX_VERTICES[] = {
    -1, -1,  1,   1, -1,  1,   1,  1,  1,
     1,  1,  1,  -1,  1,  1,  -1, -1,  1,
    -1, -1, -1,  -1,  1, -1,   1,  1, -1,
     1,  1, -1,   1, -1, -1,  -1, -1, -1,
    -1,  1, -1,  -1,  1,  1,   1,  1,  1,
     1,  1,  1,   1,  1, -1,  -1,  1, -1,
    -1, -1, -1,   1, -1, -1,   1, -1,  1,
     1, -1,  1,  -1, -1,  1,  -1, -1, -1,
     1, -1, -1,   1,  1, -1,   1,  1,  1,
     1,  1,  1,   1, -1,  1,   1, -1, -1,
    -1, -1, -1,  -1, -1,  1,  -1,  1,  1,
    -1,  1,  1,  -1,  1, -1,  -1, -1, -1
};

Skybox::Skybox() {
    m_shader = new Shader("shaders/sky.vert", "shaders/sky.frag");

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SKYBOX_VERTICES), SKYBOX_VERTICES, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

Skybox::~Skybox() {
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    delete m_shader;
}

void Skybox::draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& sunDir) {
    glDepthFunc(GL_LEQUAL);
    m_shader->use();

    glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));
    m_shader->setMat4("uView", viewNoTrans);
    m_shader->setMat4("uProjection", projection);
    m_shader->setVec3("uSunDir", sunDir);

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthFunc(GL_LESS);
}