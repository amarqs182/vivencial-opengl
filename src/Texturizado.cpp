/**
 * Tarefa 3 - Adicionando Texturas
 *
 * Leitura de coordenadas de textura (vt) do arquivo .OBJ,
 * leitura do nome da textura do arquivo .MTL,
 * aplicação de textura no objeto.
 *
 * Controles:
 *  TAB    → Próximo objeto
 *  WASD   → Translação
 *  Q/E    → Translação Z
 *  [ / ]  → Escala diminui/aumenta
 *  R      → Reseta posição
 *  ESC    → Sair
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

const GLuint WIDTH = 800, HEIGHT = 800;

struct Mesh {
    GLuint VAO;
    int nVertices;
};

struct Object3D {
    Mesh mesh;
    string name;
    glm::vec3 position;
    float scale;
    GLuint textureID;
};

vector<Object3D> objects;
int selected = 0;

// Callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }

    if (objects.empty()) return;

    Object3D& obj = objects[selected];
    const float T_STEP = 0.1f;

    switch (key) {
        case GLFW_KEY_W: obj.position.y += T_STEP; break;
        case GLFW_KEY_S: obj.position.y -= T_STEP; break;
        case GLFW_KEY_A: obj.position.x -= T_STEP; break;
        case GLFW_KEY_D: obj.position.x += T_STEP; break;
        case GLFW_KEY_Q: obj.position.z -= T_STEP; break;
        case GLFW_KEY_E: obj.position.z += T_STEP; break;
        case GLFW_KEY_LEFT_BRACKET:  obj.scale -= 0.1f; if (obj.scale < 0.1f) obj.scale = 0.1f; break;
        case GLFW_KEY_RIGHT_BRACKET: obj.scale += 0.1f; break;
        case GLFW_KEY_TAB: selected = (selected + 1) % objects.size(); break;
        case GLFW_KEY_R:
            obj.position = glm::vec3(0.0f);
            obj.scale = 1.0f;
            break;
        default: break;
    }
}

// Carrega textura de arquivo
GLuint loadTexture(const string& path) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        cout << "  Textura carregada: " << path << " (" << width << "x" << height << ", " << nrChannels << " canais)" << endl;
    } else {
        cout << "  Erro ao carregar textura: " << path << endl;
        textureID = 0;
    }
    stbi_image_free(data);

    return textureID;
}

// Lê nome da textura do arquivo .MTL
string loadMTLTexture(const string& mtlPath) {
    ifstream file(mtlPath);
    if (!file.is_open()) return "";

    string line;
    while (getline(file, line)) {
        istringstream ss(line);
        string word;
        ss >> word;
        if (word == "map_Kd") {
            string texPath;
            ss >> texPath;
            return texPath;
        }
    }
    return "";
}

// Carrega modelo .OBJ com suporte a texturas
Mesh loadOBJWithTexture(const string& objPath, const string& texPath, GLuint& outTexture) {
    vector<glm::vec3> vertices;
    vector<glm::vec2> texCoords;
    vector<GLfloat> vBuffer;

    // Descobre o diretório do .OBJ para encontrar .MTL e texturas
    string basePath = objPath.substr(0, objPath.find_last_of("/\\") + 1);

    // Tenta carregar .MTL
    string mtlTexture;
    ifstream objFile(objPath);
    if (objFile.is_open()) {
        string line;
        while (getline(objFile, line)) {
            istringstream ss(line);
            string word;
            ss >> word;
            if (word == "mtllib") {
                string mtlName;
                ss >> mtlName;
                mtlTexture = loadMTLTexture(basePath + mtlName);
            }
        }
        objFile.close();
    }

    // Lê vértices e coordenadas de textura
    objFile.open(objPath);
    if (!objFile.is_open()) {
        cerr << "Erro ao abrir: " << objPath << endl;
        return {(GLuint)-1, 0};
    }

    string line;
    while (getline(objFile, line)) {
        istringstream ss(line);
        string word;
        ss >> word;

        if (word == "v") {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (word == "vt") {
            glm::vec2 vt;
            ss >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "f") {
            while (ss >> word) {
                istringstream ss(word);
                string index;
                int vi = 0, ti = 0;

                if (getline(ss, index, '/'))
                    vi = !index.empty() ? stoi(index) - 1 : 0;
                if (getline(ss, index, '/'))
                    ti = !index.empty() ? stoi(index) - 1 : 0;

                // Posição
                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);

                // Coordenada de textura
                if (ti >= 0 && ti < (int)texCoords.size()) {
                    vBuffer.push_back(texCoords[ti].s);
                    vBuffer.push_back(texCoords[ti].t);
                } else {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                }
            }
        }
    }
    objFile.close();

    if (vBuffer.empty()) {
        cerr << "Arquivo vazio: " << objPath << endl;
        return {(GLuint)-1, 0};
    }

    // Cria VAO/VBO
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    // Posição (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Coordenada de textura (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    int nVertices = vBuffer.size() / 5;

    // Carrega textura
    outTexture = 0;
    if (!mtlTexture.empty()) {
        string fullTexPath = basePath + mtlTexture;
        outTexture = loadTexture(fullTexPath);
    }

    cout << "  OBJ carregado: " << objPath << " (" << nVertices << " vértices)" << endl;

    return {VAO, nVertices};
}

// Shaders
GLuint setupShader() {
    const GLchar* vertexSource = "#version 330 core\n"
        "layout (location = 0) in vec3 position;\n"
        "layout (location = 1) in vec2 texCoord;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "out vec2 vTexCoord;\n"
        "void main() {\n"
        "    gl_Position = projection * view * model * vec4(position, 1.0);\n"
        "    vTexCoord = texCoord;\n"
        "}\n";

    const GLchar* fragmentSource = "#version 330 core\n"
        "in vec2 vTexCoord;\n"
        "out vec4 color;\n"
        "uniform sampler2D texture1;\n"
        "uniform bool hasTexture;\n"
        "uniform vec3 baseColor;\n"
        "void main() {\n"
        "    if (hasTexture)\n"
        "        color = texture(texture1, vTexCoord);\n"
        "    else\n"
        "        color = vec4(baseColor, 1.0);\n"
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

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Tarefa 3 - Texturizado", nullptr, nullptr);
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint shaderProgram = setupShader();

    // Carrega modelos
    string basePath = "../assets/Modelos3D/";

    GLuint tex1;
    Mesh suzanneMesh = loadOBJWithTexture(basePath + "Suzanne.obj", basePath + "Suzanne.png", tex1);
    if (suzanneMesh.VAO != (GLuint)-1) {
        Object3D obj;
        obj.mesh = suzanneMesh;
        obj.name = "Suzanne";
        obj.position = glm::vec3(-1.5f, 0.0f, 0.0f);
        obj.scale = 1.0f;
        obj.textureID = tex1;
        objects.push_back(obj);
    }

    GLuint tex2;
    Mesh cubeMesh = loadOBJWithTexture(basePath + "Cube.obj", "", tex2);
    if (cubeMesh.VAO != (GLuint)-1) {
        Object3D obj;
        obj.mesh = cubeMesh;
        obj.name = "Cube";
        obj.position = glm::vec3(1.5f, 0.0f, 0.0f);
        obj.scale = 0.5f;
        obj.textureID = 0;
        objects.push_back(obj);
    }

    if (objects.empty()) {
        cerr << "Nenhum modelo carregado!" << endl;
        return -1;
    }

    cout << "\n=== Tarefa 3 - Texturizado ===" << endl;
    cout << "Objetos: " << objects.size() << endl;
    cout << "Controles:" << endl;
    cout << "  WASD/QE → Translação" << endl;
    cout << "  [ / ]   → Escala" << endl;
    cout << "  TAB     → Próximo objeto" << endl;
    cout << "  R       → Reseta posição" << endl;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

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

        for (size_t i = 0; i < objects.size(); i++) {
            const Object3D& obj = objects[i];

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::scale(model, glm::vec3(obj.scale));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

            // Configura textura
            if (obj.textureID > 0) {
                glUniform1i(glGetUniformLocation(shaderProgram, "hasTexture"), 1);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, obj.textureID);
                glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
            } else {
                glUniform1i(glGetUniformLocation(shaderProgram, "hasTexture"), 0);
                glUniform3f(glGetUniformLocation(shaderProgram, "baseColor"), 0.6f, 0.4f, 0.2f);
            }

            glBindVertexArray(obj.mesh.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.mesh.nVertices);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
