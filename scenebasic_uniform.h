#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"
#include "helper/glslprogram.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class SceneBasic_Uniform : public Scene
{
private:
    GLSLProgram prog;
    GLSLProgram skyboxProg;

    // Shadow mapping
    GLSLProgram shadowProg;

    GLuint depthMapFBO;
    GLuint depthMap;

    const unsigned int SHADOW_WIDTH = 2048;
    const unsigned int SHADOW_HEIGHT = 2048;

    GLuint vaoHandle;
    int width, height;

    // Simple bloom / post-processing
    GLSLProgram screenProg;

    GLuint sceneFBO;
    GLuint sceneColorTexture;
    GLuint sceneDepthRBO;

    GLuint screenQuadVAO;
    GLuint screenQuadVBO;

    bool bloomEnabled;
    bool fogEnabled;
    float fogAmount;

    // Player camera
    glm::vec3 cameraPos;
    float cameraYaw;
    float cameraPitch;

    // Mouse look
    bool firstMouse;
    double lastMouseX;
    double lastMouseY;

    // Smooth walking
    glm::vec3 playerVelocity;

    // Sprint and camera bob
    float bobTimer;
    float cameraBobOffset;

    // Stamina system
    float stamina;
    float maxStamina;
    bool isSprintingNow;
    float staminaBarAlpha;
    float staminaBarVisibleTimer;

    // Interaction system
    bool lookingAtCat;
    bool interactionMessageVisible;
    float interactionMessageTimer;

    glm::mat4 rotationMatrix;

    std::vector<float> statuePositions;
    std::vector<float> statueTexcoords;
    std::vector<float> statueNormals;
    int statueVertexCount;

    GLuint statueTexture;
    GLuint planeTexture;

    GLuint planeVao;
    GLuint planeVboPos;
    GLuint planeVboTex;
    GLuint planeVboNorm;
    int planeVertexCount;

    GLuint skyboxVAO;
    GLuint skyboxVBO;
    GLuint skyboxTexture;

    bool light1Enabled;
    bool light2Enabled;
    glm::vec3 light1ColorOriginal;
    glm::vec3 light2ColorOriginal;

public:
    SceneBasic_Uniform();

    void initScene() override;
    void update(float t) override;
    void render() override;
    void resize(int, int) override;

private:
    void compile();
    GLuint loadCubemap(const std::vector<std::string>& faces);
    void renderSceneDepth(const glm::mat4& lightSpaceMatrix);
};

#endif