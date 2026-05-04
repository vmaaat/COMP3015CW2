#include "scenebasic_uniform.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "helper/tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "helper/stb/stb_image.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>

#include "helper/glutils.h"
#include <GLFW/glfw3.h>

using std::string;
using std::cerr;
using std::endl;
using glm::vec3;

// -------------------------------------------------------------
// Constructor
// -------------------------------------------------------------
SceneBasic_Uniform::SceneBasic_Uniform()
    : vaoHandle(0),
    width(800),
    height(600),
    cameraPos(0.0f, 0.2f, 4.0f),
    cameraYaw(-90.0f),
    cameraPitch(0.0f),
    firstMouse(true),
    lastMouseX(400.0),
    lastMouseY(300.0),
    playerVelocity(0.0f, 0.0f, 0.0f),
    bobTimer(0.0f),
    cameraBobOffset(0.0f),
    stamina(100.0f),
    maxStamina(100.0f),
    isSprintingNow(false),
    staminaBarAlpha(0.0f),
    staminaBarVisibleTimer(0.0f),
    lookingAtCat(false),
    interactionMessageVisible(false),
    interactionMessageTimer(0.0f),
    rotationMatrix(1.0f),
    statueVertexCount(0),
    statueTexture(0),
    planeTexture(0),
    planeVao(0),
    planeVboPos(0),
    planeVboTex(0),
    planeVboNorm(0),
    planeVertexCount(0),
    skyboxVAO(0),
    skyboxVBO(0),
    skyboxTexture(0),
    depthMapFBO(0),
    depthMap(0),
    sceneFBO(0),
    sceneColorTexture(0),
    sceneDepthRBO(0),
    screenQuadVAO(0),
    screenQuadVBO(0),
    bloomEnabled(true),
    fogEnabled(false),
    fogAmount(0.0f),
    light1Enabled(true),
    light2Enabled(true),
    light1ColorOriginal(2.0f, 1.8f, 1.2f),
    light2ColorOriginal(0.6f, 0.8f, 2.0f)
{
}

// -------------------------------------------------------------
// Cubemap loader
// -------------------------------------------------------------
GLuint SceneBasic_Uniform::loadCubemap(const std::vector<std::string>& faces)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    int widthImg, heightImg, channels;

    stbi_set_flip_vertically_on_load(false);

    for (size_t i = 0; i < faces.size(); ++i)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &widthImg, &heightImg, &channels, 0);

        if (data)
        {
            GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(i),
                0,
                format,
                widthImg,
                heightImg,
                0,
                format,
                GL_UNSIGNED_BYTE,
                data);

            stbi_image_free(data);
        }
        else
        {
            cerr << "Failed to load cubemap face: " << faces[i] << endl;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return texID;
}

