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

// ryba
struct Boid {
    glm::vec3 position;
    glm::vec3 velocity;
};

// Struktura dopasowana co do bajta do std140 w shaderach
struct LightUBO {
    glm::vec3 position;
    float cutOff;
    glm::vec3 direction;
    float outerCutOff;
    glm::vec3 color;
    float constant;
    float linear;
    float quadratic;
    float pad1, pad2; // Wypełnienie do pełnych 64 bajtów
};

struct ChestLightUBO {
    glm::vec3 position;
    float intensity; 
    glm::vec3 color;
    float pad;
};

const int NUM_BOIDS = 150; // liczba ryb
std::vector<Boid> stado;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);

// okno i kamera
const unsigned int SCR_WIDTH = 1800;
const unsigned int SCR_HEIGHT = 1350;
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

float cubeAngleX = 0.0f;
float cubeAngleY = 0.0f;
float cubeRotationSpeed = 100.0f;

float lightYaw = -90.0f;   // Kąt w poziomie
float lightPitch = 45.0f;  // Kąt w pionie (wysokość)
float lightRadius = 5.0f;  // Odległość od środka
float lightSpeed = 60.0f;  // Prędkość poruszania lampą

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float spin = 0.0f;

unsigned int kwadratVAO, kwadratVBO;
unsigned int akwariumVAO, akwariumVBO;
unsigned int wodaVAO, wodaVBO;
unsigned int skyboxVAO, skyboxVBO;

unsigned int cubemapTexture;

// FUNKCJE Ryba
void initBoids() {
    for (int i = 0; i < NUM_BOIDS; i++) {
        Boid b;
        // Losowa pozycja wewnątrz akwarium
        b.position = glm::vec3(
            ((rand() % 200) / 100.0f - 1.0f) * 1.5f,  // X: od -1.5 do 1.5
            ((rand() % 200) / 100.0f - 1.0f) * 0.5f,  // Y: od -0.5 do 0.5
            ((rand() % 200) / 100.0f - 1.0f) * 0.7f   // Z: od -0.7 do 0.7
        );
        // Losowy kierunek i prędkość
        b.velocity = glm::vec3(
            ((rand() % 200) / 100.0f - 1.0f),
            ((rand() % 200) / 100.0f - 1.0f) * 0.2f, // Pływają głównie poziomo
            ((rand() % 200) / 100.0f - 1.0f)
        );
        b.velocity = glm::normalize(b.velocity) * 0.5f; // Stała prędkość startowa
        stado.push_back(b);
    }
}

void aktualizujBoids(float dt) {
    float visualRange = 0.3f;     //zasięg widzenia
    float protectedRange = 0.25f; //unikanie przeszkód
    float matchingFactor = 0.05f; //moc dopasowania kierunku
    float centeringFactor = 0.005f; //moc dążenia do środka
    float avoidFactor = 0.07f;    //unikanie ryb
    float turnFactor = 0.7f;      //moc skręcania
    float minSpeed = 0.2f;      //prędkość min
    float maxSpeed = 0.4f;      //prędkość max

    for (int i = 0; i < stado.size(); i++) {
        glm::vec3 separation(0.0f);
        glm::vec3 alignment(0.0f);
        glm::vec3 cohesion(0.0f);
        int neighboringBoids = 0;

        for (int j = 0; j < stado.size(); j++) {
            if (i == j) continue;

            float dist = glm::distance(stado[i].position, stado[j].position);

            if (dist < protectedRange) {
                separation += (stado[i].position - stado[j].position);
            }
            else if (dist < visualRange) {
                alignment += stado[j].velocity;
                cohesion += stado[j].position;
                neighboringBoids++;
            }
        }

        if (neighboringBoids > 0) {
            alignment /= neighboringBoids;
            cohesion /= neighboringBoids;

            stado[i].velocity += (alignment - stado[i].velocity) * matchingFactor;
            stado[i].velocity += (cohesion - stado[i].position) * centeringFactor;
        }
        stado[i].velocity += separation * avoidFactor;

        float minX = -1.8f, maxX = 1.8f;
        float minY = -0.8f, maxY = 0.7f;
        float minZ = -0.8f, maxZ = 0.8f;

        float turnFactor = 2.0f;

        if (stado[i].position.x < minX) stado[i].velocity.x += turnFactor * dt;
        if (stado[i].position.x > maxX) stado[i].velocity.x -= turnFactor * dt;
        if (stado[i].position.y < minY) stado[i].velocity.y += turnFactor * dt;
        if (stado[i].position.y > maxY) stado[i].velocity.y -= turnFactor * dt;
        if (stado[i].position.z < minZ) stado[i].velocity.z += turnFactor * dt;
        if (stado[i].position.z > maxZ) stado[i].velocity.z -= turnFactor * dt;

        float speed = glm::length(stado[i].velocity);
        if (speed < minSpeed) stado[i].velocity = (stado[i].velocity / speed) * minSpeed;
        if (speed > maxSpeed) stado[i].velocity = (stado[i].velocity / speed) * maxSpeed;

        stado[i].position += stado[i].velocity * dt;

        if (stado[i].position.x < minX) {
            stado[i].position.x = minX;
            stado[i].velocity.x *= -1.0f;
        }
        if (stado[i].position.x > maxX) {
            stado[i].position.x = maxX;
            stado[i].velocity.x *= -1.0f;
        }

        if (stado[i].position.y < minY) {
            stado[i].position.y = minY;
            stado[i].velocity.y *= -1.0f;
        }
        if (stado[i].position.y > maxY) {
            stado[i].position.y = maxY;
            stado[i].velocity.y *= -1.0f;
        }

        if (stado[i].position.z < minZ) {
            stado[i].position.z = minZ;
            stado[i].velocity.z *= -1.0f;
        }
        if (stado[i].position.z > maxZ) {
            stado[i].position.z = maxZ;
            stado[i].velocity.z *= -1.0f;
        }
    }
}

