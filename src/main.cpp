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
 *  WASD / Setas       → Aplicar transformação nos eixos X/Y
 *  Q / E              → Aplicar transformação no eixo Z (neg/pos)
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

// GLAD - gerenciamento de OpenGL
#include <glad/glad.h>

// GLFW - janela e eventos
#include <GLFW/glfw3.h>

// GLM - matemática para OpenGL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

// ============================================
// ESTRUTURAS DE DADOS
// ============================================

// Representa a geometria carregada (VAO + quantidade de vértices)
struct Mesh {
    GLuint VAO;
    int nVertices;
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
    
    Object3D(const string& objName, const glm::vec3& color)
        : name(objName), 
          position(0.0f), 
          rotation(0.0f), 
          scale(1.0f), 
          baseColor(color) {}
    
    // Calcula e retorna a matriz model completa
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        
        // Aplica translação
        model = glm::translate(model, position);
        
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

enum TransformMode { TRANSLATE, ROTATE, SCALE };
TransformMode currentMode = TRANSLATE;

// ============================================
// PROTÓTIPOS DE FUNÇÃO
// ============================================

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void printControls();
void printStatus();
Mesh loadSimpleOBJ(const string& filePath, const glm::vec3& color = glm::vec3(0.6f));
GLuint setupShaderProgram();

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
    if (suzanneMesh.VAO != (GLuint)-1) {
        Object3D suzanne("Suzanne", glm::vec3(0.3f, 0.8f, 0.9f));
        suzanne.position = glm::vec3(-1.2f, 0.0f, 0.0f);
        suzanne.mesh = suzanneMesh;
        sceneObjects.push_back(suzanne);
    }
    
    // Cube - cor laranja, posição direita
    Mesh cubeMesh = loadSimpleOBJ(basePath + "Cube.obj", glm::vec3(1.0f, 0.5f, 0.2f));
    if (cubeMesh.VAO != (GLuint)-1) {
        Object3D cube("Cube", glm::vec3(1.0f, 0.5f, 0.2f));
        cube.position = glm::vec3(1.2f, 0.0f, 0.0f);
        cube.scale = glm::vec3(0.4f);
        cube.mesh = cubeMesh;
        sceneObjects.push_back(cube);
    }
    
    // Suzanne 2 - cor roxo, posição centro-superior (VAO e mesh separados)
    Mesh suzanne2Mesh = loadSimpleOBJ(basePath + "Suzanne.obj", glm::vec3(0.8f, 0.2f, 0.8f));
    if (suzanne2Mesh.VAO != (GLuint)-1) {
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
    
    // Câmera fixa: posição (0, 0, 5), olhando para origem
    // ... (calculada no loop)
    
    // --- LOOP PRINCIPAL ---
    while (!glfwWindowShouldClose(window))
    {
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
        
        // --- Desenha cada objeto ---
        for (size_t i = 0; i < sceneObjects.size(); i++) {
            // Destaca visualmente o objeto selecionado
            float isSelected = (i == selectedIndex) ? 1.0f : 0.0f;
            glUniform1f(glGetUniformLocation(shaderID, "highlight"), isSelected);
            
            sceneObjects[i].draw(shaderID);
        }
        
        glfwSwapBuffers(window);
    }
    
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
        return { (GLuint)-1, 0 };
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
                    vi = !index.empty() ? stoi(index) - 1 : 0;
                if (getline(ss, index, '/')) 
                    ti = !index.empty() ? stoi(index) - 1 : 0;
                if (getline(ss, index)) 
                    ni = !index.empty() ? stoi(index) - 1 : 0;
                
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
        return { (GLuint)-1, 0 };
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
    
    // T / R / S: modos de transformação
    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        currentMode = TRANSLATE;
        cout << "[MODO: TRANSLAÇÃO]" << endl;
        return;
    }
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        currentMode = ROTATE;
        cout << "[MODO: ROTAÇÃO]" << endl;
        return;
    }
    if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        currentMode = SCALE;
        cout << "[MODO: ESCALA]" << endl;
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
    
    // Aplica transformações baseadas no modo atual
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
        case TRANSLATE: modeStr = "TRANSLAÇÃO"; break;
        case ROTATE:    modeStr = "ROTAÇÃO";    break;
        case SCALE:     modeStr = "ESCALA";     break;
    }
    
    cout << fixed << setprecision(2);
    cout << "\n┌────────────────────────────────────────────┐" << endl;
    cout << "│  SELECIONADO: " << left << setw(30) << obj.name << "│" << endl;
    cout << "│  MODO: "       << left << setw(35) << modeStr  << "│" << endl;
    cout << "├────────────────────────────────────────────┤" << endl;
    cout << "│  Posição:  [" << obj.position.x << ", " << obj.position.y << ", " << obj.position.z << "]" << endl;
    cout << "│  Rotação:  [" << obj.rotation.x << ", " << obj.rotation.y << ", " << obj.rotation.z << "]" << endl;
    cout << "│  Escala:   [" << obj.scale.x << ", " << obj.scale.y << ", " << obj.scale.z << "]" << endl;
    cout << "└────────────────────────────────────────────┘" << endl;
}