// -------------------------------------------------------------
// Init Scene
// -------------------------------------------------------------
void SceneBasic_Uniform::initScene()
{
    compile();

    std::cout << std::endl;
    prog.printActiveUniforms();

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.15f, 0.16f, 0.22f, 1.0f);

    // ---------------------------------------------------------
    // Create shadow map framebuffer
    // ---------------------------------------------------------
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Shadow framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ---------------------------------------------------------
    // Create scene framebuffer for simple bloom/post-processing
    // ---------------------------------------------------------
    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    // Colour texture
    glGenTextures(1, &sceneColorTexture);
    glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, sceneColorTexture, 0);

    // Depth renderbuffer
    glGenRenderbuffers(1, &sceneDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER, sceneDepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Scene framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ---------------------------------------------------------
    // Create fullscreen quad for post-processing
    // ---------------------------------------------------------
    float quadVertices[] = {
        // positions   // texcoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &screenQuadVAO);
    glGenBuffers(1, &screenQuadVBO);

    glBindVertexArray(screenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, screenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    GLFWwindow* window = glfwGetCurrentContext();
    if (window)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    // ---------------------------------------------------------
    // Load cat statue model
    // ---------------------------------------------------------
    {
        std::string inputFile = "media/models/statueCat.obj";

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        bool ret = tinyobj::LoadObj(
            &attrib,
            &shapes,
            &materials,
            &warn,
            &err,
            inputFile.c_str(),
            "media/models/"
        );

        if (!warn.empty())
            std::cout << "tinyobj warning: " << warn << std::endl;

        if (!err.empty())
            std::cerr << "tinyobj error: " << err << std::endl;

        if (!ret)
        {
            std::cerr << "Failed to load OBJ file: " << inputFile << std::endl;
            exit(EXIT_FAILURE);
        }

        statuePositions.clear();
        statueTexcoords.clear();
        statueNormals.clear();

        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                int vIndex = 3 * index.vertex_index;

                statuePositions.push_back(attrib.vertices[vIndex + 0]);
                statuePositions.push_back(attrib.vertices[vIndex + 1]);
                statuePositions.push_back(attrib.vertices[vIndex + 2]);

                if (!attrib.texcoords.empty() && index.texcoord_index >= 0)
                {
                    int tIndex = 2 * index.texcoord_index;

                    statueTexcoords.push_back(attrib.texcoords[tIndex + 0]);
                    statueTexcoords.push_back(1.0f - attrib.texcoords[tIndex + 1]);
                }
                else
                {
                    statueTexcoords.push_back(0.0f);
                    statueTexcoords.push_back(0.0f);
                }

                if (!attrib.normals.empty() && index.normal_index >= 0)
                {
                    int nIndex = 3 * index.normal_index;

                    statueNormals.push_back(attrib.normals[nIndex + 0]);
                    statueNormals.push_back(attrib.normals[nIndex + 1]);
                    statueNormals.push_back(attrib.normals[nIndex + 2]);
                }
                else
                {
                    statueNormals.push_back(0.0f);
                    statueNormals.push_back(0.0f);
                    statueNormals.push_back(1.0f);
                }
            }
        }

        statueVertexCount = static_cast<int>(statuePositions.size() / 3);

        std::cout << "Loaded statueCat.obj with "
            << statueVertexCount << " vertices." << std::endl;
    }

    // ---------------------------------------------------------
    // Load cat statue texture
    // ---------------------------------------------------------
    {
        int texWidth, texHeight, texChannels;
        stbi_set_flip_vertically_on_load(true);

        unsigned char* data = stbi_load("media/models/staue1Color.png",
            &texWidth,
            &texHeight,
            &texChannels,
            0);

        if (!data)
        {
            std::cerr << "Failed to load texture image: media/models/staue1Color.png" << std::endl;
        }
        else
        {
            glGenTextures(1, &statueTexture);
            glBindTexture(GL_TEXTURE_2D, statueTexture);

            GLenum format = (texChannels == 4) ? GL_RGBA : GL_RGB;

            glTexImage2D(GL_TEXTURE_2D,
                0,
                format,
                texWidth,
                texHeight,
                0,
                format,
                GL_UNSIGNED_BYTE,
                data);

            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);

            std::cout << "Loaded texture staue1Color.png ("
                << texWidth << "x" << texHeight << ")" << std::endl;
        }
    }

    // ---------------------------------------------------------
    // Create statue VAO/VBOs
    // ---------------------------------------------------------
    {
        GLuint vboHandles[3];
        glGenBuffers(3, vboHandles);

        GLuint positionBufferHandle = vboHandles[0];
        GLuint texcoordBufferHandle = vboHandles[1];
        GLuint normalBufferHandle = vboHandles[2];

        glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(statuePositions.size() * sizeof(float)),
            statuePositions.data(),
            GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, texcoordBufferHandle);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(statueTexcoords.size() * sizeof(float)),
            statueTexcoords.data(),
            GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, normalBufferHandle);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(statueNormals.size() * sizeof(float)),
            statueNormals.data(),
            GL_STATIC_DRAW);

        glGenVertexArrays(1, &vaoHandle);
        glBindVertexArray(vaoHandle);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

#if defined(__APPLE__)
        glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);

        glBindBuffer(GL_ARRAY_BUFFER, texcoordBufferHandle);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);

        glBindBuffer(GL_ARRAY_BUFFER, normalBufferHandle);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