// FUNKCJE KWADRAT
void initKwadrat() {
    float vertices[] = {
        // POZYCJA (3)         // NORMALNA (3)      // UV (2)     // TANGENT (3)
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

    // Atrybut 0: Pozycja (3 floaty)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Atrybut 1: Normalne (3 floaty)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Atrybut 2: UV (2 floaty)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Atrybut 3: Tangent (3 floaty)
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
}

void rysujKwadrat() {
    glBindVertexArray(kwadratVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// FUNKCJE AKWARIUM
void initAkwarium() {
    
    float vertices[] = {
        // TYLNA ŚCIANA
            // Pozycja            // Normalne         // UV
        -2.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         2.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         2.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         2.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -2.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -2.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        // PRZEDNIA ŚCIANA
        -2.0f, -1.0f,  1.0f,   0.0f,  0.0f, 1.0f,  0.0f, 0.0f,
         2.0f, -1.0f,  1.0f,   0.0f,  0.0f, 1.0f,  1.0f, 0.0f,
         2.0f,  1.0f,  1.0f,   0.0f,  0.0f, 1.0f,  1.0f, 1.0f,
         2.0f,  1.0f,  1.0f,   0.0f,  0.0f, 1.0f,  1.0f, 1.0f,
        -2.0f,  1.0f,  1.0f,   0.0f,  0.0f, 1.0f,  0.0f, 1.0f,
        -2.0f, -1.0f,  1.0f,   0.0f,  0.0f, 1.0f,  0.0f, 0.0f,

        // LEWA ŚCIANA
        -2.0f, -1.0f,  1.0f,  -1.0f,  0.0f, 0.0f,  0.0f, 0.0f,
        -2.0f, -1.0f, -1.0f,  -1.0f,  0.0f, 0.0f,  1.0f, 0.0f,
        -2.0f,  1.0f, -1.0f,  -1.0f,  0.0f, 0.0f,  1.0f, 1.0f,
        -2.0f,  1.0f, -1.0f,  -1.0f,  0.0f, 0.0f,  1.0f, 1.0f,
        -2.0f,  1.0f,  1.0f,  -1.0f,  0.0f, 0.0f,  0.0f, 1.0f,
        -2.0f, -1.0f,  1.0f,  -1.0f,  0.0f, 0.0f,  0.0f, 0.0f,

        // PRAWA ŚCIANA
         2.0f, -1.0f,  1.0f,   1.0f,  0.0f, 0.0f,  0.0f, 0.0f,
         2.0f, -1.0f, -1.0f,   1.0f,  0.0f, 0.0f,  1.0f, 0.0f,
         2.0f,  1.0f, -1.0f,   1.0f,  0.0f, 0.0f,  1.0f, 1.0f,
         2.0f,  1.0f, -1.0f,   1.0f,  0.0f, 0.0f,  1.0f, 1.0f,
         2.0f,  1.0f,  1.0f,   1.0f,  0.0f, 0.0f,  0.0f, 1.0f,
         2.0f, -1.0f,  1.0f,   1.0f,  0.0f, 0.0f,  0.0f, 0.0f,

        // DNO
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

    // Atrybut 0: Pozycja (3 floaty)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Atrybut 1: Normalne (3 floaty)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Atrybut 2: UV (2 floaty)
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

// FUNKCJE WODA
void initWoda() {
    float vertices[] = {
        // POZYCJA (X, Y, Z)        // NORMALNA (X, Y, Z)   // UV (U, V)

        // GÓRA (Lustro wody: Y = 0.8f)
        -1.99f,  0.8f, -0.99f,      0.0f, 1.0f, 0.0f,       0.0f, 0.0f,
         1.99f,  0.8f, -0.99f,      0.0f, 1.0f, 0.0f,       1.0f, 0.0f,
         1.99f,  0.8f,  0.99f,      0.0f, 1.0f, 0.0f,       1.0f, 1.0f,
         1.99f,  0.8f,  0.99f,      0.0f, 1.0f, 0.0f,       1.0f, 1.0f,
        -1.99f,  0.8f,  0.99f,      0.0f, 1.0f, 0.0f,       0.0f, 1.0f,
        -1.99f,  0.8f, -0.99f,      0.0f, 1.0f, 0.0f,       0.0f, 0.0f,

        // DÓŁ (Dno wody: Y = -0.99f)
        -1.99f, -0.99f, -0.99f,     0.0f, -1.0f, 0.0f,      0.0f, 0.0f,
        -1.99f, -0.99f,  0.99f,     0.0f, -1.0f, 0.0f,      0.0f, 1.0f,
         1.99f, -0.99f,  0.99f,     0.0f, -1.0f, 0.0f,      1.0f, 1.0f,
         1.99f, -0.99f,  0.99f,     0.0f, -1.0f, 0.0f,      1.0f, 1.0f,
         1.99f, -0.99f, -0.99f,     0.0f, -1.0f, 0.0f,      1.0f, 0.0f,
        -1.99f, -0.99f, -0.99f,     0.0f, -1.0f, 0.0f,      0.0f, 0.0f,

        // PRZÓD (Z = 0.99f)
        -1.99f, -0.99f,  0.99f,     0.0f, 0.0f, 1.0f,       0.0f, 0.0f,
         1.99f, -0.99f,  0.99f,     0.0f, 0.0f, 1.0f,       1.0f, 0.0f,
         1.99f,  0.8f,   0.99f,     0.0f, 0.0f, 1.0f,       1.0f, 1.0f,
         1.99f,  0.8f,   0.99f,     0.0f, 0.0f, 1.0f,       1.0f, 1.0f,
        -1.99f,  0.8f,   0.99f,     0.0f, 0.0f, 1.0f,       0.0f, 1.0f,
        -1.99f, -0.99f,  0.99f,     0.0f, 0.0f, 1.0f,       0.0f, 0.0f,

        // TYŁ (Z = -0.99f)
         1.99f, -0.99f, -0.99f,     0.0f, 0.0f, -1.0f,      0.0f, 0.0f,
        -1.99f, -0.99f, -0.99f,     0.0f, 0.0f, -1.0f,      1.0f, 0.0f,
        -1.99f,  0.8f,  -0.99f,     0.0f, 0.0f, -1.0f,      1.0f, 1.0f,
        -1.99f,  0.8f,  -0.99f,     0.0f, 0.0f, -1.0f,      1.0f, 1.0f,
         1.99f,  0.8f,  -0.99f,     0.0f, 0.0f, -1.0f,      0.0f, 1.0f,
         1.99f, -0.99f, -0.99f,     0.0f, 0.0f, -1.0f,      0.0f, 0.0f,

         // LEWO (X = -1.99f)
         -1.99f, -0.99f, -0.99f,    -1.0f, 0.0f, 0.0f,       0.0f, 0.0f,
         -1.99f, -0.99f,  0.99f,    -1.0f, 0.0f, 0.0f,       1.0f, 0.0f,
         -1.99f,  0.8f,   0.99f,    -1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
         -1.99f,  0.8f,   0.99f,    -1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
         -1.99f,  0.8f,  -0.99f,    -1.0f, 0.0f, 0.0f,       0.0f, 1.0f,
         -1.99f, -0.99f, -0.99f,    -1.0f, 0.0f, 0.0f,       0.0f, 0.0f,

         // PRAWO (X = 1.99f)
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
        // --- POPRAWIONY BLOK WYBORU FORMATU ---
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else
            format = GL_RGB; // Zabezpieczenie domyślne
        // --------------------------------------

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glBindTexture(GL_TEXTURE_2D, textureID);
        // ... reszta pozostaje bez zmian
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Kafelkowanie tekstury
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

void initSkybox() {
    float skyboxVertices[] = {
        // pozycje          
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
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

    // kursora myszy
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glewInit() != GLEW_OK) return -1;

    glEnable(GL_DEPTH_TEST);

    // kompilacja shaderów
    Shader mainShader("shaders/norm-v.glsl", "shaders/norm-f.glsl");
    Shader modelShader("shaders/ryba-v.glsl", "shaders/ryba-f.glsl");
    Shader szybaShader("shaders/szyba-v.glsl", "shaders/szyba-f.glsl");
    Shader wodaShader("shaders/woda-v.glsl", "shaders/woda-f.glsl");
    Shader skyboxShader("shaders/skybox-v.glsl", "shaders/skybox-f.glsl");
    Shader roslinaShader("shaders/roslina-v.glsl", "shaders/roslina-f.glsl");
    Shader chestShader("shaders/chest-v.glsl", "shaders/chest-f.glsl");
 
    unsigned int blockIndex = glGetUniformBlockIndex(mainShader.ID, "LightBlock");
    glUniformBlockBinding(mainShader.ID, blockIndex, 0);
    blockIndex = glGetUniformBlockIndex(modelShader.ID, "LightBlock");
    glUniformBlockBinding(modelShader.ID, blockIndex, 0);
    blockIndex = glGetUniformBlockIndex(chestShader.ID, "LightBlock");
    glUniformBlockBinding(chestShader.ID, blockIndex, 0);
    blockIndex = glGetUniformBlockIndex(wodaShader.ID, "LightBlock");
    glUniformBlockBinding(wodaShader.ID, blockIndex, 0);
    blockIndex = glGetUniformBlockIndex(roslinaShader.ID, "LightBlock");
    glUniformBlockBinding(roslinaShader.ID, blockIndex, 0);
    blockIndex = glGetUniformBlockIndex(szybaShader.ID, "LightBlock");
    glUniformBlockBinding(szybaShader.ID, blockIndex, 0);

    // razem z istniejącym blokiem glUniformBlockBinding dla LightBlock
    auto bindBlock = [](GLuint programID, const char* name, GLuint binding) {
        GLuint idx = glGetUniformBlockIndex(programID, name);
        if (idx != GL_INVALID_INDEX)
            glUniformBlockBinding(programID, idx, binding);
    };

    bindBlock(mainShader.ID, "ChestLightBlock", 1);
    bindBlock(modelShader.ID, "ChestLightBlock", 1);
    bindBlock(chestShader.ID, "ChestLightBlock", 1);
    bindBlock(roslinaShader.ID, "ChestLightBlock", 1);
    bindBlock(wodaShader.ID, "ChestLightBlock", 1);
    bindBlock(szybaShader.ID, "ChestLightBlock", 1);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    std::vector<std::string> faces = {
        "texture/clouds/clouds1_east.bmp",
        "texture/clouds/clouds1_west.bmp",
        "texture/clouds/clouds1_up.bmp",
        "texture/clouds/clouds1_down.bmp",
        "texture/clouds/clouds1_north.bmp",
        "texture/clouds/clouds1_south.bmp",
    };

    //ryba
    unsigned int piasekTexture = loadTexture2D("texture/sand/GroundSand005_COL_1K.jpg"); // kolor
    unsigned int normalMapTexture = loadTexture2D("texture/sand/GroundSand005_NRM_1K.jpg"); // wypukłość

    //skrzynia
    unsigned int chestBaseColor = loadTexture2D("models/chest/Material_Base_color.png");
    unsigned int chestNormal = loadTexture2D("models/chest/Material_Normal_OpenGL.png");
    unsigned int chestAO = loadTexture2D("models/chest/Material_Mixed_AO.png");
 
    Model rybaModel("models/fish/13009_Coral_Beauty_Angelfish_v1_l3.obj");
    Model roslinaModel("models/plant/plant.obj");
    Model skrzyniaModel("models/chest/chest.obj");

    // Generowanie bufora UBO dla światła
    unsigned int uboLight;
    glGenBuffers(1, &uboLight);
    glBindBuffer(GL_UNIFORM_BUFFER, uboLight);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(LightUBO), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboLight);

    unsigned int uboChestLight;
    glGenBuffers(1, &uboChestLight);
    glBindBuffer(GL_UNIFORM_BUFFER, uboChestLight);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ChestLightUBO), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, uboChestLight);


    //lampa nad akwarium
    glm::vec3 lightPos = glm::vec3(0.0f, 4.0f, 0.0f);   // 4 jednostki nad środkiem
    glm::vec3 lightDir = glm::vec3(0.0f, -1.0f, 0.0f);  // Świeci prosto w dół
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f); // Białe światło
    float cutOff = glm::cos(glm::radians(25.0f));       // Główny strumień światła
    float outerCutOff = glm::cos(glm::radians(35.0f));  // Zewnętrzny (rozmyty) brzeg

    cubemapTexture = loadCubemap(faces);
    initSkybox();
    initBoids();
    initAkwarium();
    initWoda();
    initKwadrat();

    // NOWE: Tekstura do refrakcji (załamania światła)
    unsigned int screenTexture;
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    // Tworzymy pustą teksturę o rozmiarze okna
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
        // Obliczanie nowej pozycji światła na półsferze
        lightPos.x = lightRadius * cos(glm::radians(lightPitch)) * cos(glm::radians(lightYaw));
        lightPos.y = lightRadius * sin(glm::radians(lightPitch));
        lightPos.z = lightRadius * cos(glm::radians(lightPitch)) * sin(glm::radians(lightYaw));

        // Światło zawsze patrzy w punkt 0,0,0
        lightDir = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - lightPos);

        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(cubeAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(cubeAngleY), glm::vec3(0.0f, 1.0f, 0.0f));

        // Zbieramy aktualne dane światła do struktury
        LightUBO lightData;
        lightData.position = lightPos;
        lightData.cutOff = cutOff;
        lightData.direction = lightDir;
        lightData.outerCutOff = outerCutOff;
        lightData.color = lightColor;
        lightData.constant = 1.0f;
        lightData.linear = 0.04f;
        lightData.quadratic = 0.015f;

        // Wysyłamy paczkę RAZ do karty graficznej
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

        modelShader.use();
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);

        for (int i = 0; i < stado.size(); i++) {
            glm::mat4 fishModelMatrix = glm::mat4(1.0f);

            //pozycja akwarium
            fishModelMatrix = glm::rotate(fishModelMatrix, glm::radians(cubeAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
            fishModelMatrix = glm::rotate(fishModelMatrix, glm::radians(cubeAngleY), glm::vec3(0.0f, 1.0f, 0.0f));
            fishModelMatrix = glm::translate(fishModelMatrix, stado[i].position);

            // ruch ryby
            float angleY = atan2(-stado[i].velocity.z, stado[i].velocity.x);
            fishModelMatrix = glm::rotate(fishModelMatrix, angleY, glm::vec3(0.0f, 1.0f, 0.0f));

            // animacja
            float swimSpeed = glm::length(stado[i].velocity);
            float wobble = sin(currentFrame * 25.0f * swimSpeed + i) * 0.25f;
            fishModelMatrix = glm::rotate(fishModelMatrix, wobble, glm::vec3(0.0f, 1.0f, 0.0f));

            float speedXZ = sqrt(stado[i].velocity.x * stado[i].velocity.x + stado[i].velocity.z * stado[i].velocity.z);
            float angleZ = atan2(stado[i].velocity.y, speedXZ);
            fishModelMatrix = glm::rotate(fishModelMatrix, angleZ, glm::vec3(0.0f, 0.0f, 1.0f));

            // skala ryby
            fishModelMatrix = glm::scale(fishModelMatrix, glm::vec3(0.15f));

            fishModelMatrix = glm::rotate(fishModelMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            fishModelMatrix = glm::rotate(fishModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

            modelShader.setMat4("model", fishModelMatrix);
            rybaModel.Draw(modelShader);
        }

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

        // --- RYSOWANIE SKRZYNI ---
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

        // Rysujemy
        skrzyniaModel.Draw(chestShader);

        // skybox
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

        // RYSOWANIE MARKERA ŚWIATŁA (czarny kwadracik)
        szybaShader.use();
        glm::mat4 lampaModel = glm::mat4(1.0f);
        lampaModel = glm::translate(lampaModel, lightPos);
        lampaModel = glm::scale(lampaModel, glm::vec3(0.05f)); // Zmniejszamy do rozmiaru kropki

        szybaShader.setMat4("model", lampaModel);
        szybaShader.setVec4("baseColor", 0.0f, 0.0f, 0.0f, 1.0f); // Całkowicie czarny

        // Rysujemy za pomocą modelu wody (ma 36 wierzchołków = pełna kostka)
        glBindVertexArray(wodaVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &kwadratVAO);
    glDeleteBuffers(1, &kwadratVBO);
    glDeleteVertexArrays(1, &akwariumVAO);
    glDeleteBuffers(1, &akwariumVBO);
    glDeleteVertexArrays(1, &wodaVAO);
    glDeleteBuffers(1, &wodaVBO);

    glfwTerminate();
    return 0;
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