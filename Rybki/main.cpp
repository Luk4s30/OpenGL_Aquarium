#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "Shader.h"
#include "Camera.h"
#include "model.h"

#include <vector>
#include <cstdlib>

// STRUKTURY
struct Boid {
    glm::vec3 position;
    glm::vec3 velocity;
};

struct LightUBO {
    glm::vec3 position;
    float cutOff;
    glm::vec3 direction;
    float outerCutOff;
    glm::vec3 color;
    float constant;
    float linear;
    float quadratic;
    float pad1, pad2;
};

struct ChestLightUBO {
    glm::vec3 position;
    float intensity;
    glm::vec3 color;
    float pad;
};

struct RoslinaPos { float x, z, skala, offset; };
std::vector<RoslinaPos> rosliny = {
    { -1.2f,  0.5f, 0.05f, 0.0f },
    {  0.3f,  0.0f, 0.06f, 2.4f },
    {  1.0f,  0.2f, 0.04f, 5.1f },
    { -0.5f, -0.3f, 0.03f, 1.3f },
};

enum ObstacleType {
    OBS_SPHERE,
    OBS_CONE
};

struct Obstacle {
    glm::vec3 position;
    float radiusBottom;
    float radiusTop;
    float height;
    ObstacleType type;
};

// ZMIENNE GLOBALNE
const int NUM_BOIDS = 50;
std::vector<Boid> stado;
std::vector<Obstacle> przeszkody;

// okno i kamera
const unsigned int SCR_WIDTH = 1800;
const unsigned int SCR_HEIGHT = 1350;
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

float cubeAngleX = 0.0f;
float cubeAngleY = 0.0f;
float cubeRotationSpeed = 100.0f;

float lightYaw = -90.0f;
float lightPitch = 45.0f;
float lightRadius = 5.0f;
float lightSpeed = 60.0f;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float spin = 0.0f;

// Bufory obiektów
unsigned int kwadratVAO, kwadratVBO;
unsigned int akwariumVAO, akwariumVBO;
unsigned int wodaVAO, wodaVBO;
unsigned int skyboxVAO, skyboxVBO;
unsigned int cubemapTexture;

// Bufory debug
unsigned int debugLineVAO, debugLineVBO;
unsigned int debugCircleVAO, debugCircleVBO;
bool showDebug = true;
bool pKeyPressed = false;

// DEKLARACJE FUNKCJI
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
unsigned int loadCubemap(std::vector<std::string> faces);
unsigned int loadTexture2D(char const* path);

// FUNKCJE INICJALIZUJĄCE 
void initBoids() {
    for (int i = 0; i < NUM_BOIDS; i++) {
        Boid b;
        b.position = glm::vec3(
            ((rand() % 200) / 100.0f - 1.0f) * 1.5f,
            ((rand() % 200) / 100.0f - 1.0f) * 0.5f,
            ((rand() % 200) / 100.0f - 1.0f) * 0.7f
        );
        b.velocity = glm::vec3(
            ((rand() % 200) / 100.0f - 1.0f),
            ((rand() % 200) / 100.0f - 1.0f) * 0.2f,
            ((rand() % 200) / 100.0f - 1.0f)
        );
        b.velocity = glm::normalize(b.velocity) * 0.5f;
        stado.push_back(b);
    }
}

void initPrzeszkody() {
    przeszkody.clear();

    przeszkody.push_back({ glm::vec3(1.6f, -1.0f, -0.65f), 0.25f, 0.25f, 0.0f, OBS_SPHERE });

    float bazowaWysokosc = 20.0f;
    float bazowyPromienLodygi = 1.0f;
    float bazowyPromienKorony = 6.0f;
    float lokalnePrzesuniecieX = -4.0f;
    float lokalnePrzesuniecieZ = -4.0f;

    float aktualnySpin = 0.0f;

    for (const auto& r : rosliny) {
        aktualnySpin += 0.5f;

        float wysokosc = bazowaWysokosc * r.skala;
        float promienLodygi = bazowyPromienLodygi * r.skala;
        float promienKorony = bazowyPromienKorony * r.skala;

        glm::mat4 lokalnaMacierz = glm::mat4(1.0f);
        lokalnaMacierz = glm::translate(lokalnaMacierz, glm::vec3(r.x, -1.1f, r.z));
        lokalnaMacierz = glm::rotate(lokalnaMacierz, aktualnySpin, glm::vec3(0.0f, -1.0f, 0.0f));
        lokalnaMacierz = glm::scale(lokalnaMacierz, glm::vec3(r.skala));

        glm::vec4 pozycjaLodygi = lokalnaMacierz * glm::vec4(lokalnePrzesuniecieX, 0.0f, lokalnePrzesuniecieZ, 1.0f);

        przeszkody.push_back({ glm::vec3(pozycjaLodygi.x, pozycjaLodygi.y, pozycjaLodygi.z), promienLodygi, promienKorony, wysokosc, OBS_CONE });
    }
}