#else
        glBindVertexBuffer(0, positionBufferHandle, 0, sizeof(GLfloat) * 3);
        glBindVertexBuffer(1, texcoordBufferHandle, 0, sizeof(GLfloat) * 2);
        glBindVertexBuffer(2, normalBufferHandle, 0, sizeof(GLfloat) * 3);

        glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribBinding(0, 0);

        glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribBinding(1, 1);

        glVertexAttribFormat(2, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribBinding(2, 2);
#endif

        glBindVertexArray(0);
    }

    // ---------------------------------------------------------
    // Load floor texture
    // ---------------------------------------------------------
    {
        int texWidth, texHeight, texChannels;
        stbi_set_flip_vertically_on_load(true);

        unsigned char* data = stbi_load("media/models/grass.png",
            &texWidth,
            &texHeight,
            &texChannels,
            0);

        if (!data)
        {
            std::cerr << "Failed to load floor texture grass.png" << std::endl;
        }
        else
        {
            glGenTextures(1, &planeTexture);
            glBindTexture(GL_TEXTURE_2D, planeTexture);

            GLenum format = (texChannels == 4) ? GL_RGBA : GL_RGB;

            glTexImage2D(GL_TEXTURE_2D,
                0,
                format,
                texWidth,
                texHeight,
                0,
                format,
                GL_UNSIGNED_BYTE,
                data);

            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);

            std::cout << "Loaded floor texture grass.png ("
                << texWidth << "x" << texHeight << ")" << std::endl;
        }
    }

    // ---------------------------------------------------------
    // Create larger walkable ground plane with raised bumpy edges
    // ---------------------------------------------------------
    {
        std::vector<float> planePos;
        std::vector<float> planeTex;
        std::vector<float> planeNorm;

        const int gridSize = 80;
        const float mapSize = 30.0f;
        const float halfSize = mapSize / 2.0f;
        const float step = mapSize / gridSize;

        auto getHeight = [](float x, float z)
            {
                float edgeStart = 8.0f;
                float distFromCentre = glm::max(abs(x), abs(z));

                if (distFromCentre < edgeStart)
                    return 0.0f;

                float edgeAmount = (distFromCentre - edgeStart) / 6.0f;
                if (edgeAmount > 1.0f) edgeAmount = 1.0f;

                float bump =
                    sin(x * 0.9f) * 0.35f +
                    cos(z * 1.4f) * 0.25f +
                    sin((x + z) * 0.7f) * 0.30f +
                    cos((x - z) * 1.1f) * 0.20f;

                return edgeAmount * (1.4f + bump);
            };

        auto addVertex = [&](float x, float z)
            {
                float y = getHeight(x, z);

                planePos.push_back(x);
                planePos.push_back(y);
                planePos.push_back(z);

                planeTex.push_back((x + halfSize) / 2.0f);
                planeTex.push_back((z + halfSize) / 2.0f);

                planeNorm.push_back(0.0f);
                planeNorm.push_back(1.0f);
                planeNorm.push_back(0.0f);
            };

        for (int z = 0; z < gridSize; ++z)
        {
            for (int x = 0; x < gridSize; ++x)
            {
                float x0 = -halfSize + x * step;
                float z0 = -halfSize + z * step;
                float x1 = x0 + step;
                float z1 = z0 + step;

                addVertex(x0, z0);
                addVertex(x1, z0);
                addVertex(x1, z1);

                addVertex(x0, z0);
                addVertex(x1, z1);
                addVertex(x0, z1);
            }
        }

        planeVertexCount = static_cast<int>(planePos.size() / 3);

        glGenVertexArrays(1, &planeVao);
        glGenBuffers(1, &planeVboPos);
        glGenBuffers(1, &planeVboTex);
        glGenBuffers(1, &planeVboNorm);

        glBindVertexArray(planeVao);

        glBindBuffer(GL_ARRAY_BUFFER, planeVboPos);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(planePos.size() * sizeof(float)),
            planePos.data(),
            GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

        glBindBuffer(GL_ARRAY_BUFFER, planeVboTex);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(planeTex.size() * sizeof(float)),
            planeTex.data(),
            GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

        glBindBuffer(GL_ARRAY_BUFFER, planeVboNorm);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(planeNorm.size() * sizeof(float)),
            planeNorm.data(),
            GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

        glBindVertexArray(0);
    }

    // ---------------------------------------------------------
    // Create skybox cube geometry
    // ---------------------------------------------------------
    {
        float skyboxVertices[] = {
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };

        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);

        glBindVertexArray(skyboxVAO);

        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER,
            sizeof(skyboxVertices),
            skyboxVertices,
            GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glBindVertexArray(0);
    }

    // ---------------------------------------------------------
    // Load skybox cubemap textures
    // ---------------------------------------------------------
    {
        std::vector<std::string> faces = {
            "media/skybox/skybox_right.png",
            "media/skybox/skybox_left.png",
            "media/skybox/skybox_top.png",
            "media/skybox/skybox_bottom.png",
            "media/skybox/skybox_front.png",
            "media/skybox/skybox_back.png"
        };

        skyboxTexture = loadCubemap(faces);

        skyboxProg.use();

        GLuint skyboxHandle = skyboxProg.getHandle();
        GLint skyboxLoc = glGetUniformLocation(skyboxHandle, "skybox");

        if (skyboxLoc != -1)
        {
            glUniform1i(skyboxLoc, 0);
        }
    }

    // ---------------------------------------------------------
    // Set main texture sampler
    // ---------------------------------------------------------
    prog.use();

    GLuint programHandle = prog.getHandle();
    GLint texLoc = glGetUniformLocation(programHandle, "diffuseTex");

    if (texLoc != -1)
    {
        glUniform1i(texLoc, 0);
    }
}

