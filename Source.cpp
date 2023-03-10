#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <memory>

#include "Headers/Shader.h"
#include "Headers/Model.h"
#include "Headers/cyTriMesh.h"
#include "Headers/Camera.h"
#include "Headers/Quad.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouseMovementCallback(GLFWwindow* window, double xpos, double ypos);

#pragma region parameters
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0, 0, 30.0f));
float cameraSpeed = 2.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

std::vector<Model*> models;
const char * modelNames[] = {
    "bunny",
    "SIMPLE ROUND TABLE",
    "teapot",
    "cube"
};
glm::vec3 modelTransformation[] = {
    glm::vec3(0.0f, 5.0f, -20.0f), glm::vec3(3.0f, 3.0f, 3.0f), //translation, scale
    glm::vec3(-17.0f, -9.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(-17.0f, -2.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f),
    glm::vec3(0.0f, -9.0f, 0.0f), glm::vec3(0.6f, 1.0f, 0.6f),
};

bool flipModel[] = {
    false,
    false,
    true,
    false,
};
#pragma endregion
glm::vec3 lights[] = {
    glm::vec3(20, 10, 10),    glm::vec3(1,1,1),               glm::vec3(1,0.007f,0.0002f),          //position, color, (constant, linear, quadratic)
    glm::vec3(-25, -5, -35), glm::vec3(0.224f,0.42f,0.659f), glm::vec3(1,0.007f,0.0002f),
    glm::vec3(25, -5, -35), glm::vec3(0.306f,0.714f,0.71f), glm::vec3(1,0.027,0.0028),
};

int main() {

#pragma region OpenGL Initialization
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, false);
    //glfwWindowHint(GLFW_STENCIL_BITS, 8);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SSR", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouseMovementCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

#pragma endregion
    glm::mat4 projection = glm::mat4(1.0f);
    projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 300.0f);

    Quad* quad = new Quad();

    GLint originalFrameBuffer;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &originalFrameBuffer);

#pragma region GBuffer Initialization
    GLuint gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    GLuint gPosition, gNormal, gAlbedo, gSpecular;

    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gNormal, 0);
    glGenTextures(1, &gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gAlbedo, 0);
    glGenTextures(1, &gSpecular);
    glBindTexture(GL_TEXTURE_2D, gSpecular);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gSpecular, 0);

    //depth texture
    GLuint depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depthMap, 0);
   
    GLenum drawBuffers1[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, drawBuffers1);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << std::hex << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
        return -1;
    }
#pragma endregion

#pragma region lighting pass Buffer Initialization
    GLuint LPFB; //Lighting Pass Buffer
    glGenFramebuffers(1, &LPFB);
    glBindFramebuffer(GL_FRAMEBUFFER, LPFB);
    GLuint colorBuffer;
    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
    GLenum drawBuffers2[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers2);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "problems binding LPFB" << std::endl;
        return -1;
    }
#pragma endregion

#pragma region SSR pass Buffer Initialization
    GLuint ssrFB; //SSR Frame Buffer
    glGenFramebuffers(1, &ssrFB);
    glBindFramebuffer(GL_FRAMEBUFFER, ssrFB);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depthMap, 0);
    GLuint reflectionColorBuffer;
    glGenTextures(1, &reflectionColorBuffer);
    glBindTexture(GL_TEXTURE_2D, reflectionColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionColorBuffer, 0);
    GLenum drawBuffers3[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers3);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "problems binding ssrFB" << std::endl;
        return -1;
    }
