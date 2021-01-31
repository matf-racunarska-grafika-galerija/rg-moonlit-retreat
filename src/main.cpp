#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(const char *path);

unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = true;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 islandPosition = glm::vec3(0.0f, 0.35f, 0.9f);
    float islandScale = 0.1f;
    glm::vec3 bardPosition;
    float bardScale;
    PointLight pointLight;
    bool spotlight = false;
    glm::vec3 tempPosition;
    float tempScale;
    float tempRotation;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
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
        << islandScale << '\n'
        << bardPosition.x << '\n'
        << bardPosition.y << '\n'
        << bardPosition.z << '\n'
        << bardScale << '\n'
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
           >> islandScale
           >> bardPosition.x
           >> bardPosition.y
           >> bardPosition.z
           >> bardScale
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
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
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
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader objShader("resources/shaders/object_lighting.vs", "resources/shaders/object_lighting.fs");
    Shader waterShader("resources/shaders/water_lighting.vs", "resources/shaders/water_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader sourceShader("resources/shaders/light_source.vs", "resources/shaders/light_source.fs");

    // load models
    // -----------
    Model bard("resources/objects/sleepybard/sleepybard.obj");
    bard.SetShaderTextureNamePrefix("material.");
    Model island("resources/objects/island/islandwithdecor.obj");
    island.SetShaderTextureNamePrefix("material.");
    Model mountainisland("resources/objects/mountainisland/mountain.obj");
    mountainisland.SetShaderTextureNamePrefix("material.");
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


    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

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

    // transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // load textures
    // -------------
    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/waterdark.png").c_str());

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
                    //FileSystem::getPath("resources/textures/skybox/right.png"),
                    //FileSystem::getPath("resources/textures/skybox/left.png"),
                    //FileSystem::getPath("resources/textures/skybox/top.png"),
                    //FileSystem::getPath("resources/textures/skybox/bottom.png"),
                    //FileSystem::getPath("resources/textures/skybox/front.png"),
                    //FileSystem::getPath("resources/textures/skybox/back.png")

                    //rotated skybox to put the moon in a different position

                    FileSystem::getPath("resources/textures/skybox/front.png"),
                    FileSystem::getPath("resources/textures/skybox/back.png"),
                    FileSystem::getPath("resources/textures/skybox/top.png"),
                    FileSystem::getPath("resources/textures/skybox/bottom.png"),
                    FileSystem::getPath("resources/textures/skybox/left.png"),
                    FileSystem::getPath("resources/textures/skybox/right.png")
            };
    unsigned int cubemapTexture = loadCubemap(faces);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms

        objShader.use();
        objShader.setVec3("viewPos", programState->camera.Position);
        objShader.setFloat("material.shininess", 32.0f);
        objShader.setBool("celShading", false);
        skyboxShader.setBool("celShading", false);
        waterShader.setBool("celShading", false);
        sourceShader.setBool("celShading", false);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
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

        glm::vec3 pos0 = transMat1 * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec3 pos1 = transMat2 * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        /*
           Here we set all the uniforms for the 5/6 types of lights we have. We have to set them manually and index
           the proper PointLight struct in the array to set each uniform variable. This can be done more code-friendly
           by defining light types as classes and set their values in there, or by using a more efficient uniform approach
           by using 'Uniform buffer objects', but that is something we'll discuss in the 'Advanced GLSL' tutorial.
        */
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

        // spotLight
        objShader.setVec3("spotLight.position", programState->camera.Position);
        objShader.setVec3("spotLight.direction", programState->camera.Front);
        objShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        if(programState->spotlight) {
            objShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
            objShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        }
        else{
            objShader.setVec3("spotLight.diffuse", 0.0f, 0.0f, 0.0f);
            objShader.setVec3("spotLight.specular", 0.0f, 0.0f, 0.0f);
        }
        objShader.setFloat("spotLight.constant", 1.0f);
        objShader.setFloat("spotLight.linear", 0.09);
        objShader.setFloat("spotLight.quadratic", 0.032);
        objShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        objShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));



        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, programState->islandPosition); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(programState->islandScale));    // it's a bit too big for our scene, so scale it down
        objShader.setMat4("model", model);
        island.Draw(objShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, programState->bardPosition);
        model = glm::scale(model, glm::vec3(programState->bardScale));
        model = glm::rotate(model, glm::radians(315.0f), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        bard.Draw(objShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(20.0f, -7.0f, 20.0f));
        model = glm::scale(model, glm::vec3(7.0, 7.0, 7.0));
        objShader.setMat4("model", model);
        mountainisland.Draw(objShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-20.0f, -6.0f, 20.0f));
        model = glm::scale(model, glm::vec3(6.0, 6.0, 6.0));
        objShader.setMat4("model", model);
        mountainisland.Draw(objShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-20.0f, -8.0f, -20.0f));
        model = glm::scale(model, glm::vec3(8.0, 8.0, 8.0));
        objShader.setMat4("model", model);
        mountainisland.Draw(objShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-15.0f, -5.5f, -15.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
        objShader.setMat4("model", model);
        sand_terrain.Draw(objShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.85f, 1.0f, 0.5f));
        model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        support_beam.Draw(objShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.05f, 1.0f, -0.5f));
        model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        support_beam.Draw(objShader);




        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-2.51f, 0.97f, -0.76f));
        model = glm::scale(model, glm::vec3(0.38f));
        model = glm::rotate(model, glm::radians(216.0f), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        boat.Draw(objShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.34f, 1.03f, 0.03f));
        model = glm::scale(model, glm::vec3(0.065));
        objShader.setMat4("model", model);
        barrel.Draw(objShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.79f, -0.21f, 1.65f));
        model = glm::scale(model, glm::vec3(0.190f));
        model = glm::rotate(model, glm::radians(303.0f), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        cliffs.Draw(objShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(programState->tempPosition));
        model = glm::scale(model, glm::vec3(programState->tempScale));
        model = glm::rotate(model, glm::radians(programState->tempRotation), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        cliffs.Draw(objShader);


        /* NEW OBJECT WITH TEMPORARY TRANSFORMATIONS
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(programState->tempPosition));
        model = glm::scale(model, glm::vec3(programState->tempScale));
        model = glm::rotate(model, glm::radians(programState->tempRotation), glm::vec3(0,1,0));
        objShader.setMat4("model", model);
        x.Draw(objShader);
         */

        //object shading end, start of light source shading

        sourceShader.use();
        sourceShader.setMat4("projection", projection);
        sourceShader.setMat4("view", view);

        sourceShader.setBool("celShading", false);
//        glm::mat4 chinese_lanternmodel = glm::mat4(1.0f);
//        chinese_lanternmodel = glm::translate(chinese_lanternmodel, glm::vec3(1.05f, 1.95f, -0.46f));
//        chinese_lanternmodel = glm::scale(chinese_lanternmodel, glm::vec3(0.08f, 0.08f, 0.08f));
//        chinese_lanternmodel = glm::rotate(chinese_lanternmodel, sin(currentFrame*2)*glm::radians(40.0f), glm::vec3(0,0,1));
//        chinese_lanternmodel = glm::translate(chinese_lanternmodel, glm::vec3(0.0f, -3.0f, 0.0f));
        sourceShader.setMat4("model", transMat1);
        chinese_lantern.Draw(sourceShader);

//        chinese_lanternmodel = glm::mat4(1.0f);
//        chinese_lanternmodel = glm::translate(chinese_lanternmodel, glm::vec3(-1.85f, 1.95f, 0.56f));
//        chinese_lanternmodel = glm::scale(chinese_lanternmodel, glm::vec3(0.08f, 0.08f, 0.08f));
//        chinese_lanternmodel = glm::rotate(chinese_lanternmodel, sin(0.6f+currentFrame*2)*glm::radians(40.0f), glm::vec3(0,0,1));
//        chinese_lanternmodel = glm::translate(chinese_lanternmodel, glm::vec3(0.0f, -3.0f, 0.0f));
        sourceShader.setMat4("model", transMat2);
        chinese_lantern.Draw(sourceShader);

        //lightsource shading end, start of water shading
        std::map<float, glm::vec3> sorted;
        for (unsigned int i = 0; i < waterSquares.size(); i++)
        {
            float distance = glm::length(programState->camera.Position - waterSquares[i]);
            sorted[distance] = waterSquares[i];
        }

        // draw objects
        waterShader.use();
        waterShader.setInt("material.diffuse", 0);
        waterShader.setInt("material.specular", 1);

        // light properties
        waterShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
        waterShader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
        waterShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

        // material properties
        waterShader.setFloat("material.shininess", 64.0f);

        waterShader.setVec3("light.direction", -0.2f, -1.0f, -0.3f);
        waterShader.setVec3("viewPos", programState->camera.Position);

        waterShader.setMat4("projection", projection);
        waterShader.setMat4("view", view);

        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);

        // water squares
        glBindVertexArray(transparentVAO);
        for (std::map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, it->second);
            waterShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        skyboxShader.use();
        skyboxShader.setInt("skybox", 0);

        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);

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
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        programState->spotlight = !programState->spotlight;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.

    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);

        ImGui::DragFloat3("Island position", (float*)&programState->islandPosition, 0.05,-20.0, 20.0);
        ImGui::DragFloat("Island scale", &programState->islandScale, 0.05, 0.1, 4.0);
        ImGui::DragFloat3("Bard position", (float*)&programState->bardPosition, 0.05,-20.0, 20.0);
        ImGui::DragFloat("Bard scale", &programState->bardScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat3("Temp position", (float*)&programState->tempPosition, 0.01,-20.0, 20.0);
        ImGui::DragFloat("Temp scale", &programState->tempScale, 0.005, 0.02, 4.0);
        ImGui::DragFloat("Temp rotation", &programState->tempRotation, 0.5, 0.0, 360.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
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
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
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
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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

unsigned int loadCubemap(vector<std::string> faces)
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
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
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