// -------------------------------------------------------------
// Compile shaders
// -------------------------------------------------------------
void SceneBasic_Uniform::compile()
{
    try
    {
        prog.compileShader("shader/basic_uniform.vert");
        prog.compileShader("shader/basic_uniform.frag");
        prog.link();

        skyboxProg.compileShader("shader/skybox.vert");
        skyboxProg.compileShader("shader/skybox.frag");
        skyboxProg.link();

        shadowProg.compileShader("shader/shadow_map.vert");
        shadowProg.compileShader("shader/shadow_map.frag");
        shadowProg.link();

        screenProg.compileShader("shader/screen_quad.vert");
        screenProg.compileShader("shader/simple_bloom.frag");
        screenProg.link();

        prog.use();
    }
    catch (GLSLProgramException& e)
    {
        cerr << e.what() << endl;
        exit(EXIT_FAILURE);
    }
}

void SceneBasic_Uniform::renderSceneDepth(const glm::mat4& lightSpaceMatrix)
{
    shadowProg.use();

    GLuint programHandle = shadowProg.getHandle();

    GLint modelLoc = glGetUniformLocation(programHandle, "model");
    GLint lightSpaceLoc = glGetUniformLocation(programHandle, "lightSpaceMatrix");

    if (lightSpaceLoc != -1)
        glUniformMatrix4fv(lightSpaceLoc, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

    // -------- Floor --------
    glm::mat4 planeModel = glm::mat4(1.0f);
    planeModel = glm::translate(planeModel, glm::vec3(0.0f, -0.55f, 0.0f));

    if (modelLoc != -1)
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &planeModel[0][0]);

    glBindVertexArray(planeVao);
    glDrawArrays(GL_TRIANGLES, 0, planeVertexCount);

    // -------- Cat --------
    glm::mat4 catModel = glm::mat4(1.0f);
    catModel = glm::scale(catModel, glm::vec3(0.15f));
    catModel = glm::translate(catModel, glm::vec3(0.0f, -3.5f, 0.0f));

    if (modelLoc != -1)
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &catModel[0][0]);

    glBindVertexArray(vaoHandle);
    glDrawArrays(GL_TRIANGLES, 0, statueVertexCount);

    glBindVertexArray(0);
}

