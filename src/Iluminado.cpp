/**
 * Tarefa 5 - Adicionando Iluminação
 *
 * Implementação do modelo de Phong:
 * - Leitura de normais (vn) do .OBJ
 * - Leitura de coeficientes de material (Ka, Kd, Ks, Ns) do .MTL
 * - Cálculo de iluminação ambiente + difusa + especular no fragment shader
 *
 * Controles:
 *  WASD/QE → Translação do objeto
 *  [ / ]   → Escala
 *  TAB     → Próximo objeto
 *  1/2/3   → Alterna componentes de iluminação (A/D/E)
 *  ESC     → Sair
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

const GLuint WIDTH = 800, HEIGHT = 800;

struct Material {
    glm::vec3 Ka;   // Ambient
    glm::vec3 Kd;   // Diffuse
    glm::vec3 Ks;   // Specular
    float Ns;        // Shininess
};

struct Mesh {
    GLuint VAO;
    int nVertices;
};

struct Object3D {
    Mesh mesh;
    string name;
    glm::vec3 position;
    float scale;
    Material material;
};

vector<Object3D> objects;
int selected = 0;

bool showAmbient = true;
bool showDiffuse = true;
bool showSpecular = true;

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
        case GLFW_KEY_1: showAmbient = !showAmbient; cout << "Ambient: " << (showAmbient ? "ON" : "OFF") << endl; break;
        case GLFW_KEY_2: showDiffuse = !showDiffuse; cout << "Diffuse: " << (showDiffuse ? "ON" : "OFF") << endl; break;
        case GLFW_KEY_3: showSpecular = !showSpecular; cout << "Specular: " << (showSpecular ? "ON" : "OFF") << endl; break;
        default: break;
    }
}

// Lê material do .MTL
Material loadMTL(const string& mtlPath) {
    Material mat;
    mat.Ka = glm::vec3(0.1f);
    mat.Kd = glm::vec3(0.8f, 0.8f, 0.8f);
    mat.Ks = glm::vec3(1.0f);
    mat.Ns = 32.0f;

    ifstream file(mtlPath);
    if (!file.is_open()) return mat;

    string line;
    while (getline(file, line)) {
        istringstream ss(line);
        string word;
        ss >> word;

        if (word == "Ka") ss >> mat.Ka.x >> mat.Ka.y >> mat.Ka.z;
        else if (word == "Kd") ss >> mat.Kd.x >> mat.Kd.y >> mat.Kd.z;
        else if (word == "Ks") ss >> mat.Ks.x >> mat.Ks.y >> mat.Ks.z;
        else if (word == "Ns") ss >> mat.Ns;
    }
    return mat;
}

// Carrega .OBJ com normais
Mesh loadOBJWithNormals(const string& objPath, Material& outMat) {
    vector<glm::vec3> vertices;
    vector<glm::vec3> normals;
    vector<GLfloat> vBuffer;

    string basePath = objPath.substr(0, objPath.find_last_of("/\\") + 1);

    // Lê MTL
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
                outMat = loadMTL(basePath + mtlName);
            }
        }
        objFile.close();
    }

    // Lê vértices e normais
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
        else if (word == "vn") {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (word == "f") {
            while (ss >> word) {
                istringstream ss(word);
                string index;
                int vi = 0, ni = 0;

                // v//vn
                if (getline(ss, index, '/')) vi = !index.empty() ? stoi(index) - 1 : 0;
                getline(ss, index, '/'); // pula vt
                if (getline(ss, index)) ni = !index.empty() ? stoi(index) - 1 : 0;

                // Posição
                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);

                // Normal
                if (ni >= 0 && ni < (int)normals.size()) {
                    vBuffer.push_back(normals[ni].x);
                    vBuffer.push_back(normals[ni].y);
                    vBuffer.push_back(normals[ni].z);
                } else {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(1.0f);
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

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    // Posição (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    int nVertices = vBuffer.size() / 6;
    cout << "  OBJ: " << objPath << " (" << nVertices << " vértices)" << endl;
    cout << "  Ka=" << outMat.Ka.x << "," << outMat.Ka.y << "," << outMat.Ka.z
         << " Kd=" << outMat.Kd.x << "," << outMat.Kd.y << "," << outMat.Kd.z
         << " Ks=" << outMat.Ks.x << "," << outMat.Ks.y << "," << outMat.Ks.z
         << " Ns=" << outMat.Ns << endl;

    return {VAO, nVertices};
}

GLuint setupShader() {
    const GLchar* vertexSource = "#version 330 core\n"
        "layout (location = 0) in vec3 position;\n"
        "layout (location = 1) in vec3 normal;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "out vec3 vNormal;\n"
        "out vec3 vFragPos;\n"
        "void main() {\n"
        "    vec4 worldPos = model * vec4(position, 1.0);\n"
        "    vFragPos = worldPos.xyz;\n"
        "    vNormal = mat3(transpose(inverse(model))) * normal;\n"
        "    gl_Position = projection * view * worldPos;\n"
        "}\n";

    const GLchar* fragmentSource = "#version 330 core\n"
        "in vec3 vNormal;\n"
        "in vec3 vFragPos;\n"
        "out vec4 FragColor;\n"
        "uniform vec3 lightPos;\n"
        "uniform vec3 viewPos;\n"
        "uniform vec3 Ka;\n"
        "uniform vec3 Kd;\n"
        "uniform vec3 Ks;\n"
        "uniform float Ns;\n"
        "uniform bool showAmbient;\n"
        "uniform bool showDiffuse;\n"
        "uniform bool showSpecular;\n"
        "void main() {\n"
        "    vec3 norm = normalize(vNormal);\n"
        "    vec3 lightDir = normalize(lightPos - vFragPos);\n"
        "    vec3 viewDir = normalize(viewPos - vFragPos);\n"
        "    vec3 reflectDir = reflect(-lightDir, norm);\n"
        "    // Ambient\n"
        "    vec3 ambient = showAmbient ? Ka * vec3(0.3) : vec3(0.0);\n"
        "    // Diffuse\n"
        "    float diff = max(dot(norm, lightDir), 0.0);\n"
        "    vec3 diffuse = showDiffuse ? Kd * diff * vec3(1.0) : vec3(0.0);\n"
        "    // Specular\n"
        "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), Ns);\n"
        "    vec3 specular = showSpecular ? Ks * spec * vec3(1.0) : vec3(0.0);\n"
        "    vec3 result = ambient + diffuse + specular;\n"
        "    FragColor = vec4(result, 1.0);\n"
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

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Tarefa 5 - Iluminacao (Phong)", nullptr, nullptr);
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

    string basePath = "../assets/Modelos3D/";

    Material mat1;
    Mesh suzanneMesh = loadOBJWithNormals(basePath + "Suzanne.obj", mat1);
    if (suzanneMesh.VAO != (GLuint)-1) {
        Object3D obj;
        obj.mesh = suzanneMesh;
        obj.name = "Suzanne";
        obj.position = glm::vec3(-1.5f, 0.0f, 0.0f);
        obj.scale = 1.0f;
        obj.material = mat1;
        objects.push_back(obj);
    }

    Material mat2;
    Mesh cubeMesh = loadOBJWithNormals(basePath + "Cube.obj", mat2);
    if (cubeMesh.VAO != (GLuint)-1) {
        Object3D obj;
        obj.mesh = cubeMesh;
        obj.name = "Cube";
        obj.position = glm::vec3(1.5f, 0.0f, 0.0f);
        obj.scale = 0.5f;
        obj.material = mat2;
        objects.push_back(obj);
    }

    if (objects.empty()) {
        cerr << "Nenhum modelo carregado!" << endl;
        return -1;
    }

    cout << "\n=== Tarefa 5 - Iluminacao (Phong) ===" << endl;
    cout << "Controles:" << endl;
    cout << "  WASD/QE → Translação" << endl;
    cout << "  [ / ]   → Escala" << endl;
    cout << "  TAB     → Próximo objeto" << endl;
    cout << "  1/2/3   → Alterna A/D/E" << endl;

    glm::vec3 lightPos(2.0f, 2.0f, 2.0f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
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
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), 0.0f, 0.0f, 5.0f);

        glUniform1i(glGetUniformLocation(shaderProgram, "showAmbient"), showAmbient);
        glUniform1i(glGetUniformLocation(shaderProgram, "showDiffuse"), showDiffuse);
        glUniform1i(glGetUniformLocation(shaderProgram, "showSpecular"), showSpecular);

        for (size_t i = 0; i < objects.size(); i++) {
            const Object3D& obj = objects[i];

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::scale(model, glm::vec3(obj.scale));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(glGetUniformLocation(shaderProgram, "Ka"), 1, glm::value_ptr(obj.material.Ka));
            glUniform3fv(glGetUniformLocation(shaderProgram, "Kd"), 1, glm::value_ptr(obj.material.Kd));
            glUniform3fv(glGetUniformLocation(shaderProgram, "Ks"), 1, glm::value_ptr(obj.material.Ks));
            glUniform1f(glGetUniformLocation(shaderProgram, "Ns"), obj.material.Ns);

            glBindVertexArray(obj.mesh.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.mesh.nVertices);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
