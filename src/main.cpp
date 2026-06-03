/**
 * Vivencial - Model Viewer 3D
 *
 * Leitura de arquivos .OBJ e aplicação de transformações
 * em múltiplos objetos 3D.
 *
 * Controles:
 *  TAB                → Próximo objeto (seleção ciclada)
 *  T                  → Modo Translação
 *  R                  → Modo Rotação
 *  S                  → Modo Escala
 *  P                  → Modo Trajetória
 *  WASD / Setas       → Aplicar transformação nos eixos X/Y
 *  Q / E              → Aplicar transformação no eixo Z (neg/pos)
 *  N                  → Adiciona ponto na trajetória (no modo P)
 *  SPACE              → Play / Pause da trajetória (no modo P)
 *  F                  → Salva trajetórias em arquivo (no modo P)
 *  L                  → Carrega trajetórias de arquivo (no modo P)
 *  X                  → Limpa trajetória do objeto selecionado (no modo P)
 *  ESC                → Sair
 *
 * Compilação: mkdir build && cd build && cmake .. && make
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>

// GLAD - gerenciamento de OpenGL
#include <glad/glad.h>

// GLFW - janela e eventos
#include <GLFW/glfw3.h>

// GLM - matemática para OpenGL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

// Safe stoi with default value
int safeStoi(const string& s, int defaultVal = 0) {
    try { return stoi(s); }
    catch (...) { return defaultVal; }
}

// ============================================
// ESTRUTURAS DE DADOS
// ============================================

// Representa a geometria carregada (VAO + quantidade de vértices)
struct Mesh {
    GLuint VAO;
    int nVertices;
};

// Trajetória cíclica: lista de pontos de controle
struct Trajectory {
    vector<glm::vec3> points;    // Pontos de controle
    bool playing = false;        // Se está em ciclo
    float speed = 1.0f;          // Velocidade (unidades/seg)
    float progress = 0.0f;       // Progresso entre dois pontos [0,1]
    int segmentIndex = 0;        // Índice do segmento atual (points[i] → points[i+1])

    // Retorna posição atual interpolada (linear entre dois pontos)
    glm::vec3 getCurrentPosition(const glm::vec3& basePosition) const {
        if (points.size() < 2 || !playing)
            return basePosition;

        int n = points.size();
        int i = segmentIndex % n;
        int j = (segmentIndex + 1) % n;

        // Interpolação linear entre points[i] e points[j]
        return glm::mix(points[i], points[j], progress);
    }

    // Avança o progresso ao longo dos pontos. deltaTime em segundos.
    void advance(float deltaTime) {
        if (points.size() < 2 || !playing) return;

        int n = points.size();
        float segLen = 1.0f / n;  // Fração do ciclo para cada segmento
        progress += (speed * deltaTime) / segLen;

        while (progress >= 1.0f) {
            progress -= 1.0f;
            segmentIndex = (segmentIndex + 1) % n;
        }
    }

    void addPoint(const glm::vec3& p) {
        if (points.size() < 1000) {
            points.push_back(p);
        } else {
            cerr << "Limite de 1000 pontos atingido." << endl;
        }
    }

    void clear() {
        points.clear();
        playing = false;
        progress = 0.0f;
        segmentIndex = 0;
    }
};

// Representa um objeto da cena com suas transformações
struct Object3D {
    Mesh mesh;
    string name;

    // Transformações (armazenadas como separadas para edição fácil)
    glm::vec3 position;   // Translação (x, y, z)
    glm::vec3 rotation;   // Rotação em graus (x, y, z)
    glm::vec3 scale;      // Escala (x, y, z)

    // Cor padrão (diferente para cada objeto, para diferenciar)
    glm::vec3 baseColor;

    // Trajetória
    Trajectory trajectory;

    Object3D(const string& objName, const glm::vec3& color)
        : name(objName),
          position(0.0f),
          rotation(0.0f),
          scale(1.0f),
          baseColor(color) {}

    // Calcula e retorna a matriz model completa
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);

        // Posição final: translação manual + trajetória (se ativa)
        glm::vec3 finalPos = trajectory.getCurrentPosition(position);

        // Aplica translação
        model = glm::translate(model, finalPos);

        // Aplica rotação (ordem: X → Y → Z)
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        // Aplica escala
        model = glm::scale(model, scale);

        return model;
    }

    void draw(GLuint shaderID) const {
        // Configura a matriz model no shader
        GLint modelLoc = glGetUniformLocation(shaderID, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(getModelMatrix()));

        // Desenha o objeto
        glBindVertexArray(mesh.VAO);
        glDrawArrays(GL_TRIANGLES, 0, mesh.nVertices);
    }
};

// ============================================
// VARIÁVEIS GLOBAIS
// ============================================

const GLuint WIDTH = 800, HEIGHT = 800;
vector<Object3D> sceneObjects;
unsigned int selectedIndex = 0;

enum TransformMode { TRANSLATE, ROTATE, SCALE, TRAJECTORY };
TransformMode currentMode = TRANSLATE;

// ============================================
// PROTÓTIPOS DE FUNÇÃO
// ============================================

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void printControls();
void printStatus();
Mesh loadSimpleOBJ(const string& filePath, const glm::vec3& color = glm::vec3(0.6f));
GLuint setupShaderProgram();
bool saveTrajectories(const string& filename);
bool loadTrajectories(const string& filename);

// ============================================
// SHADERS GLSL
// ============================================

const GLchar* vertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float highlight;

out vec3 vertexColor;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);

    // Aplica o efeito de destaque quando o objeto está selecionado
    vertexColor = color * (1.0 + highlight * 0.3);
}
)";

const GLchar* fragmentShaderSource = R"(
#version 330 core

in vec3 vertexColor;
out vec4 outColor;

void main()
{
    outColor = vec4(vertexColor, 1.0);
}
)";

// ============================================
// MAIN
// ============================================

int main(int argc, char* argv[])
{
    // --- GLFW ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
        "Vivencial - Visualizador 3D [.OBJ]", nullptr, nullptr);
    if (!window) {
        cerr << "Falha ao criar janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);

    // GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Falha ao inicializar GLAD" << endl;
        glfwTerminate();
        return -1;
    }

    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* version = (const char*)glGetString(GL_VERSION);
    if (renderer && version) {
        cout << "Renderer: " << renderer << endl;
        cout << "OpenGL: " << version << endl;
    } else {
        cerr << "Erro ao obter informações do OpenGL" << endl;
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    // Compila o shader program
    GLuint shaderID = setupShaderProgram();

    // --- CENA: Carrega modelos ---
    string basePath = "../assets/Modelos3D/";

    // Tenta encontrar o diretório correto dos modelos
    ifstream testFile(basePath + "Suzanne.obj");
    if (!testFile.good()) {
        // Alternativa: procura relativo ao executável ou diretório atual
        basePath = "./assets/Modelos3D/";
        testFile.close();
        testFile.open(basePath + "Suzanne.obj");
        if (!testFile.good()) {
            // Tenta caminho direto
            basePath = "/home/asm/CGCCHibrido/assets/Modelos3D/";
        }
    }
    testFile.close();

    cout << "Diretório de modelos: " << basePath << endl;

    // Suzanne - cor ciano, posição esquerda
    Mesh suzanneMesh = loadSimpleOBJ(basePath + "Suzanne.obj", glm::vec3(0.3f, 0.8f, 0.9f));
    if (suzanneMesh.VAO != 0) {
        Object3D suzanne("Suzanne", glm::vec3(0.3f, 0.8f, 0.9f));
        suzanne.position = glm::vec3(-1.2f, 0.0f, 0.0f);
        suzanne.mesh = suzanneMesh;
        sceneObjects.push_back(suzanne);
    }

    // Cube - cor laranja, posição direita
    Mesh cubeMesh = loadSimpleOBJ(basePath + "Cube.obj", glm::vec3(1.0f, 0.5f, 0.2f));
    if (cubeMesh.VAO != 0) {
        Object3D cube("Cube", glm::vec3(1.0f, 0.5f, 0.2f));
        cube.position = glm::vec3(1.2f, 0.0f, 0.0f);
        cube.scale = glm::vec3(0.4f);
        cube.mesh = cubeMesh;
        sceneObjects.push_back(cube);
    }

    // Suzanne 2 - cor roxo, posição centro-superior (VAO e mesh separados)
    Mesh suzanne2Mesh = loadSimpleOBJ(basePath + "Suzanne.obj", glm::vec3(0.8f, 0.2f, 0.8f));
    if (suzanne2Mesh.VAO != 0) {
        Object3D suzanne2("Suzanne 2", glm::vec3(0.8f, 0.2f, 0.8f));
        suzanne2.position = glm::vec3(0.0f, 1.2f, 0.0f);
        suzanne2.scale = glm::vec3(0.5f);
        suzanne2.mesh = suzanne2Mesh;
        sceneObjects.push_back(suzanne2);
    }

    if (sceneObjects.empty()) {
        cerr << "Nenhum modelo carregado. Verifique o caminho dos arquivos .OBJ" << endl;
        return -1;
    }

    cout << "\n" << sceneObjects.size() << " objeto(s) carregado(s)." << endl;
    printControls();
    printStatus();

    // Tempo
    float lastFrame = 0.0f;

    // --- LOOP PRINCIPAL ---
    while (!glfwWindowShouldClose(window))
    {
        // DeltaTime
        float currentFrame = (float)glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        // Limpa buffers
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);  // Fundo azul escuro
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderID);

        // --- Matrizes de câmera ---
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),   // Posição da câmera
            glm::vec3(0.0f, 0.0f, 0.0f),   // Ponto de olhar
            glm::vec3(0.0f, 1.0f, 0.0f)    // Vetor "up"
        );

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),              // FOV
            (float)WIDTH / (float)HEIGHT,     // Aspect ratio
            0.1f, 100.0f                       // Near/far
        );

        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"),
                           1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"),
                           1, GL_FALSE, glm::value_ptr(projection));

        // --- Atualiza e desenha cada objeto ---
        for (size_t i = 0; i < sceneObjects.size(); i++) {
            // Atualiza trajetória
            sceneObjects[i].trajectory.advance(deltaTime);

            // Destaca visualmente o objeto selecionado
            float isSelected = (i == selectedIndex) ? 1.0f : 0.0f;
            glUniform1f(glGetUniformLocation(shaderID, "highlight"), isSelected);

            sceneObjects[i].draw(shaderID);
        }

        glfwSwapBuffers(window);
    }

    // Cleanup
    for (const auto& obj : sceneObjects) {
        glDeleteVertexArrays(1, &obj.mesh.VAO);
    }
    glDeleteProgram(shaderID);

    glfwTerminate();
    return 0;
}

// ============================================
// FUNÇÃO: Leitura de arquivo .OBJ
// ============================================

Mesh loadSimpleOBJ(const string& filePath, const glm::vec3& color) {
    vector<glm::vec3> vertices;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> normals;
    vector<GLfloat> vBuffer;

    ifstream arqEntrada(filePath.c_str());
    if (!arqEntrada.is_open()) {
        cerr << "Erro ao tentar ler o arquivo: " << filePath << endl;
        return { 0, 0 };
    }

    string line;
    while (getline(arqEntrada, line)) {
        istringstream ssline(line);
        string word;
        ssline >> word;

        if (word == "v") {
            // Vértice: v x y z
            glm::vec3 v;
            ssline >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (word == "vt") {
            // Coordenada de textura: vt s t
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn") {
            // Normal: vn nx ny nz
            glm::vec3 n;
            ssline >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (word == "f") {
            // Face: f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
            while (ssline >> word) {
                istringstream ss(word);
                string index;
                int vi = 0, ti = 0, ni = 0;

                // Parse dos índices com separadores '/'
                if (getline(ss, index, '/'))
                    vi = safeStoi(index) - 1;
                if (getline(ss, index, '/'))
                    ti = safeStoi(index) - 1;
                if (getline(ss, index))
                    ni = safeStoi(index) - 1;

                // Bounds check for vertices
                if (vi < 0 || vi >= (int)vertices.size()) {
                    cerr << "  Índice de vértice inválido: " << vi + 1 << endl;
                    continue;
                }

                // Adiciona posição do vértice ao buffer
                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);

                // Adiciona cor (fixa por enquanto)
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);
            }
        }
    }

    arqEntrada.close();

    if (vBuffer.empty()) {
        cerr << "Arquivo vazio ou inválido: " << filePath << endl;
        return { 0, 0 };
    }

    // --- Gera VBO e VAO ---
    GLuint VBO, VAO;

    // VBO: buffer com os dados dos vértices
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat),
                 vBuffer.data(), GL_STATIC_DRAW);

    // VAO: configuração dos atributos de vértice
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Atributo 0: posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Atributo 1: cor (r, g, b)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(GLfloat),
                          (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Desvincula buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    int nVertices = vBuffer.size() / 6;  // x, y, z, r, g, b

    cout << "  ✓ Carregado: " << filePath << " (" << nVertices << " vértices)" << endl;

    return { VAO, nVertices };
}

// ============================================
// FUNÇÃO: Compilação do Shader Program
// ============================================

GLuint setupShaderProgram() {
    // Vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Verifica erros de compilação do vertex shader
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cerr << "Erro no Vertex Shader:\n" << infoLog << endl;
    }

    // Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Verifica erros de compilação do fragment shader
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cerr << "Erro no Fragment Shader:\n" << infoLog << endl;
    }

    // Shader Program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Verifica erros de linkagem
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cerr << "Erro na linkagem do Shader Program:\n" << infoLog << endl;
    }

    // Limpa shaders individuais (já linkados no programa)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// ============================================
// FUNÇÃO: Salvar trajetórias em arquivo
// ============================================

bool saveTrajectories(const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Erro ao salvar: " << filename << endl;
        return false;
    }

    for (size_t i = 0; i < sceneObjects.size(); i++) {
        const Trajectory& t = sceneObjects[i].trajectory;
        file << "# Objeto: " << sceneObjects[i].name << endl;
        file << "objeto " << i << " pontos " << t.points.size() << endl;
        for (size_t j = 0; j < t.points.size(); j++) {
            file << "  " << t.points[j].x << " "
                 << t.points[j].y << " "
                 << t.points[j].z << endl;
        }
        file << endl;
    }

    file.close();
    cout << "Trajetórias salvas em: " << filename << endl;
    return true;
}

// ============================================
// FUNÇÃO: Carregar trajetórias de arquivo
// ============================================

bool loadTrajectories(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Erro ao carregar: " << filename << endl;
        return false;
    }

    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        istringstream ss(line);
        string token;
        ss >> token;

        if (token == "objeto") {
            int objIdx;
            int nPoints;
            ss >> objIdx >> token >> nPoints;

            if (objIdx >= 0 && objIdx < (int)sceneObjects.size()) {
                Trajectory& t = sceneObjects[objIdx].trajectory;
                t.clear();
                for (int j = 0; j < nPoints; j++) {
                    getline(file, line);
                    if (line.empty()) continue;
                    istringstream ps(line);
                    glm::vec3 p;
                    ps >> p.x >> p.y >> p.z;
                    t.addPoint(p);
                }
                cout << "  Carregados " << t.points.size()
                     << " pontos para " << sceneObjects[objIdx].name << endl;
            }
        }
    }

    file.close();
    cout << "Trajetórias carregadas de: " << filename << endl;
    return true;
}

// ============================================
// FUNÇÃO: Eventos de Teclado
// ============================================

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    // Esc: sai do programa
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }

    // TAB: seleciona próximo objeto
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        selectedIndex = (selectedIndex + 1) % sceneObjects.size();
        printStatus();
        return;
    }

    // T / R / S / P: modos de transformação
    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        currentMode = TRANSLATE;
        cout << "[MODO: TRANSLAÇÃO]" << endl;
        printStatus();
        return;
    }
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        currentMode = ROTATE;
        cout << "[MODO: ROTAÇÃO]" << endl;
        printStatus();
        return;
    }
    if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        currentMode = SCALE;
        cout << "[MODO: ESCALA]" << endl;
        printStatus();
        return;
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        currentMode = TRAJECTORY;
        cout << "[MODO: TRAJETÓRIA]" << endl;
        printStatus();
        return;
    }

    // Verifica se há objetos selecionáveis
    if (sceneObjects.empty()) return;

    // Referência ao objeto selecionado
    Object3D& obj = sceneObjects[selectedIndex];

    // Passos de transformação
    const float T_STEP = 0.1f;     // Passo de translação
    const float R_STEP = 5.0f;     // Passo de rotação (graus)
    const float S_STEP = 0.1f;     // Passo de escala

    // --- MODO TRAJETÓRIA (P) ---
    if (currentMode == TRAJECTORY) {
        // N: adiciona ponto当前位置
        if (key == GLFW_KEY_N && action == GLFW_PRESS) {
            obj.trajectory.addPoint(obj.position);
            cout << "  Ponto adicionado em [" << obj.position.x << ", "
                 << obj.position.y << ", " << obj.position.z << "]"
                 << " (total: " << obj.trajectory.points.size() << ")" << endl;
            printStatus();
            return;
        }

        // SPACE: play/pause
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            if (obj.trajectory.points.size() >= 2) {
                obj.trajectory.playing = !obj.trajectory.playing;
                cout << "  Trajetória: " << (obj.trajectory.playing ? "PLAY" : "PAUSE") << endl;
            } else {
                cout << "  Precisa de pelo menos 2 pontos para iniciar." << endl;
            }
            printStatus();
            return;
        }

        // F: salvar todas as trajetórias
        if (key == GLFW_KEY_F && action == GLFW_PRESS) {
            saveTrajectories("trajetorias.txt");
            return;
        }

        // L: carregar trajetórias
        if (key == GLFW_KEY_L && action == GLFW_PRESS) {
            loadTrajectories("trajetorias.txt");
            return;
        }

        // X: limpar trajetória do objeto selecionado
        if (key == GLFW_KEY_X && action == GLFW_PRESS) {
            obj.trajectory.clear();
            cout << "  Trajetória de " << obj.name << " limpa." << endl;
            printStatus();
            return;
        }

        return;  // No modo TRAJETÓRIA, WASD não faz nada
    }

    // --- MODOS DE TRANSFORMAÇÃO (T/R/S) ---
    if (currentMode == TRANSLATE) {
        if (key == GLFW_KEY_W || key == GLFW_KEY_UP)    obj.position.y += T_STEP;
        if (key == GLFW_KEY_S || key == GLFW_KEY_DOWN)  obj.position.y -= T_STEP;
        if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT)  obj.position.x -= T_STEP;
        if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) obj.position.x += T_STEP;
        if (key == GLFW_KEY_Q)                          obj.position.z -= T_STEP;
        if (key == GLFW_KEY_E)                          obj.position.z += T_STEP;
    }
    else if (currentMode == ROTATE) {
        if (key == GLFW_KEY_W || key == GLFW_KEY_UP)    obj.rotation.x += R_STEP;
        if (key == GLFW_KEY_S || key == GLFW_KEY_DOWN)  obj.rotation.x -= R_STEP;
        if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT)  obj.rotation.y -= R_STEP;
        if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) obj.rotation.y += R_STEP;
        if (key == GLFW_KEY_Q)                          obj.rotation.z -= R_STEP;
        if (key == GLFW_KEY_E)                          obj.rotation.z += R_STEP;
    }
    else if (currentMode == SCALE) {
        if (key == GLFW_KEY_W || key == GLFW_KEY_UP)    obj.scale.y += S_STEP;
        if (key == GLFW_KEY_S || key == GLFW_KEY_DOWN)    obj.scale.y -= S_STEP;
        if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT)    obj.scale.x -= S_STEP;
        if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT)   obj.scale.x += S_STEP;
        if (key == GLFW_KEY_Q)                            obj.scale.z -= S_STEP;
        if (key == GLFW_KEY_E)                            obj.scale.z += S_STEP;

        // Escala uniforme (setas superior/inferior)
        if (key == GLFW_KEY_PAGE_UP)   obj.scale += glm::vec3(S_STEP);
        if (key == GLFW_KEY_PAGE_DOWN) obj.scale -= glm::vec3(S_STEP);

        // Garante escala mínima positiva para evitar inversão
        if (obj.scale.x < 0.01f) obj.scale.x = 0.01f;
        if (obj.scale.y < 0.01f) obj.scale.y = 0.01f;
        if (obj.scale.z < 0.01f) obj.scale.z = 0.01f;
    }

    printStatus();
}

// ============================================
// FUNÇÃO: Mostra os controles no console
// ============================================

void printControls() {
    cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                    CONTROLES DO VISUALIZADOR                  ║
╠══════════════════════════════════════════════════════════════╣
║  TAB          → Seleciona próximo objeto                      ║
║  T            → Modo Translação                               ║
║  R            → Modo Rotação                                  ║
║  S            → Modo Escala                                   ║
║  P            → Modo Trajetória                               ║
║                                                                ║
║  Translate (T):                                                ║
║    W/S / ↑↓   → Y+, Y-                                        ║
║    A/D / ←→   → X-, X+                                        ║
║    Q/E        → Z-, Z+                                        ║
║                                                                ║
║  Rotate (R):                                                   ║
║    W/S / ↑↓   → Rotaciona eixo X                              ║
║    A/D / ←→   → Rotaciona eixo Y                              ║
║    Q/E        → Rotaciona eixo Z                              ║
║                                                                ║
║  Scale (S):                                                    ║
║    W/S / ↑↓   → Escala Y                                      ║
║    A/D / ←→   → Escala X                                      ║
║    Q/E        → Escala Z                                      ║
║    PageUp/Down→ Escala uniforme (+/-)                         ║
║                                                                ║
║  Trajetória (P):                                               ║
║    N          → Adiciona ponto当前位置                         ║
║    SPACE      → Play / Pause                                  ║
║    F          → Salva em arquivo                              ║
║    L          → Carrega de arquivo                            ║
║    X          → Limpa trajetória                              ║
║                                                                ║
║  ESC          → Sair                                          ║
╚══════════════════════════════════════════════════════════════╝
)" << endl;
}

// ============================================
// FUNÇÃO: Mostra status atual
// ============================================

void printStatus() {
    if (sceneObjects.empty()) return;

    const Object3D& obj = sceneObjects[selectedIndex];
    string modeStr;
    switch (currentMode) {
        case TRANSLATE:   modeStr = "TRANSLAÇÃO";  break;
        case ROTATE:      modeStr = "ROTAÇÃO";     break;
        case SCALE:       modeStr = "ESCALA";      break;
        case TRAJECTORY:  modeStr = "TRAJETÓRIA";  break;
    }

    cout << fixed << setprecision(2);
    cout << "\n┌────────────────────────────────────────────┐" << endl;
    cout << "│  SELECIONADO: " << left << setw(30) << obj.name << "│" << endl;
    cout << "│  MODO: "       << left << setw(35) << modeStr  << "│" << endl;
    cout << "├────────────────────────────────────────────┤" << endl;
    cout << "│  Posição:  [" << obj.position.x << ", " << obj.position.y << ", " << obj.position.z << "]" << endl;
    cout << "│  Rotação:  [" << obj.rotation.x << ", " << obj.rotation.y << ", " << obj.rotation.z << "]" << endl;
    cout << "│  Escala:   [" << obj.scale.x << ", " << obj.scale.y << ", " << obj.scale.z << "]" << endl;

    // Mostra info da trajetória
    const Trajectory& t = obj.trajectory;
    cout << "├────────────────────────────────────────────┤" << endl;
    cout << "│  Trajetória: " << t.points.size() << " pontos" << endl;
    if (t.points.size() >= 2) {
        cout << "│  Status: " << (t.playing ? "PLAY" : "PAUSE") << endl;
        if (t.playing) {
            int seg = t.segmentIndex % t.points.size();
            int nxt = (seg + 1) % t.points.size();
            cout << "│  Segmento: " << seg << " → " << nxt
                 << " (progresso: " << t.progress * 100 << "%)" << endl;
        }
    }
    cout << "└────────────────────────────────────────────┘" << endl;
}