// -------------------------------------------------------------
// Update player movement and light toggles
// -------------------------------------------------------------
void SceneBasic_Uniform::update(float t)
{
    GLFWwindow* window = glfwGetCurrentContext();
    if (!window) return;

    static float lastTime = 0.0f;
    float deltaTime = t - lastTime;
    lastTime = t;

    if (deltaTime <= 0.0f || deltaTime > 0.1f)
        deltaTime = 0.016f;

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    if (firstMouse)
    {
        lastMouseX = mouseX;
        lastMouseY = mouseY;
        firstMouse = false;
    }

    double offsetX = mouseX - lastMouseX;
    double offsetY = lastMouseY - mouseY;

    lastMouseX = mouseX;
    lastMouseY = mouseY;

    float mouseSensitivity = 0.08f;

    cameraYaw += static_cast<float>(offsetX) * mouseSensitivity;
    cameraPitch += static_cast<float>(offsetY) * mouseSensitivity;

    if (cameraPitch > 89.0f)
        cameraPitch = 89.0f;

    if (cameraPitch < -40.0f)
        cameraPitch = -40.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.y = sin(glm::radians(cameraPitch));
    front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front = glm::normalize(front);

    // ---------------------------------------------------------
    // Interaction check: is player looking at cat statue?
    // ---------------------------------------------------------
    glm::vec3 catPosition(0.0f, 0.0f, 0.0f);
    glm::vec3 toCat = glm::normalize(catPosition - cameraPos);

    float lookDot = glm::dot(front, toCat);
    float distanceToCat = glm::length(catPosition - cameraPos);

    // Higher dot = more directly looking at it
    lookingAtCat = (lookDot > 0.985f && distanceToCat < 1.8f);

    // Press E to interact
    static bool eWasPressed = false;
    bool ePressed = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;

    if (lookingAtCat && ePressed && !eWasPressed)
    {
        fogEnabled = !fogEnabled;

        interactionMessageVisible = true;
        interactionMessageTimer = 2.0f;

        if (fogEnabled)
            std::cout << "The statue summons fog" << std::endl;
        else
            std::cout << "The fog clears" << std::endl;
    }

    eWasPressed = ePressed;

    // Message timer
    if (interactionMessageVisible)
    {
        interactionMessageTimer -= deltaTime;

        if (interactionMessageTimer <= 0.0f)
        {
            interactionMessageTimer = 0.0f;
            interactionMessageVisible = false;
        }
    }

    glm::vec3 walkForward = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    glm::vec3 right = glm::normalize(glm::cross(walkForward, glm::vec3(0.0f, 1.0f, 0.0f)));

    glm::vec3 inputDirection(0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        inputDirection += walkForward;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        inputDirection -= walkForward;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        inputDirection -= right;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        inputDirection += right;

    if (glm::length(inputDirection) > 0.0f)
        inputDirection = glm::normalize(inputDirection);

    bool wantsToSprint = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

    bool isMoving = glm::length(inputDirection) > 0.0f;

    // Only allow sprinting if stamina is above a small threshold
    isSprintingNow = wantsToSprint && isMoving && stamina > 5.0f;

    float walkSpeed = 2.0f;
    float sprintSpeed = 5.0f;
    float maxSpeed = isSprintingNow ? sprintSpeed : walkSpeed;

    // Stamina drain/regeneration
    static float staminaRegenDelayTimer = 0.0f;

    float staminaDrainRate = 35.0f;  
    float staminaRegenRate = 25.0f;
    float staminaRegenDelay = 1.5f;   

    if (isSprintingNow)
    {
        stamina -= staminaDrainRate * deltaTime;
        staminaRegenDelayTimer = 0.0f;

        if (stamina <= 0.0f)
        {
            stamina = 0.0f;
            isSprintingNow = false;
            maxSpeed = walkSpeed;
        }
    }
    else
    {
        if (!wantsToSprint)
        {
            staminaRegenDelayTimer += deltaTime;

            if (staminaRegenDelayTimer >= staminaRegenDelay)
            {
                stamina += staminaRegenRate * deltaTime;

                if (stamina > maxStamina)
                    stamina = maxStamina;
            }
        }
    }

    float acceleration = isSprintingNow ? 10.0f : 8.0f;
    float friction = 7.0f;

    glm::vec3 targetVelocity = inputDirection * maxSpeed;

    if (glm::length(inputDirection) > 0.0f)
        playerVelocity = glm::mix(playerVelocity, targetVelocity, acceleration * deltaTime);
    else
        playerVelocity = glm::mix(playerVelocity, glm::vec3(0.0f), friction * deltaTime);

    cameraPos += playerVelocity * deltaTime;

    // ---------------------------------------------------------
    // Simple collision around cat statue
    // ---------------------------------------------------------
    glm::vec3 catCollisionCentre(0.0f, 0.2f, 0.0f);
    float catCollisionRadius = 0.65f;

    glm::vec3 playerFlat(cameraPos.x, 0.2f, cameraPos.z);
    glm::vec3 toPlayer = playerFlat - catCollisionCentre;

    float distanceFromCat = glm::length(toPlayer);

    if (distanceFromCat < catCollisionRadius)
    {
        glm::vec3 pushDirection = glm::normalize(toPlayer);
        playerFlat = catCollisionCentre + pushDirection * catCollisionRadius;

        cameraPos.x = playerFlat.x;
        cameraPos.z = playerFlat.z;
    }

    // ---------------------------------------------------------
    // Camera bobbing while walking/sprinting
    // ---------------------------------------------------------
    float horizontalSpeed = glm::length(glm::vec3(playerVelocity.x, 0.0f, playerVelocity.z));

    if (horizontalSpeed > 0.05f)
    {
        float bobSpeed = isSprintingNow ? 11.0f : 8.0f;
        float bobAmount = isSprintingNow ? 0.045f : 0.03f;

        bobTimer += deltaTime * bobSpeed;
        cameraBobOffset = sin(bobTimer) * bobAmount;
    }
    else
    {
        cameraBobOffset = glm::mix(cameraBobOffset, 0.0f, 8.0f * deltaTime);
        bobTimer = 0.0f;
    }

    cameraPos.y = 0.2f + cameraBobOffset;

    const float playerBoundary = 7.5f;

    if (cameraPos.x > playerBoundary) cameraPos.x = playerBoundary;
    if (cameraPos.x < -playerBoundary) cameraPos.x = -playerBoundary;
    if (cameraPos.z > playerBoundary) cameraPos.z = playerBoundary;
    if (cameraPos.z < -playerBoundary) cameraPos.z = -playerBoundary;

    // Toggle bloom on/off with B
    static bool bWasPressed = false;
    bool bPressed = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;

    if (bPressed && !bWasPressed)
    {
        bloomEnabled = !bloomEnabled;

        if (bloomEnabled)
            std::cout << "Bloom enabled" << std::endl;
        else
            std::cout << "Bloom disabled" << std::endl;
    }

    bWasPressed = bPressed;

    // Smooth fog fade in/out
    float targetFogAmount = fogEnabled ? 1.0f : 0.0f;
    fogAmount = glm::mix(fogAmount, targetFogAmount, 2.0f * deltaTime);

    if (fogAmount < 0.01f)
        fogAmount = 0.0f;

    if (fogAmount > 0.99f)
        fogAmount = 1.0f;

    // ---------------------------------------------------------
    // Stamina bar fade behaviour
    // ---------------------------------------------------------
    if (isSprintingNow || stamina < maxStamina)
    {
        staminaBarVisibleTimer = 2.5f;
        staminaBarAlpha += 5.0f * deltaTime;
    }
    else
    {
        staminaBarVisibleTimer -= deltaTime;

        if (staminaBarVisibleTimer <= 0.0f)
        {
            staminaBarVisibleTimer = 0.0f;
            staminaBarAlpha -= 1.8f * deltaTime;
        }
    }

    if (staminaBarAlpha > 1.0f) staminaBarAlpha = 1.0f;
    if (staminaBarAlpha < 0.0f) staminaBarAlpha = 0.0f;
}

// -------------------------------------------------------------
// Render
// -------------------------------------------------------------
void SceneBasic_Uniform::render()
{
    
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = (height > 0)
        ? static_cast<float>(width) / static_cast<float>(height)
        : 1.0f;

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

    // Build first-person/player camera direction
    glm::vec3 front;
    front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.y = sin(glm::radians(cameraPitch));
    front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front = glm::normalize(front);

    vec3 eye = cameraPos;
    vec3 target = cameraPos + front;
    vec3 up(0.0f, 1.0f, 0.0f);

    glm::mat4 view = glm::lookAt(eye, target, up);

    // ---------------------------------------------------------
    // Shadow pass: render scene from light's point of view
    // ---------------------------------------------------------
    float time = glfwGetTime() * 0.01f;

    glm::vec3 lightPos(
        4.0f * sin(time),
        5.0f,
        4.0f * cos(time)
    );

    glm::mat4 lightProjection = glm::ortho(
        -15.0f, 15.0f,
        -15.0f, 15.0f,
        1.0f, 20.0f
    );

    glm::mat4 lightView = glm::lookAt(
        lightPos,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    renderSceneDepth(lightSpaceMatrix);

    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ---------------------------------------------------------
    // 1. Draw skybox
    // ---------------------------------------------------------
    glDepthFunc(GL_LEQUAL);

    skyboxProg.use();

    GLuint skyboxHandle = skyboxProg.getHandle();
    glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));

    GLint viewLoc = glGetUniformLocation(skyboxHandle, "view");
    GLint projLoc = glGetUniformLocation(skyboxHandle, "projection");

    if (viewLoc != -1)
    {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &viewNoTrans[0][0]);
    }

    if (projLoc != -1)
    {
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &proj[0][0]);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);

    // ---------------------------------------------------------
    // 2. Draw floor and cat
    // ---------------------------------------------------------
    prog.use();

    GLuint programHandle = prog.getHandle();

    GLint diffuseLoc = glGetUniformLocation(programHandle, "diffuseTex");
    GLint shadowMapLoc = glGetUniformLocation(programHandle, "shadowMap");

    if (diffuseLoc != -1)
        glUniform1i(diffuseLoc, 0);

    if (shadowMapLoc != -1)
        glUniform1i(shadowMapLoc, 1);

    GLint viewPosLoc = glGetUniformLocation(programHandle, "viewPos");

    if (viewPosLoc != -1)
    {
        glUniform3f(viewPosLoc, eye.x, eye.y, eye.z);
    }

    GLint fogEnabledLoc = glGetUniformLocation(programHandle, "fogEnabled");
    GLint fogColorLoc = glGetUniformLocation(programHandle, "fogColor");
    GLint fogDensityLoc = glGetUniformLocation(programHandle, "fogDensity");
    GLint fogAmountLoc = glGetUniformLocation(programHandle, "fogAmount");

    if (fogEnabledLoc != -1)
        glUniform1i(fogEnabledLoc, fogEnabled ? 1 : 0);

    if (fogColorLoc != -1)
        glUniform3f(fogColorLoc, 0.28f, 0.27f, 0.33f);

    if (fogDensityLoc != -1)
        glUniform1f(fogDensityLoc, 0.11f);

    if (fogAmountLoc != -1)
        glUniform1f(fogAmountLoc, fogAmount);

    // Light 1 follows player camera
    GLint l1PosLoc = glGetUniformLocation(programHandle, "light1.position");
    GLint l1ColLoc = glGetUniformLocation(programHandle, "light1.color");

    if (l1PosLoc != -1)
    {
        vec3 light1Pos = eye + vec3(0.0f, 0.5f, 0.0f);
        glUniform3f(l1PosLoc, light1Pos.x, light1Pos.y, light1Pos.z);
    }

    if (l1ColLoc != -1)
    {
        vec3 col = light1Enabled ? light1ColorOriginal : vec3(0.0f);
        glUniform3f(l1ColLoc, col.x, col.y, col.z);
    }

    // Light 2 stays fixed in the scene
    GLint l2PosLoc = glGetUniformLocation(programHandle, "light2.position");
    GLint l2ColLoc = glGetUniformLocation(programHandle, "light2.color");

    if (l2PosLoc != -1)
    {
        glUniform3f(l2PosLoc,
            lightPos.x,
            lightPos.y,
            lightPos.z
        );
    }

    if (l2ColLoc != -1)
    {
        vec3 col = light2Enabled ? light2ColorOriginal : vec3(0.0f);
        glUniform3f(l2ColLoc, col.x, col.y, col.z);
    }

    GLint modelLoc = glGetUniformLocation(programHandle, "model");
    GLint viewMainLoc = glGetUniformLocation(programHandle, "view");
    GLint projMainLoc = glGetUniformLocation(programHandle, "projection");
    GLint lightSpaceLoc = glGetUniformLocation(programHandle, "lightSpaceMatrix");

    if (viewMainLoc != -1)
        glUniformMatrix4fv(viewMainLoc, 1, GL_FALSE, &view[0][0]);

    if (projMainLoc != -1)
        glUniformMatrix4fv(projMainLoc, 1, GL_FALSE, &proj[0][0]);

    if (lightSpaceLoc != -1)
        glUniformMatrix4fv(lightSpaceLoc, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

    glm::mat4 planeModel = glm::mat4(1.0f);

    const float planeY = -0.55f;
    planeModel = glm::translate(planeModel, glm::vec3(0.0f, planeY, 0.0f));

    if (modelLoc != -1)
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &planeModel[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, planeTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(planeVao);
    glDrawArrays(GL_TRIANGLES, 0, planeVertexCount);
    glBindVertexArray(0);

    // Draw cat statue
    glm::mat4 catModel = glm::mat4(1.0f);

    const float catScale = 0.15f;
    const float catY = -3.5f;

    catModel = glm::scale(catModel, glm::vec3(catScale, catScale, catScale));
    catModel = glm::translate(catModel, glm::vec3(0.0f, catY, 0.0f));

    if (modelLoc != -1)
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &catModel[0][0]);

    glBindTexture(GL_TEXTURE_2D, statueTexture);

    glBindVertexArray(vaoHandle);
    glDrawArrays(GL_TRIANGLES, 0, statueVertexCount);
    glBindVertexArray(0);

    // ---------------------------------------------------------
    // 3. Draw stamina bar overlay
    // ---------------------------------------------------------
    if (staminaBarAlpha > 0.01f)
    {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);

        int barWidth = 300;
        int barHeight = 15;
        int barX = (width - barWidth) / 2;
        int barY = 30;

        // Background colour fades with alpha
        float bg = 0.05f * staminaBarAlpha;

        glScissor(barX, barY, barWidth, barHeight);
        glClearColor(bg, bg, bg, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Fill width clamps properly to zero
        float staminaPercent = stamina / maxStamina;

        if (staminaPercent < 0.0f) staminaPercent = 0.0f;
        if (staminaPercent > 1.0f) staminaPercent = 1.0f;

        int fillWidth = static_cast<int>(barWidth * staminaPercent);

        if (stamina <= 1.0f)
        {
            fillWidth = 0;
        }

        if (fillWidth > 0)
        {
            int centerX = barX + barWidth / 2;
            int halfWidth = fillWidth / 2;

            glScissor(centerX - halfWidth, barY, fillWidth, barHeight);

            // Red warning when stamina is low
            float warningAmount = 0.0f;

            if (staminaPercent < 0.25f)
            {
                warningAmount = 1.0f - (staminaPercent / 0.25f);
            }

            // Flash strength
            float flash = 0.5f + 0.5f * sin(glfwGetTime() * 12.0f);
            float redStrength = warningAmount * flash;

            // Blend from white to flashing red
            float r = 1.0f * staminaBarAlpha;
            float g = (1.0f - redStrength) * staminaBarAlpha;
            float b = (1.0f - redStrength) * staminaBarAlpha;

            glClearColor(r, g, b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        glDisable(GL_SCISSOR_TEST);
        glEnable(GL_DEPTH_TEST);

        glClearColor(0.15f, 0.16f, 0.22f, 1.0f);
    }

    // ---------------------------------------------------------
    // 4. Draw interaction reticle
    // ---------------------------------------------------------
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);

    int dotSize = lookingAtCat ? 10 : 5;
    int dotX = (width / 2) - (dotSize / 2);
    int dotY = (height / 2) - (dotSize / 2);

    // Centre dot colour
    if (lookingAtCat)
    {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    }

    glScissor(dotX, dotY, dotSize, dotSize);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.15f, 0.16f, 0.22f, 1.0f);

    // ---------------------------------------------------------
    // Final pass: draw fullscreen quad with bloom shader
    // ---------------------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);

    screenProg.use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneColorTexture);

    GLuint screenHandle = screenProg.getHandle();
    GLint texLoc = glGetUniformLocation(screenHandle, "sceneTexture");

    GLint bloomEnabledLoc = glGetUniformLocation(screenHandle, "bloomEnabled");

    if (bloomEnabledLoc != -1)
        glUniform1i(bloomEnabledLoc, bloomEnabled ? 1 : 0);

    if (texLoc != -1)
        glUniform1i(texLoc, 0);

    glBindVertexArray(screenQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

// -------------------------------------------------------------
// Resize
// -------------------------------------------------------------
void SceneBasic_Uniform::resize(int w, int h)
{
    width = w;
    height = h;

    glViewport(0, 0, w, h);
}