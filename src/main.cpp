#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xPos, double yPos);

void scroll_callback(GLFWwindow *window, double xOffset, double yOffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(const char *path);

unsigned int loadCubeMap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct ProgramState {
    bool ImGuiEnabled = true;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    bool spotlight = false;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);
    void LoadFromFile(std::string filename);
    glm::vec3 tempPosition=glm::vec3(0.0f, 2.0f, 0.0f);
    float tempScale=1.0f;
    float tempRotation=0.0f;
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n'
        << spotlight << '\n'
        << tempPosition.x << '\n'
        << tempPosition.y << '\n'
        << tempPosition.z << '\n'
        << tempScale << '\n'
        << tempRotation << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z
           >> spotlight
           >> tempPosition.x
           >> tempPosition.y
           >> tempPosition.z
           >> tempScale
           >> tempRotation;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Moonlit Retreat", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Im gui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    Shader objShader("resources/shaders/object_lighting.vs", "resources/shaders/object_lighting.fs");
    Shader waterShader("resources/shaders/water_blending.vs", "resources/shaders/water_blending.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader sourceShader("resources/shaders/light_source.vs", "resources/shaders/light_source.fs");
    Shader discardShader("resources/shaders/discard_shader.vs", "resources/shaders/discard_shader.fs");
    Shader waterfallShader("resources/shaders/waterfall_shader.vs", "resources/shaders/waterfall_shader.fs");
    Shader rippleShader("resources/shaders/ripple_shader.vs", "resources/shaders/ripple_shader.fs");

    // load models
    Model bard("resources/objects/sleepy_bard/sleepy_bard.obj");
    bard.SetShaderTextureNamePrefix("material.");
    Model island("resources/objects/island/island_with_decor.obj");
    island.SetShaderTextureNamePrefix("material.");
    Model mountain_island("resources/objects/mountain_island/mountain.obj");
    mountain_island.SetShaderTextureNamePrefix("material.");
    Model sand_terrain("resources/objects/sand_terrain/sand_terrain.obj");
    sand_terrain.SetShaderTextureNamePrefix("material.");
    Model support_beam("resources/objects/support_beam/support_beam.obj");
    support_beam.SetShaderTextureNamePrefix("material.");
    Model chinese_lantern("resources/objects/chinese_lantern/chinese_lantern.obj");
    chinese_lantern.SetShaderTextureNamePrefix("material.");
    Model boat("resources/objects/boat/boat.obj");
    boat.SetShaderTextureNamePrefix("material.");
    Model barrel("resources/objects/barrel/barrel.obj");
    barrel.SetShaderTextureNamePrefix("material.");
    Model cliffs("resources/objects/cliffs/cliffs.obj");
    cliffs.SetShaderTextureNamePrefix("material.");
    Model granite("resources/objects/granite/granite.obj");
    cliffs.SetShaderTextureNamePrefix("material.");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------

    float transparentVertices[] = {
            // positions         //normals         // texture Coords (swapped y coordinates because texture is flipped upside down)
             25.0f,  0.0f,  25.0f, 0.0f, 1.0f, 0.0f, 40.0f, 0.0f,
            -25.0f,  0.0f,  25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            -25.0f,  0.0f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 40.0f,

             25.0f,  0.0f,  25.0f, 0.0f, 1.0f, 0.0f, 40.0f, 0.0f,
            -25.0f,  0.0f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 40.0f,
             25.0f,  0.0f, -25.0f, 0.0f, 1.0f, 0.0f, 40.0f, 40.0f
    };

    float transparentVertices2[] = {
            // positions         // texture Coords
            0.0f, -0.5f,  0.0f,  0.0f,  0.0f,
            0.0f,  0.5f,  0.0f,  0.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  1.0f,

            0.0f, -0.5f,  0.0f,  0.0f,  0.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    float rippleVertices[] = {
            // positions         // texture Coords
            -0.5f, 0.0f, -0.5f, 0.0f,  0.0f,
            -0.5f, 0.0f,  0.5f, 0.0f,  1.0f,
             0.5f, 0.0f, -0.5f, 1.0f,  0.0f,
            -0.5f, 0.0f,  0.5f, 0.0f,  1.0f,
             0.5f, 0.0f, -0.5f, 1.0f,  0.0f,
             0.5f, 0.0f,  0.5f, 1.0f,  1.0f
    };

    float waterfallVertices[] = {
            // positions         // texture Coords
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f, //top right
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f, //bottom right
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f, //bottom left
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f  //top left
    };

    unsigned int waterfallIndices[] = {
            0, 1, 3,
            1, 2, 3
    };

    float skyboxVertices[] = {
            // positions
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

    // transparent VAO for water
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // transparent VAO for grass
    unsigned int transparentVAO2, transparentVBO2;
    glGenVertexArrays(1, &transparentVAO2);
    glGenBuffers(1, &transparentVBO2);
    glBindVertexArray(transparentVAO2);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices2), transparentVertices2, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // VAO for the ripples
    unsigned int rippleVAO, rippleVBO;
    glGenVertexArrays(1, &rippleVAO);
    glGenBuffers(1, &rippleVBO);
    glBindVertexArray(rippleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rippleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rippleVertices), rippleVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // waterfall VAO
    unsigned int waterfallVAO, waterfallVBO, waterfallEBO;
    glGenVertexArrays(1, &waterfallVAO);
    glGenBuffers(1, &waterfallVBO);
    glGenBuffers(1, &waterfallEBO);

    glBindVertexArray(waterfallVAO);

    glBindBuffer(GL_ARRAY_BUFFER, waterfallVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(waterfallVertices), waterfallVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterfallEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(waterfallIndices), waterfallIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)nullptr);

    // load textures
    // -------------
    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/water_dark.png").c_str());
    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/grass.png").c_str());
    unsigned int waterfallTexture = loadTexture(FileSystem::getPath("resources/textures/seamless waterfall.jpeg").c_str());
    unsigned int rippleTexture = loadTexture(FileSystem::getPath("resources/textures/ripple.jpg").c_str());


    // transparent window locations
    // --------------------------------
    vector<glm::vec3> waterSquares
            {
                    glm::vec3(-25.0f, 1.0f, -25.0f),
                    glm::vec3(-25.0f, 1.0f, 25.0f),
                    glm::vec3(25.0f, 1.0f, -25.0f),
                    glm::vec3(25.0f, 1.0f, 25.0f)
            };
    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/front.png"),
                    FileSystem::getPath("resources/textures/skybox/back.png"),
                    FileSystem::getPath("resources/textures/skybox/top.png"),
                    FileSystem::getPath("resources/textures/skybox/bottom.png"),
                    FileSystem::getPath("resources/textures/skybox/left.png"),
                    FileSystem::getPath("resources/textures/skybox/right.png")
            };

    vector<glm::vec3> vegetation
            {
                    glm::vec3(-1.5f, 1.5f, -0.48f),
                    glm::vec3( 1.5f, 1.5f, 0.51f),
                    glm::vec3( 0.0f, 1.5f, 0.7f),
                    glm::vec3(-0.7f, 1.5f, -2.3f),
                    glm::vec3 (1.0f, 1.5f, -1.2f),
                    glm::vec3 (-0.1f, 1.5f, -0.63f),
                    glm::vec3 (-1.75f, 1.5f, 1.0f),
                    glm::vec3 (-0.6f, 1.5f, -2.0f)
            };

    vector<glm::vec3> waterfall_tiles
            {
                    glm::vec3( -0.8f, 1.12f, 1.0f),
                    glm::vec3( -0.8f, 1.37f, 1.0f),
                    glm::vec3( -0.8f, 1.62f, 1.0f),
                    glm::vec3( -0.77f, 1.86f, 1.03f),
                    glm::vec3( -0.66f, 2.03f, 1.14f),
                    glm::vec3( -0.50f, 2.11f, 1.30f),
                    glm::vec3( -0.33f, 2.13f, 1.47f)
            };

    unsigned int cubeMapTexture = loadCubeMap(faces);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = (float)currentFrame - lastFrame;
        lastFrame = (float)currentFrame;

        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        objShader.use();
        objShader.setVec3("viewPos", programState->camera.Position);
        objShader.setFloat("material.shininess", 32.0f);
        objShader.setBool("celShading", true);
        skyboxShader.setBool("celShading", true);
        waterShader.setBool("celShading", true);
        sourceShader.setBool("celShading", true);

        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        objShader.setMat4("projection", projection);
        objShader.setMat4("view", view);

        // transformation matrices for the lanterns and the lights

        glm::mat4 transMat1 = glm::mat4(1.0f);
        transMat1 = glm::translate(transMat1, glm::vec3(1.05f, 1.95f, -0.46f));
        transMat1 = glm::scale(transMat1, glm::vec3(0.08f, 0.08f, 0.08f));
        transMat1 = glm::rotate(transMat1, sin(currentFrame*2)*glm::radians(60.0f), glm::vec3(0,0,1));
        transMat1 = glm::translate(transMat1, glm::vec3(0.0f, -3.0f, 0.0f));

        glm::mat4 transMat2 = glm::mat4(1.0f);
        transMat2 = glm::translate(transMat2, glm::vec3(-1.85f, 1.95f, 0.56f));
        transMat2 = glm::scale(transMat2, glm::vec3(0.08f, 0.08f, 0.08f));
        transMat2 = glm::rotate(transMat2, sin(0.6f+currentFrame*2)*glm::radians(60.0f), glm::vec3(0,0,1));
        transMat2 = glm::translate(transMat2, glm::vec3(0.0f, -3.0f, 0.0f));

        glm::mat4 baseMat1 = glm::mat4(1.0f);
        baseMat1 = glm::translate(transMat1, glm::vec3(1.05f, 1.95f, 0.06f));
        baseMat1 = glm::scale(baseMat1, glm::vec3(0.08f, 0.08f, 0.08f));
        glm::mat4 baseMat2 = glm::mat4(1.0f);
        baseMat2 = glm::translate(transMat2, glm::vec3(-1.85f, 1.95f, 1.16f));
        baseMat2 = glm::scale(baseMat2, glm::vec3(0.08f, 0.08f, 0.08f));


        glm::vec3 pos0 = transMat1 * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec3 pos1 = transMat2 * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        glm::vec3 basePos0 = baseMat1 *  glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec3 basePos1 = baseMat2 *  glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        glm::vec3 spotlight_vector1 = normalize(pos0 - basePos0);
        glm::vec3 spotlight_vector2 = normalize(pos1 - basePos1);

        // directional light

        objShader.setVec3("dirLight.direction", -1.0f, -0.2f, 0.0f);
        objShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.20f);
        objShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.6f);
        objShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.7f);

        // point light 1

        objShader.setVec3("pointLights[0].position", pos0);
        objShader.setVec3("pointLights[0].ambient", 0.10f, 0.05f, 0.05f);
        objShader.setVec3("pointLights[0].diffuse", 0.8f, 0.6f, 0.6f);
        objShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 0.0f);
        objShader.setFloat("pointLights[0].constant", 1.0f);
        objShader.setFloat("pointLights[0].linear", 0.09);
        objShader.setFloat("pointLights[0].quadratic", 0.032);

        // point light 2

        objShader.setVec3("pointLights[1].position", pos1);
        objShader.setVec3("pointLights[1].ambient", 0.10f, 0.05f, 0.05f);
        objShader.setVec3("pointLights[1].diffuse", 0.8f, 0.6f, 0.6f);
        objShader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 0.0f);
        objShader.setFloat("pointLights[1].constant", 1.0f);
        objShader.setFloat("pointLights[1].linear", 0.09);
        objShader.setFloat("pointLights[1].quadratic", 0.032);

        // spotLight 1

        objShader.setVec3("spotLights[0].position", basePos0);
        objShader.setVec3("spotLights[0].direction", spotlight_vector1);
        objShader.setVec3("spotLights[0].ambient", 0.0f, 0.0f, 0.0f);
        if(programState->spotlight) {
            objShader.setVec3("spotLights[0].diffuse", 1.0f, 1.0f, 1.0f);
            objShader.setVec3("spotLights[0].specular", 1.0f, 1.0f, 1.0f);
        }
        else{
            objShader.setVec3("spotLights[0].diffuse", 0.0f, 0.0f, 0.0f);
            objShader.setVec3("spotLights[0].specular", 0.0f, 0.0f, 0.0f);
        }
        objShader.setFloat("spotLights[0].constant", 1.0f);
        objShader.setFloat("spotLights[0].linear", 0.09);
        objShader.setFloat("spotLights[0].quadratic", 0.032);
        objShader.setFloat("spotLights[0].cutOff", glm::cos(glm::radians(2.5f)));
        objShader.setFloat("spotLights[0].outerCutOff", glm::cos(glm::radians(5.0f)));

        // spotLight 2

        objShader.setVec3("spotLights[1].position", basePos1);
        objShader.setVec3("spotLights[1].direction", spotlight_vector2);
        objShader.setVec3("spotLights[1].ambient", 0.0f, 0.0f, 0.0f);
        if(programState->spotlight) {
            objShader.setVec3("spotLights[1].diffuse", 1.0f, 1.0f, 1.0f);
            objShader.setVec3("spotLights[1].specular", 1.0f, 1.0f, 1.0f);
        }
        else{
            objShader.setVec3("spotLights[1].diffuse", 0.0f, 0.0f, 0.0f);
            objShader.setVec3("spotLights[1].specular", 0.0f, 0.0f, 0.0f);
        }
        objShader.setFloat("spotLights[1].constant", 1.0f);
        objShader.setFloat("spotLights[1].linear", 0.09);
        objShader.setFloat("spotLights[1].quadratic", 0.032);
        objShader.setFloat("spotLights[1].cutOff", glm::cos(glm::radians(2.5f)));
        objShader.setFloat("spotLights[1].outerCutOff", glm::cos(glm::radians(5.0f)));


        // rendering the loaded models

        //island
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.35f, 0.9f));
        model = glm::scale(model, glm::vec3(0.1f));
        objShader.setMat4("model", model);
        island.Draw(objShader);

        //bard
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.42f, 1.11f, 0.08f));
        model = glm::scale(model, glm::vec3(0.24f));
        model = glm::rotate(model, glm::radians(315.0f), glm::vec3(0,1,0));
        model = glm::rotate(model, glm::radians(350.0f), glm::vec3(1,0,0));
        objShader.setMat4("model", model);
        bard.Draw(objShader);

        //mountain island 1
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(20.0f, -7.0f, 20.0f));
        model = glm::scale(model, glm::vec3(7.0, 7.0, 7.0));
        objShader.setMat4("model", model);
        mountain_island.Draw(objShader);

        //mountain island 2
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-20.0f, -6.0f, 20.0f));
        model = glm::scale(model, glm::vec3(6.0, 6.0, 6.0));
        objShader.setMat4("model", model);
        mountain_island.Draw(objShader);

        //mountain island 3
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-20.0f, -8.0f, -20.0f));
        model = glm::scale(model, glm::vec3(8.0, 8.0, 8.0));
        objShader.setMat4("model", model);
        mountain_island.Draw(objShader);

        //underwater terrain island 1
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-15.0f, -5.5f, -15.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
        objShader.setMat4("model", model);
        sand_terrain.Draw(objShader);

        //lantern support 1
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.85f, 1.0f, 0.5f));
        model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        support_beam.Draw(objShader);

        //lantern support 2
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.05f, 1.0f, -0.5f));
        model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        support_beam.Draw(objShader);

        //boat
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-2.51f, 0.97f, -0.76f));
        model = glm::scale(model, glm::vec3(0.38f));
        model = glm::rotate(model, glm::radians(216.0f), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        boat.Draw(objShader);

        //barrel the bard is sitting on
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.34f, 1.03f, 0.03f));
        model = glm::scale(model, glm::vec3(0.065));
        objShader.setMat4("model", model);
        barrel.Draw(objShader);

        //cliffs out of which the small waterfall is flowing
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.79f, -0.21f, 1.65f));
        model = glm::scale(model, glm::vec3(0.190f));
        model = glm::rotate(model, glm::radians(303.0f), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        cliffs.Draw(objShader);

        //cliffs 2
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.31f, 0.09f, 2.65f));
        model = glm::scale(model, glm::vec3(0.245f));
        model = glm::rotate(model, glm::radians(49.5f), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        cliffs.Draw(objShader);

        //granite protrusion in the cliff
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.22f, 2.14f, 1.37f));
        model = glm::scale(model, glm::vec3(74.08f));
        model = glm::rotate(model, glm::radians(244.0f), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        granite.Draw(objShader);

        //granite 2
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.63f, 2.24f, 1.97f));
        model = glm::scale(model, glm::vec3(74.08f));
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1,0,0));
        model = glm::rotate(model, glm::radians(12.5f), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        granite.Draw(objShader);

        /* template for a new object
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(programState->tempPosition));
        model = glm::scale(model, glm::vec3(programState->tempScale));
        model = glm::rotate(model, glm::radians(programState->tempRotation), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        x.Draw(objShader);
         */

        //object rendering end, start of light source rendering

        sourceShader.use();
        sourceShader.setMat4("projection", projection);
        sourceShader.setMat4("view", view);
        sourceShader.setBool("celShading", false);

        //using the transformation matrices from earlier
        sourceShader.setMat4("model", transMat1);
        chinese_lantern.Draw(sourceShader);
        sourceShader.setMat4("model", transMat2);
        chinese_lantern.Draw(sourceShader);

        //light source rendering end, start of waterfall rendering

        waterfallShader.use();
        waterfallShader.setMat4("projection", projection);
        waterfallShader.setMat4("view", view);
        waterfallShader.setFloat("currentFrame", currentFrame);
        glBindVertexArray(waterfallVAO);
        glBindTexture(GL_TEXTURE_2D, waterfallTexture);
        for (unsigned int i = 0; i < waterfall_tiles.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, waterfall_tiles[i]);

            model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));
            if(i==3)
                model = glm::rotate(model, 170.0f, (glm::vec3(1.0f, 0.0f, 0.0f)));
            if(i==4)
                model = glm::rotate(model, glm::radians(62.5f), (glm::vec3(1.0f, 0.0f, 0.0f)));
            if(i==5)
                model = glm::rotate(model, glm::radians(80.0f), (glm::vec3(1.0f, 0.0f, 0.0f)));
            if(i==6)
                model = glm::rotate(model, glm::radians(90.0f), (glm::vec3(1.0f, 0.0f, 0.0f)));
            waterfallShader.setMat4("model", model);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }

        //waterfall rendering end, start of vegetation rendering

        discardShader.use();
        discardShader.setMat4("projection", projection);
        discardShader.setMat4("view", view);
        glBindVertexArray(transparentVAO2);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        for (unsigned int i = 0; i < vegetation.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, vegetation[i]);
            model = glm::rotate(model, (float)i*60.0f, glm::vec3(0.0, 0.1, 0.0));
            discardShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        //vegetation rendering end, start of ripple rendering

        rippleShader.use();
        rippleShader.setMat4("projection", projection);
        rippleShader.setMat4("view", view);
        rippleShader.setFloat("currentFrame", currentFrame);
        glBindVertexArray(rippleVAO);
        glBindTexture(GL_TEXTURE_2D, rippleTexture);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.76f, 1.001f, 0.87f));
        model = glm::scale(model, glm::vec3(programState->tempScale));
        model = glm::rotate(model, glm::radians(programState->tempRotation), glm::vec3(0,1,0));
        rippleShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);


        //ripple rendering end, start of water rendering

        //essentially unnecessary sorting, might change implementation, so it's there just in case
        std::map<float, glm::vec3> sorted;
        for (auto & waterSquare : waterSquares)
        {
            float distance = glm::length(programState->camera.Position - waterSquare);
            sorted[distance] = waterSquare;
        }

        waterShader.use();
        waterShader.setVec3("viewPos", programState->camera.Position);

        waterShader.setMat4("projection", projection);
        waterShader.setMat4("view", view);
        waterShader.setFloat("currentFrame", currentFrame);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glBindVertexArray(transparentVAO);
        for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, it->second);
            waterShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        //water rendering end, start of sky box rendering

        skyboxShader.use();
        skyboxShader.setInt("skybox", 0);

        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS); // set depth function back to default

        if (programState->ImGuiEnabled)
            DrawImGui(programState);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);
    glDeleteVertexArrays(1, &transparentVAO);
    glDeleteBuffers(1, &transparentVAO);
    glDeleteVertexArrays(1, &transparentVAO2);
    glDeleteBuffers(1, &transparentVAO2);
    glDeleteVertexArrays(1, &waterfallVAO);
    glDeleteBuffers(1, &waterfallVAO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        programState->camera.ChangeSpeed(true);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        programState->camera.ChangeSpeed(false);

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    (void) window;

    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xPos, double yPos) {
    (void) window;

    if (firstMouse) {
        lastX = (float)xPos;
        lastY = (float)yPos;
        firstMouse = false;
    }

    float xOffset = (float)xPos - lastX;
    float yOffset = lastY - (float)yPos; // reversed since y-coordinates go from bottom to top

    lastX = (float)xPos;
    lastY = (float)yPos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xOffset, yOffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xOffset, double yOffset) {
    (void) window;
    (void) xOffset;
    programState->camera.ProcessMouseScroll((float)yOffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        ImGui::Begin("Hello window");

        ImGui::DragFloat3("Temp position", (float*)&programState->tempPosition, 0.01,-20.0, 20.0);
        ImGui::DragFloat("Temp scale", &programState->tempScale, 0.02, 0.02, 128.0);
        ImGui::DragFloat("Temp rotation", &programState->tempRotation, 0.5, 0.0, 360.0);

        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void) scancode;
    (void) mods;
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        programState->spotlight = !programState->spotlight;
    }
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLint format = GL_RED;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);


        if(nullptr != strstr(path, "grass.png") || (nullptr != strstr(path, "ripple.jpg"))){
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        }
        else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubeMap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "CubeMap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}