void initDebug() {
    glGenVertexArrays(1, &debugLineVAO);
    glGenBuffers(1, &debugLineVBO);
    glBindVertexArray(debugLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    std::vector<float> circleVertices;
    int segments = 16;
    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * 3.14159265f / segments;
        circleVertices.push_back(cos(angle));
        circleVertices.push_back(0.0f);
        circleVertices.push_back(sin(angle));
    }

    glGenVertexArrays(1, &debugCircleVAO);
    glGenBuffers(1, &debugCircleVBO);
    glBindVertexArray(debugCircleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, debugCircleVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void initKwadrat() {
    float vertices[] = {
        -1.99f, -0.99f, -0.99f,  0.0f, 1.0f, 0.0f,    0.0f,  1.5f,  1.0f, 0.0f, 0.0f,
         1.99f, -0.99f, -0.99f,  0.0f, 1.0f, 0.0f,    1.5f,  1.5f,  1.0f, 0.0f, 0.0f,
         1.99f, -0.99f,  0.99f,  0.0f, 1.0f, 0.0f,    1.5f,  0.0f,  1.0f, 0.0f, 0.0f,
         1.99f, -0.99f,  0.99f,  0.0f, 1.0f, 0.0f,    1.5f,  0.0f,  1.0f, 0.0f, 0.0f,
        -1.99f, -0.99f,  0.99f,  0.0f, 1.0f, 0.0f,    0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
        -1.99f, -0.99f, -0.99f,  0.0f, 1.0f, 0.0f,    0.0f,  1.5f,  1.0f, 0.0f, 0.0f
    };
    glGenVertexArrays(1, &kwadratVAO);
    glGenBuffers(1, &kwadratVBO);
    glBindVertexArray(kwadratVAO);
    glBindBuffer(GL_ARRAY_BUFFER, kwadratVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
}

void rysujKwadrat() {
    glBindVertexArray(kwadratVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void initAkwarium() {
    float vertices[] = {
        -2.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         2.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         2.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         2.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -2.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -2.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        -2.0f, -1.0f,  1.0f,   0.0f,  0.0f, 1.0f,  0.0f, 0.0f,
         2.0f, -1.0f,  1.0f,   0.0f,  0.0f, 1.0f,  1.0f, 0.0f,
         2.0f,  1.0f,  1.0f,   0.0f,  0.0f, 1.0f,  1.0f, 1.0f,
         2.0f,  1.0f,  1.0f,   0.0f,  0.0f, 1.0f,  1.0f, 1.0f,
        -2.0f,  1.0f,  1.0f,   0.0f,  0.0f, 1.0f,  0.0f, 1.0f,
        -2.0f, -1.0f,  1.0f,   0.0f,  0.0f, 1.0f,  0.0f, 0.0f,
        -2.0f, -1.0f,  1.0f,  -1.0f,  0.0f, 0.0f,  0.0f, 0.0f,
        -2.0f, -1.0f, -1.0f,  -1.0f,  0.0f, 0.0f,  1.0f, 0.0f,
        -2.0f,  1.0f, -1.0f,  -1.0f,  0.0f, 0.0f,  1.0f, 1.0f,
        -2.0f,  1.0f, -1.0f,  -1.0f,  0.0f, 0.0f,  1.0f, 1.0f,
        -2.0f,  1.0f,  1.0f,  -1.0f,  0.0f, 0.0f,  0.0f, 1.0f,
        -2.0f, -1.0f,  1.0f,  -1.0f,  0.0f, 0.0f,  0.0f, 0.0f,
         2.0f, -1.0f,  1.0f,   1.0f,  0.0f, 0.0f,  0.0f, 0.0f,
         2.0f, -1.0f, -1.0f,   1.0f,  0.0f, 0.0f,  1.0f, 0.0f,
         2.0f,  1.0f, -1.0f,   1.0f,  0.0f, 0.0f,  1.0f, 1.0f,
         2.0f,  1.0f, -1.0f,   1.0f,  0.0f, 0.0f,  1.0f, 1.0f,
         2.0f,  1.0f,  1.0f,   1.0f,  0.0f, 0.0f,  0.0f, 1.0f,
         2.0f, -1.0f,  1.0f,   1.0f,  0.0f, 0.0f,  0.0f, 0.0f,
        -2.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
         2.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         2.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         2.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
        -2.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        -2.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    };
    glGenVertexArrays(1, &akwariumVAO);
    glGenBuffers(1, &akwariumVBO);
    glBindVertexArray(akwariumVAO);
    glBindBuffer(GL_ARRAY_BUFFER, akwariumVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

void rysujAkwarium() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glBindVertexArray(akwariumVAO);
    glDrawArrays(GL_TRIANGLES, 0, 30);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void initWoda() {
    float vertices[] = {
        -1.99f,  0.8f, -0.99f,      0.0f, 1.0f, 0.0f,       0.0f, 0.0f,
         1.99f,  0.8f, -0.99f,      0.0f, 1.0f, 0.0f,       1.0f, 0.0f,
         1.99f,  0.8f,  0.99f,      0.0f, 1.0f, 0.0f,       1.0f, 1.0f,
         1.99f,  0.8f,  0.99f,      0.0f, 1.0f, 0.0f,       1.0f, 1.0f,
        -1.99f,  0.8f,  0.99f,      0.0f, 1.0f, 0.0f,       0.0f, 1.0f,
        -1.99f,  0.8f, -0.99f,      0.0f, 1.0f, 0.0f,       0.0f, 0.0f,
        -1.99f, -0.99f, -0.99f,     0.0f, -1.0f, 0.0f,      0.0f, 0.0f,
        -1.99f, -0.99f,  0.99f,     0.0f, -1.0f, 0.0f,      0.0f, 1.0f,
         1.99f, -0.99f,  0.99f,     0.0f, -1.0f, 0.0f,      1.0f, 1.0f,
         1.99f, -0.99f,  0.99f,     0.0f, -1.0f, 0.0f,      1.0f, 1.0f,
         1.99f, -0.99f, -0.99f,     0.0f, -1.0f, 0.0f,      1.0f, 0.0f,
        -1.99f, -0.99f, -0.99f,     0.0f, -1.0f, 0.0f,      0.0f, 0.0f,
        -1.99f, -0.99f,  0.99f,     0.0f, 0.0f, 1.0f,       0.0f, 0.0f,
         1.99f, -0.99f,  0.99f,     0.0f, 0.0f, 1.0f,       1.0f, 0.0f,
         1.99f,  0.8f,   0.99f,     0.0f, 0.0f, 1.0f,       1.0f, 1.0f,
         1.99f,  0.8f,   0.99f,     0.0f, 0.0f, 1.0f,       1.0f, 1.0f,
        -1.99f,  0.8f,   0.99f,     0.0f, 0.0f, 1.0f,       0.0f, 1.0f,
        -1.99f, -0.99f,  0.99f,     0.0f, 0.0f, 1.0f,       0.0f, 0.0f,
         1.99f, -0.99f, -0.99f,     0.0f, 0.0f, -1.0f,      0.0f, 0.0f,
        -1.99f, -0.99f, -0.99f,     0.0f, 0.0f, -1.0f,      1.0f, 0.0f,
        -1.99f,  0.8f,  -0.99f,     0.0f, 0.0f, -1.0f,      1.0f, 1.0f,
        -1.99f,  0.8f,  -0.99f,     0.0f, 0.0f, -1.0f,      1.0f, 1.0f,
         1.99f,  0.8f,  -0.99f,     0.0f, 0.0f, -1.0f,      0.0f, 1.0f,
         1.99f, -0.99f, -0.99f,     0.0f, 0.0f, -1.0f,      0.0f, 0.0f,
         -1.99f, -0.99f, -0.99f,    -1.0f, 0.0f, 0.0f,       0.0f, 0.0f,
         -1.99f, -0.99f,  0.99f,    -1.0f, 0.0f, 0.0f,       1.0f, 0.0f,
         -1.99f,  0.8f,   0.99f,    -1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
         -1.99f,  0.8f,   0.99f,    -1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
         -1.99f,  0.8f,  -0.99f,    -1.0f, 0.0f, 0.0f,       0.0f, 1.0f,
         -1.99f, -0.99f, -0.99f,    -1.0f, 0.0f, 0.0f,       0.0f, 0.0f,
          1.99f, -0.99f,  0.99f,     1.0f, 0.0f, 0.0f,       0.0f, 0.0f,
          1.99f, -0.99f, -0.99f,     1.0f, 0.0f, 0.0f,       1.0f, 0.0f,
          1.99f,  0.8f,  -0.99f,     1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
          1.99f,  0.8f,  -0.99f,     1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
          1.99f,  0.8f,   0.99f,     1.0f, 0.0f, 0.0f,       0.0f, 1.0f,
          1.99f, -0.99f,  0.99f,     1.0f, 0.0f, 0.0f,       0.0f, 0.0f
    };
    glGenVertexArrays(1, &wodaVAO);
    glGenBuffers(1, &wodaVBO);
    glBindVertexArray(wodaVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wodaVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

void rysujWode() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glBindVertexArray(wodaVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void initSkybox() {
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}


// LOGIKA BOIDS
glm::vec3 limitVector(glm::vec3 v, float max) {
    float length = glm::length(v);
    if (length > max && length > 0.0f) {
        return (v / length) * max;
    }
    return v;
}

void aktualizujBoids(float dt) {
    float visualRange = 0.4f; //zasięg widzenia
    float protectedRange = 0.3f; // utrzymywany dystans pomiędzy innymi
    float maxSpeed = 0.6f;
    float minSpeed = 0.2f;
    float maxForce = 0.5f; // siła skętu

    float separationWeight = 2.0f;
    float alignmentWeight = 1.0f;
    float cohesionWeight = 1.0f;
    float avoidanceWeight = 3.0f;

    float minX = -2.0f, maxX = 2.0f;
    float minY = -1.0f, maxY = 0.8f;
    float minZ = -0.8f, maxZ = 0.8f;
    float wallMargin = 0.3f;

    for (int i = 0; i < stado.size(); i++) {
        glm::vec3 separation(0.0f);
        glm::vec3 alignment(0.0f);
        glm::vec3 cohesion(0.0f);
        glm::vec3 avoidance(0.0f);

        int neighboringBoids = 0;
        glm::vec3 acceleration(0.0f);

        glm::vec3 wallAvoidance(0.0f);
        if (stado[i].position.x < minX + wallMargin) wallAvoidance.x = 1.0f;
        if (stado[i].position.x > maxX - wallMargin) wallAvoidance.x = -1.0f;
        if (stado[i].position.y < minY + wallMargin) wallAvoidance.y = 1.0f;
        if (stado[i].position.y > maxY - wallMargin) wallAvoidance.y = -1.0f;
        if (stado[i].position.z < minZ + wallMargin) wallAvoidance.z = 1.0f;
        if (stado[i].position.z > maxZ - wallMargin) wallAvoidance.z = -1.0f;

        avoidance += wallAvoidance * 2.0f;

        for (const auto& obs : przeszkody) {
            glm::vec3 pushDir(0.0f);
            float dist = 0.0f;
            float dangerRadius = 0.0f;

            if (obs.type == OBS_SPHERE) {
                dangerRadius = obs.radiusBottom + 0.15f;
                dist = glm::distance(stado[i].position, obs.position);

                if (dist < dangerRadius) {
                    pushDir = stado[i].position - obs.position;
                }
            }
            else if (obs.type == OBS_CONE) {

                glm::vec3 p1 = obs.position;
                float height = obs.height;
                glm::vec3 axisDir(0.0f, 1.0f, 0.0f);

                glm::vec3 w = stado[i].position - p1;
                float u = glm::dot(w, axisDir);
                float t = glm::clamp(u / height, 0.0f, 1.0f);
                float currentRadius = obs.radiusBottom + t * (obs.radiusTop - obs.radiusBottom);
                dangerRadius = currentRadius + 0.15f;

                float clampedU = glm::clamp(u, 0.0f, height);
                glm::vec3 closestPointOnAxis = p1 + clampedU * axisDir;
                glm::vec3 outVector = stado[i].position - closestPointOnAxis;
                dist = glm::length(outVector);

                if (dist < dangerRadius && glm::length(outVector) > 0) {
                    pushDir = outVector;
                }
                else {
                    pushDir = glm::vec3(0.0f);
                    dist = 999.0f;
                }
            }

            if (dist < dangerRadius && glm::length(pushDir) > 0) {
                pushDir = glm::normalize(pushDir);
                pushDir *= (dangerRadius - dist) / dangerRadius;
                avoidance += pushDir;
            }
        }

        // CZUJNIK STADA
        for (int j = 0; j < stado.size(); j++) {
            if (i == j) continue;

            float dist = glm::distance(stado[i].position, stado[j].position);

            if (dist < protectedRange) {
                glm::vec3 diff = stado[i].position - stado[j].position;
                if (dist > 0.0f) diff /= dist;
                separation += diff;
            }
            else if (dist < visualRange) {
                alignment += stado[j].velocity;
                cohesion += stado[j].position;
                neighboringBoids++;
            }
        }

        if (neighboringBoids > 0) {
            alignment /= neighboringBoids;
            alignment = glm::normalize(alignment) * maxSpeed;
            alignment = alignment - stado[i].velocity;

            cohesion /= neighboringBoids;
            glm::vec3 desiredCohesion = cohesion - stado[i].position;
            desiredCohesion = glm::normalize(desiredCohesion) * maxSpeed;
            cohesion = desiredCohesion - stado[i].velocity;
        }

        if (glm::length(separation) > 0) {
            separation = glm::normalize(separation) * maxSpeed;
            separation = separation - stado[i].velocity;
        }

        if (glm::length(avoidance) > 0) {
            avoidance = glm::normalize(avoidance) * maxSpeed;
            avoidance = avoidance - stado[i].velocity;
        }

        // PRIORYTETY
        float currentAvoidanceMag = glm::length(avoidance);
        acceleration += limitVector(avoidance * avoidanceWeight, maxForce);

        if (currentAvoidanceMag < 0.1f) {
            acceleration += limitVector(separation * separationWeight, maxForce);
            acceleration += limitVector(alignment * alignmentWeight, maxForce);
            acceleration += limitVector(cohesion * cohesionWeight, maxForce);
        }
        else {
            acceleration += limitVector(separation * (separationWeight * 0.5f), maxForce);
        }

        // AKTUALIZACJA
        stado[i].velocity += acceleration * dt;

        float speed = glm::length(stado[i].velocity);
        if (speed < minSpeed) stado[i].velocity = (stado[i].velocity / speed) * minSpeed;
        if (speed > maxSpeed) stado[i].velocity = (stado[i].velocity / speed) * maxSpeed;

        stado[i].position += stado[i].velocity * dt;

        if (stado[i].position.x <= minX || stado[i].position.x >= maxX) stado[i].velocity.x *= -1.0f;
        if (stado[i].position.y <= minY || stado[i].position.y >= maxY) stado[i].velocity.y *= -1.0f;
        if (stado[i].position.z <= minZ || stado[i].position.z >= maxZ) stado[i].velocity.z *= -1.0f;

        stado[i].position.x = glm::clamp(stado[i].position.x, minX, maxX);
        stado[i].position.y = glm::clamp(stado[i].position.y, minY, maxY);
        stado[i].position.z = glm::clamp(stado[i].position.z, minZ, maxZ);
    }
}

void rysujDebug(Shader& shader) {
    if (!showDebug) return;

    glDisable(GL_DEPTH_TEST);
    shader.use();

    glm::mat4 baseModel = glm::mat4(1.0f);
    baseModel = glm::rotate(baseModel, glm::radians(cubeAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
    baseModel = glm::rotate(baseModel, glm::radians(cubeAngleY), glm::vec3(0.0f, 1.0f, 0.0f));

    shader.setVec4("baseColor", 1.0f, 0.0f, 0.0f, 1.0f);
    glBindVertexArray(debugLineVAO);
    for (const auto& boid : stado) {
        glm::vec3 p1 = boid.position;
        glm::vec3 p2 = boid.position + (boid.velocity * 1.5f);

        float lineData[6] = { p1.x, p1.y, p1.z, p2.x, p2.y, p2.z };
        glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineData), lineData);

        shader.setMat4("model", baseModel);
        glDrawArrays(GL_LINES, 0, 2);
    }

    shader.setVec4("baseColor", 0.0f, 1.0f, 0.0f, 1.0f);

    for (const auto& obs : przeszkody) {
        if (obs.type == OBS_SPHERE) {
            glBindVertexArray(debugCircleVAO);

            float r = obs.radiusBottom + 0.15f;
            glm::mat4 model = baseModel;
            model = glm::translate(model, obs.position);
            model = glm::scale(model, glm::vec3(r));

            shader.setMat4("model", model);
            glDrawArrays(GL_LINE_STRIP, 0, 17);

            glm::mat4 modelXY = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            shader.setMat4("model", modelXY);
            glDrawArrays(GL_LINE_STRIP, 0, 17);

            glm::mat4 modelYZ = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            shader.setMat4("model", modelYZ);
            glDrawArrays(GL_LINE_STRIP, 0, 17);
        }
        else if (obs.type == OBS_CONE) {
            float rBot = obs.radiusBottom + 0.15f;
            float rTop = obs.radiusTop + 0.15f;

            glBindVertexArray(debugCircleVAO);

            glm::mat4 modelBot = baseModel;
            modelBot = glm::translate(modelBot, obs.position);
            modelBot = glm::scale(modelBot, glm::vec3(rBot));
            shader.setMat4("model", modelBot);
            glDrawArrays(GL_LINE_STRIP, 0, 17);

            glm::mat4 modelTop = baseModel;
            modelTop = glm::translate(modelTop, obs.position + glm::vec3(0.0f, obs.height, 0.0f));
            modelTop = glm::scale(modelTop, glm::vec3(rTop));
            shader.setMat4("model", modelTop);
            glDrawArrays(GL_LINE_STRIP, 0, 17);

            glBindVertexArray(debugLineVAO); 

            int numLines = 8;
            for (int i = 0; i < numLines; i++) {
                float angle = i * 2.0f * 3.14159265f / numLines;
                float cx = cos(angle);
                float cz = sin(angle);

                glm::vec3 pBot = obs.position + glm::vec3(cx * rBot, 0.0f, cz * rBot);
                glm::vec3 pTop = obs.position + glm::vec3(cx * rTop, obs.height, cz * rTop);

                float lineData[6] = { pBot.x, pBot.y, pBot.z, pTop.x, pTop.y, pTop.z };
                glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineData), lineData);

                shader.setMat4("model", baseModel);
                glDrawArrays(GL_LINES, 0, 2);
            }
        }
    }

    glEnable(GL_DEPTH_TEST);
}


int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Rybki", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glewInit() != GLEW_OK) return -1;
    glEnable(GL_DEPTH_TEST);

    Shader mainShader("shaders/norm-v.glsl", "shaders/norm-f.glsl");
    Shader modelShader("shaders/ryba-v.glsl", "shaders/ryba-f.glsl");
    Shader szybaShader("shaders/szyba-v.glsl", "shaders/szyba-f.glsl");
    Shader wodaShader("shaders/woda-v.glsl", "shaders/woda-f.glsl");
    Shader skyboxShader("shaders/skybox-v.glsl", "shaders/skybox-f.glsl");
    Shader roslinaShader("shaders/roslina-v.glsl", "shaders/roslina-f.glsl");
    Shader chestShader("shaders/chest-v.glsl", "shaders/chest-f.glsl");

    auto bindBlock = [](GLuint programID, const char* name, GLuint binding) {
        GLuint idx = glGetUniformBlockIndex(programID, name);
        if (idx != GL_INVALID_INDEX) glUniformBlockBinding(programID, idx, binding);
        };

    bindBlock(mainShader.ID, "LightBlock", 0);
    bindBlock(modelShader.ID, "LightBlock", 0);
    bindBlock(chestShader.ID, "LightBlock", 0);
    bindBlock(wodaShader.ID, "LightBlock", 0);
    bindBlock(roslinaShader.ID, "LightBlock", 0);
    bindBlock(szybaShader.ID, "LightBlock", 0);

    bindBlock(mainShader.ID, "ChestLightBlock", 1);
    bindBlock(modelShader.ID, "ChestLightBlock", 1);
    bindBlock(chestShader.ID, "ChestLightBlock", 1);
    bindBlock(roslinaShader.ID, "ChestLightBlock", 1);
    bindBlock(wodaShader.ID, "ChestLightBlock", 1);
    bindBlock(szybaShader.ID, "ChestLightBlock", 1);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    std::vector<std::string> faces = {
        "texture/clouds/clouds1_east.bmp", "texture/clouds/clouds1_west.bmp",
        "texture/clouds/clouds1_up.bmp", "texture/clouds/clouds1_down.bmp",
        "texture/clouds/clouds1_north.bmp", "texture/clouds/clouds1_south.bmp",
    };

    unsigned int piasekTexture = loadTexture2D("texture/sand/GroundSand005_COL_1K.jpg");
    unsigned int normalMapTexture = loadTexture2D("texture/sand/GroundSand005_NRM_1K.jpg");
    unsigned int chestBaseColor = loadTexture2D("models/chest/Material_Base_color.png");
    unsigned int chestNormal = loadTexture2D("models/chest/Material_Normal_OpenGL.png");
    unsigned int chestAO = loadTexture2D("models/chest/Material_Mixed_AO.png");

    Model rybaModel("models/fish/13009_Coral_Beauty_Angelfish_v1_l3.obj");
    Model roslinaModel("models/plant/plant.obj");
    Model skrzyniaModel("models/chest/chest.obj");

    unsigned int uboLight, uboChestLight;
    glGenBuffers(1, &uboLight);
    glBindBuffer(GL_UNIFORM_BUFFER, uboLight);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(LightUBO), NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboLight);

    glGenBuffers(1, &uboChestLight);
    glBindBuffer(GL_UNIFORM_BUFFER, uboChestLight);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ChestLightUBO), NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, uboChestLight);

    glm::vec3 lightPos = glm::vec3(0.0f, 4.0f, 0.0f);
    glm::vec3 lightDir = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float cutOff = glm::cos(glm::radians(25.0f));
    float outerCutOff = glm::cos(glm::radians(35.0f));

    cubemapTexture = loadCubemap(faces);
    initSkybox();
    initPrzeszkody();
    initBoids();
    initAkwarium();
    initWoda();
    initKwadrat();
    initDebug();

    unsigned int screenTexture;
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    struct RoslinaPos { float x, z, skala, offset; };
    std::vector<RoslinaPos> rosliny = {
        { -1.2f,  0.5f, 0.05f, 0.0f },
        {  0.3f,  0.0f, 0.06f, 2.4f },
        {  1.0f,  0.2f, 0.04f, 5.1f },
        { -0.5f, -0.3f, 0.03f, 1.3f },
    };

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        aktualizujBoids(deltaTime);
        processInput(window);

        lightPos.x = lightRadius * cos(glm::radians(lightPitch)) * cos(glm::radians(lightYaw));
        lightPos.y = lightRadius * sin(glm::radians(lightPitch));
        lightPos.z = lightRadius * cos(glm::radians(lightPitch)) * sin(glm::radians(lightYaw));
        lightDir = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - lightPos);

        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(cubeAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(cubeAngleY), glm::vec3(0.0f, 1.0f, 0.0f));

        LightUBO lightData;
        lightData.position = lightPos;
        lightData.cutOff = cutOff;
        lightData.direction = lightDir;
        lightData.outerCutOff = outerCutOff;
        lightData.color = lightColor;
        lightData.constant = 1.0f;
        lightData.linear = 0.04f;
        lightData.quadratic = 0.015f;

        glBindBuffer(GL_UNIFORM_BUFFER, uboLight);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightUBO), &lightData);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        ChestLightUBO chestData;
        chestData.position = glm::vec3(1.6f, -0.7f, -0.65f);
        chestData.intensity = 0.8f + 0.2f * sin(currentFrame * 2.0f);
        chestData.color = glm::vec3(1.0f, 0.75f, 0.1f);

        glBindBuffer(GL_UNIFORM_BUFFER, uboChestLight);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ChestLightUBO), &chestData);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Dno
        mainShader.use();
        mainShader.setMat4("projection", projection);
        mainShader.setMat4("view", view);
        mainShader.setMat4("model", model);
        mainShader.setInt("diffuseTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, piasekTexture);
        mainShader.setInt("normalMap", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalMapTexture);
        mainShader.setVec4("baseColor", 0.6f, 0.8f, 0.9f, 0.2f);
        rysujKwadrat();

        // Ryby
        modelShader.use();
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);

        modelShader.setFloat("time", currentFrame);

        for (int i = 0; i < stado.size(); i++) {
            glm::mat4 fishModelMatrix = glm::mat4(1.0f);

            fishModelMatrix = glm::rotate(fishModelMatrix, glm::radians(cubeAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
            fishModelMatrix = glm::rotate(fishModelMatrix, glm::radians(cubeAngleY), glm::vec3(0.0f, 1.0f, 0.0f));
            fishModelMatrix = glm::translate(fishModelMatrix, stado[i].position);

            float angleY = atan2(-stado[i].velocity.z, stado[i].velocity.x);
            fishModelMatrix = glm::rotate(fishModelMatrix, angleY, glm::vec3(0.0f, 1.0f, 0.0f));

            float speedXZ = sqrt(stado[i].velocity.x * stado[i].velocity.x + stado[i].velocity.z * stado[i].velocity.z);
            float angleZ = atan2(stado[i].velocity.y, speedXZ);
            fishModelMatrix = glm::rotate(fishModelMatrix, angleZ, glm::vec3(0.0f, 0.0f, 1.0f));

            fishModelMatrix = glm::scale(fishModelMatrix, glm::vec3(0.15f));
            fishModelMatrix = glm::rotate(fishModelMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            fishModelMatrix = glm::rotate(fishModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

            modelShader.setMat4("model", fishModelMatrix);

            float swimSpeed = glm::length(stado[i].velocity);
            modelShader.setFloat("swimSpeed", swimSpeed);
            modelShader.setFloat("randomOffset", (float)i * 0.1f);

            rybaModel.Draw(modelShader);
        }

        // Rośliny
        roslinaShader.use();
        roslinaShader.setMat4("projection", projection);
        roslinaShader.setMat4("view", view);
        roslinaShader.setFloat("time", currentFrame);
        roslinaShader.setInt("hasTexture", 0);
        roslinaShader.setVec3("matColor", glm::vec3(0.1f, 0.45f, 0.15f));

        spin = 0.0f;
        for (auto& r : rosliny) {
            spin += 0.5;
            glm::mat4 roslinaMatrix = glm::mat4(1.0f);
            roslinaMatrix = glm::rotate(roslinaMatrix, glm::radians(cubeAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
            roslinaMatrix = glm::rotate(roslinaMatrix, glm::radians(cubeAngleY), glm::vec3(0.0f, 1.0f, 0.0f));
            roslinaMatrix = glm::translate(roslinaMatrix, glm::vec3(r.x, -1.1f, r.z));
            roslinaMatrix = glm::rotate(roslinaMatrix, spin, glm::vec3(0.0f, -1.0f, 0.0f));
            roslinaMatrix = glm::scale(roslinaMatrix, glm::vec3(r.skala));

            roslinaShader.setMat4("model", roslinaMatrix);
            roslinaShader.setFloat("randomOffset", r.offset);
            roslinaModel.Draw(roslinaShader);
        }

        // Skrzynia
        chestShader.use();
        chestShader.setMat4("projection", projection);
        chestShader.setMat4("view", view);
        chestShader.setVec3("viewPos", camera.Position);

        glm::mat4 chestMatrix = glm::mat4(1.0f);
        chestMatrix = glm::rotate(chestMatrix, glm::radians(cubeAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
        chestMatrix = glm::rotate(chestMatrix, glm::radians(cubeAngleY), glm::vec3(0.0f, 1.0f, 0.0f));
        chestMatrix = glm::translate(chestMatrix, glm::vec3(1.6f, -1.0f, -0.65f));
        chestMatrix = glm::rotate(chestMatrix, 0.5f, glm::vec3(0.0f, -1.0f, 0.0f));
        chestMatrix = glm::scale(chestMatrix, glm::vec3(0.0035f));

        chestShader.setMat4("model", chestMatrix);
        chestShader.setInt("baseColorMap", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, chestBaseColor);
        chestShader.setInt("normalMap", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, chestNormal);
        chestShader.setInt("aoMap", 2);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, chestAO);
        skrzyniaModel.Draw(chestShader);

        // Skybox
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        glm::mat4 viewSkybox = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", viewSkybox);
        skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        glBindTexture(GL_TEXTURE_2D, screenTexture);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, currentWidth, currentHeight, 0);

        // Woda
        wodaShader.use();
        wodaShader.setMat4("projection", projection);
        wodaShader.setMat4("view", view);
        wodaShader.setMat4("model", model);
        wodaShader.setFloat("time", currentFrame);
        wodaShader.setVec3("viewPos", camera.Position);
        wodaShader.setInt("skybox", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        wodaShader.setInt("screenTexture", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, screenTexture);
        wodaShader.setVec2("resolution", glm::vec2((float)currentWidth, (float)currentHeight));
        wodaShader.setVec4("baseColor", 0.15f, 0.6f, 0.85f, 0.4f);
        rysujWode();

        // Akwarium
        szybaShader.use();
        szybaShader.setMat4("projection", projection);
        szybaShader.setMat4("view", view);
        szybaShader.setMat4("model", model);
        szybaShader.setVec3("viewPos", camera.Position);
        szybaShader.setVec4("baseColor", 0.6f, 0.8f, 0.9f, 0.2f);
        szybaShader.setInt("skybox", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        rysujAkwarium();

        // Marker Światła
        szybaShader.use();
        glm::mat4 lampaModel = glm::mat4(1.0f);
        lampaModel = glm::translate(lampaModel, lightPos);
        lampaModel = glm::scale(lampaModel, glm::vec3(0.05f));
        szybaShader.setMat4("model", lampaModel);
        szybaShader.setVec4("baseColor", 0.0f, 0.0f, 0.0f, 1.0f);
        glBindVertexArray(wodaVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        // DEBUGOWANIE
        szybaShader.use();
        szybaShader.setMat4("projection", projection);
        szybaShader.setMat4("view", view);
        rysujDebug(szybaShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &kwadratVAO);
    glDeleteBuffers(1, &kwadratVBO);
    glDeleteVertexArrays(1, &akwariumVAO);
    glDeleteBuffers(1, &akwariumVBO);
    glDeleteVertexArrays(1, &wodaVAO);
    glDeleteBuffers(1, &wodaVBO);
    glDeleteVertexArrays(1, &debugLineVAO);
    glDeleteBuffers(1, &debugLineVBO);
    glDeleteVertexArrays(1, &debugCircleVAO);
    glDeleteBuffers(1, &debugCircleVBO);

    glfwTerminate();
    return 0;
}

// FUNKCJE POMOCNICZE

unsigned int loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cout << "Błąd textur: " << faces[i] << std::endl;
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

unsigned int loadTexture2D(char const* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;
        else format = GL_RGB;

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(0, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(1, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(2, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(3, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    lightPitch += lightSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  lightPitch -= lightSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  lightYaw -= lightSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) lightYaw += lightSpeed * deltaTime;

    if (lightPitch > 89.0f) lightPitch = 89.0f;
    if (lightPitch < 5.0f) lightPitch = 5.0f;

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        if (!pKeyPressed) {
            showDebug = !showDebug;
            pKeyPressed = true;
        }
    }
    else {
        pKeyPressed = false;
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}