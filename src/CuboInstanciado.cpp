/**
 * Tarefa 2 - Instanciando objetos na cena 3D
 *
 * Cubo com cores por face, controle de translação/escala,
 * múltiplas instâncias na cena.
 *
 * Controles:
 *  W/S    → Translação Y+ / Y-
 *  A/D    → Translação X- / X+
 *  I/J    → Translação Y+ / Y- (alternativo)
 *  Q/E    → Translação Z- / Z+
 *  [      → Escala uniforme diminui
 *  ]      → Escala uniforme aumenta
 *  TAB    → Próximo objeto selecionado
 *  N      → Adiciona novo cubo na cena
 *  ESC    → Sair
 */

#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

const GLuint WIDTH = 800, HEIGHT = 800;

struct Cube {
    glm::vec3 position;
    float scale;
    glm::vec3 color;
};

vector<Cube> cubes;
int selected = 0;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }

    if (cubes.empty()) return;

    Cube& c = cubes[selected];

    const float T_STEP = 0.1f;
    const float S_STEP = 0.1f;

    switch (key) {
        // Translação
        case GLFW_KEY_W: c.position.y += T_STEP; break;
        case GLFW_KEY_S: c.position.y -= T_STEP; break;
        case GLFW_KEY_A: c.position.x -= T_STEP; break;
        case GLFW_KEY_D: c.position.x += T_STEP; break;
        case GLFW_KEY_I: c.position.y += T_STEP; break;
        case GLFW_KEY_J: c.position.y -= T_STEP; break;
        case GLFW_KEY_Q: c.position.z -= T_STEP; break;
        case GLFW_KEY_E: c.position.z += T_STEP; break;

        // Escala uniforme
        case GLFW_KEY_LEFT_BRACKET:  c.scale -= S_STEP; if (c.scale < 0.1f) c.scale = 0.1f; break;
        case GLFW_KEY_RIGHT_BRACKET: c.scale += S_STEP; break;

        // Selecionar próximo
        case GLFW_KEY_TAB: selected = (selected + 1) % cubes.size(); break;

        // Adicionar novo cubo
        case GLFW_KEY_N: {
            Cube newCube;
            newCube.position = glm::vec3(0.0f);
            newCube.scale = 0.5f;
            // Cores cíclicas para diferenciar
            static int colorIdx = 0;
            glm::vec3 colors[] = {
                glm::vec3(1,0,0), glm::vec3(0,1,0), glm::vec3(0,0,1),
                glm::vec3(1,1,0), glm::vec3(1,0,1), glm::vec3(0,1,1)
            };
            newCube.color = colors[colorIdx % 6];
            colorIdx++;
            cubes.push_back(newCube);
            selected = cubes.size() - 1;
            cout << "Cubo adicionado! Total: " << cubes.size() << endl;
            break;
        }
        default: break;
    }
}

GLuint setupShader() {
    const GLchar* vertexSource = "#version 330 core\n"
        "layout (location = 0) in vec3 position;\n"
        "layout (location = 1) in vec3 color;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "out vec3 vColor;\n"
        "void main() {\n"
        "    gl_Position = projection * view * model * vec4(position, 1.0);\n"
        "    vColor = color;\n"
        "}\n";

    const GLchar* fragmentSource = "#version 330 core\n"
        "in vec3 vColor;\n"
        "out vec4 color;\n"
        "void main() {\n"
        "    color = vec4(vColor, 1.0);\n"
        "}\n";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint setupCubeVAO() {
    // Cubo com 6 faces coloridas (cores embutidas nos vértices)
    float vertices[] = {
        // Frente (vermelho)
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
        // Trás (verde)
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
        // Direita (azul)
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
        // Esquerda (amarelo)
        -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
        // Cima (magenta)
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
        // Baixo (ciano)
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Tarefa 2 - Cubo Instanciado", nullptr, nullptr);
    if (!window) {
        cerr << "Falha ao criar janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "OpenGL: " << glGetString(GL_VERSION) << endl;

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    GLuint shaderProgram = setupShader();
    GLuint cubeVAO = setupCubeVAO();

    // Cubo inicial
    Cube initial;
    initial.position = glm::vec3(0.0f);
    initial.scale = 0.5f;
    initial.color = glm::vec3(1.0f, 0.5f, 0.2f);
    cubes.push_back(initial);

    cout << "\n=== Tarefa 2 - Cubo Instanciado ===" << endl;
    cout << "Controles:" << endl;
    cout << "  WASD/IJ → Translação" << endl;
    cout << "  Q/E     → Translação Z" << endl;
    cout << "  [ / ]   → Escala diminui/aumenta" << endl;
    cout << "  TAB     → Próximo objeto" << endl;
    cout << "  N       → Adiciona cubo" << endl;
    cout << "  ESC     → Sair" << endl;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Câmera fixa
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f
        );

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(cubeVAO);

        for (size_t i = 0; i < cubes.size(); i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubes[i].position);
            model = glm::scale(model, glm::vec3(cubes[i].scale));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

            // Aplica cor do cubo (multiplica com cores das faces)
            GLint modelLoc = glGetUniformLocation(shaderProgram, "model");

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glfwTerminate();
    return 0;
}