#pragma endregion

    Shader forwardPassShader("Shaders/ForwardPassVS.vs", "Shaders/ForwardPassFS.fs");

    int numOfModels = 3;
    cyTriMesh* Objectctm = new cyTriMesh;
    for (int i = 0; i < numOfModels; i++)
    {  
        if (!Objectctm->LoadFromFileObj(("Models/" + std::string(modelNames[i]) + ".obj").c_str())) {
            return -1;
        }
        models.push_back(new Model(*Objectctm, forwardPassShader, modelTransformation[i*2], modelTransformation[i*2+1], projection, true, flipModel[i]));
    }
    delete Objectctm;
    /* cyTriMesh Spherectm;
    if (!Spherectm.LoadFromFileObj("Models/sphere.obj")) {
        return -1;
    }*/
    //Model* sphere = new Model(Spherectm, forwardPassShader, glm::vec3(-10.0f, -5.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), projection, true, false);    //sphere
    cyTriMesh Groundctm;
    if (!Groundctm.LoadFromFileObj("Models/ground.obj")) {
        return -1;
    }   
    Model* ground = new Model(Groundctm, forwardPassShader, glm::vec3(0.0f, -20.0f, 0.0f), glm::vec3(10.0f, 1.0f, 10.0f), projection, true, false); //ground

    Shader lightingPassShader("Shaders/DeferredPassVS.vs", "Shaders/DeferredPassFS.fs");
    lightingPassShader.use();
    lightingPassShader.setInt("gNormal", 0);
    lightingPassShader.setInt("gAlbedo", 1);
    lightingPassShader.setInt("gSpecular", 2);
    lightingPassShader.setInt("depthMap", 3);
    for (int i = 0; i < sizeof(lights) / sizeof(lights[0]); i++)
    {
        lightingPassShader.setVec3("lights[" + std::to_string(i) + "].intensity", lights[i * 3 + 1]);
        lightingPassShader.setFloat("lights[" + std::to_string(i) + "].constant", lights[i * 3 + 2].x);
        lightingPassShader.setFloat("lights[" + std::to_string(i) + "].linear", lights[i * 3 + 2].y);
        lightingPassShader.setFloat("lights[" + std::to_string(i) + "].quadratic", lights[i * 3 + 2].z);
    }


    Shader SSRShader("Shaders/SSRVS.vs", "Shaders/SSRFS.fs");
    SSRShader.use();
    SSRShader.setInt("gNormal", 0);
    SSRShader.setInt("colorBuffer", 1);
    SSRShader.setInt("depthMap", 2);

    Shader outputShader("Shaders/SSRVS.vs", "Shaders/outputFS.fs");
    outputShader.use();
    outputShader.setInt("colorTexture", 0);
    outputShader.setInt("refTexture", 1);
    outputShader.setInt("specularTexture", 2);

    glm::mat4 view = glm::mat4(1.0);

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;


        //Forward Pass
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gBuffer);
        glClearColor(0, 0, 0, 1.0f);
        glStencilMask(0xFF);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glStencilFunc(GL_EQUAL, 0, 0xFF);
        glStencilMask(0x00); 
        view = camera.GetViewMatrix();
        for (Model* m : models)
        {   
            m->Draw(view);
        }

        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
        ground->Draw(view);
        //sphere->setLightPosition(view* glm::vec4(lightPosition, 1));
        //sphere->Draw(view);

        //Deferred(Lighting) Pass
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, LPFB);
        glClearColor(0, 0, 0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gAlbedo);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gSpecular);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMap);

        lightingPassShader.use();
        lightingPassShader.setVec3("lightPosition", view * glm::vec4(lights[0], 1));
        lightingPassShader.setFloat("SCR_WIDTH", SCR_WIDTH);
        lightingPassShader.setFloat("SCR_HEIGHT", SCR_HEIGHT);
        lightingPassShader.setMat4("invProj", glm::inverse(projection));
        for (int i = 0; i < sizeof(lights) / sizeof(lights[0]); i++)
        {     
            lightingPassShader.setVec3("lights[" + std::to_string(i) + "].position", view * glm::vec4(lights[i * 3], 1));
        }

  
        glBindVertexArray(quad->GetVAO());
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        //SSR
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ssrFB);
        glClearColor(0, 0, 0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDepthMask(GL_FALSE);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, depthMap);

        SSRShader.use();
        SSRShader.setFloat("SCR_WIDTH", SCR_WIDTH);
        SSRShader.setFloat("SCR_HEIGHT", SCR_HEIGHT);
        SSRShader.setMat4("invProjection", glm::inverse(projection));
        SSRShader.setMat4("projection", projection);

        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glBindVertexArray(quad->GetVAO());
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glDepthMask(GL_TRUE);

        //output
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, originalFrameBuffer);
        glClearColor(0, 0, 0, 1.0f);
        glStencilMask(0xFF);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, reflectionColorBuffer);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gSpecular);

        outputShader.use();
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glBindVertexArray(quad->GetVAO());
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (Model* m : models)
    {
        delete m;
    }
    delete ground;
    delete quad;
    glDeleteFramebuffers(1, &gBuffer);
    glDeleteFramebuffers(1, &LPFB);
    glDeleteFramebuffers(1, &ssrFB);
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.move(FORWARD, deltaTime * cameraSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.move(BACKWARD, deltaTime * cameraSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.move(LEFT, deltaTime * cameraSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.move(RIGHT, deltaTime * cameraSpeed);
    }
}

void mouseMovementCallback(GLFWwindow* window, double xpos, double ypos) {
    camera.rotate(xpos, ypos);
}

