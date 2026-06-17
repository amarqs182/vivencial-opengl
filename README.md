# Vivencial - Visualizador 3D de Arquivos .OBJ

Atividade vivencial da disciplina de Computação Gráfica — Unisinos

Leitura de arquivos Wavefront `.OBJ` e aplicação de transformações geométricas em múltiplos objetos 3D usando OpenGL.

## Funcionalidades

- ✅ Leitura de arquivos `.OBJ` (geometria: vértices e faces)
- ✅ Exibição de múltiplos objetos 3D na cena
- ✅ Seleção de objetos via teclado (TAB cicla entre objetos)
- ✅ Visualmente destaca o objeto selecionado
- ✅ Translação (T) nos eixos x, y e z
- ✅ Rotação (R) nos eixos x, y e z
- ✅ Escala (S) nos eixos x, y e z + escala uniforme
- ✅ Cores distintas para cada objeto
- ✅ Iluminação Phong (ambiente + difusa + especular)
- ✅ Trajetórias cíclicas com interpolação linear
- ✅ Salvar/carregar trajetórias em arquivo

## Estrutura do Projeto

```
vivencial-opengl/
├── CMakeLists.txt          # Build configuration
├── README.md               # Este arquivo
├── src/
│   ├── main.cpp            # Código principal (OBJ loader, transforms, render, trajectories)
│   ├── Camera.cpp          # Implementação da classe Camera
│   ├── Trajectory.cpp      # Implementação da classe Trajectory
│   ├── TrajectoryDemo.cpp  # Exercício separado de trajetórias com Phong
│   ├── Hello3D.cpp         # Tarefa 1
│   ├── CuboInstanciado.cpp # Tarefa 2
│   ├── Texturizado.cpp     # Tarefa 3
│   ├── Hello3DCamera.cpp   # Tarefa 4
│   ├── Iluminado.cpp       # Tarefa 5
│   └── glad.c              # OpenGL loader (GLAD)
├── include/
│   ├── Camera.h            # Header da classe Camera
│   ├── Trajectory.h        # Header da classe Trajectory
│   └── glad/
│       ├── glad.h
│       └── KHR/
│           └── khrplatform.h
├── assets/
│   └── Modelos3D/          # Modelos .OBJ (Suzanne, Cube, etc.)
│       ├── Suzanne.obj
│       ├── Cube.obj
│       └── SuzanneSubdiv1.obj
└── build/                  # Diretório de build (gerado pelo CMake)
```

## Dependências

| Biblioteca | Instalação (Arch Linux) | Instalação (Debian/Ubuntu) |
|---|---|---|
| GLFW 3 | `sudo pacman -S glfw-x11` | `sudo apt install libglfw3-dev` |
| GLM | `sudo pacman -S glm` | `sudo apt install libglm-dev` |
| CMake | `sudo pacman -S cmake` | `sudo apt install cmake` |
| GCC/G++ | `sudo pacman -S gcc` | `sudo apt install build-essential` |
| Mesa (OpenGL) | `sudo pacman -S mesa` | `sudo apt install libgl1-mesa-dev` |

> **Nota:** O GLAD já está incluído no projeto (`src/glad.c` + `include/glad/glad.h`).

## Compilação e Execução

```bash
# Clone o repositório
git clone <URL_DO_SEU_REPOSITORIO>
cd vivencial-opengl

# Crie o diretório de build
mkdir -p build && cd build

# Configure e compile
cmake ..
make

# Execute (a partir do diretório build, para que o caminho ../assets/ funcione)
./VivencialOBJViewer
```

> **Importante:** O executável deve ser rodado a partir do diretório `build/` para que o caminho relativo `../assets/Modelos3D/` seja resolvido corretamente.

## Controles

| Tecla | Ação |
|---|---|
| **TAB** | Seleciona o próximo objeto (ciclado) |
| **T** | Modo Translação |
| **R** | Modo Rotação |
| **S** | Modo Escala |
| **ESC** | Sair do programa |

### Translação (modo T)

| Tecla | Eixo |
|---|---|
| W / ↑ | Y+ |
| S / ↓ | Y- |
| A / ← | X- |
| D / → | X+ |
| Q | Z- |
| E | Z+ |

### Rotação (modo R)

| Tecla | Eixo |
|---|---|
| W / ↑ | Rotaciona eixo X+ |
| S / ↓ | Rotaciona eixo X- |
| A / ← | Rotaciona eixo Y- |
| D / → | Rotaciona eixo Y+ |
| Q | Rotaciona eixo Z- |
| E | Rotaciona eixo Z+ |

### Escala (modo S)

| Tecla | Eixo |
|---|---|
| W / ↑ | Escala Y+ |
| S / ↓ | Escala Y- |
| A / ← | Escala X- |
| D / → | Escala X+ |
| Q | Escala Z- |
| E | Escala Z+ |
| Page Up | Escala uniforme + |
| Page Down | Escala uniforme - |

## Estruturas de Dados

### `struct Mesh`
Armazena os dados de geometria carregados do arquivo `.OBJ`:
- `VAO` — Vertex Array Object (OpenGL)
- `nVertices` — Número de vértices para `glDrawArrays`

### `struct Object3D`
Representa um objeto completo na cena:
- `mesh` — Geometria (Mesh)
- `name` — Nome do objeto (para identificação no console)
- `position` — Vetor de translação (x, y, z)
- `rotation` — Vetor de rotação em graus (x, y, z)
- `scale` — Vetor de escala (x, y, z)
- `baseColor` — Cor do objeto
- `draw(shaderID)` — Calcula a matriz model e desenha o objeto

### Armazenamento
Os objetos são armazenados em um `std::vector<Object3D>`:
```cpp
vector<Object3D> sceneObjects;
```

### `struct Trajectory`
Gerencia trajetórias cíclicas para objetos:
- `points` — Lista de pontos de controle (vector<glm::vec3>)
- `playing` — Se a trajetória está em ciclo (bool)
- `speed` — Velocidade de movimento (float)
- `progress` — Progresso entre dois pontos [0,1] (float)
- `segmentIndex` — Índice do segmento atual (int)
- `getCurrentPosition()` — Retorna posição interpolada
- `advance(deltaTime)` — Avança o progresso
- `addPoint(p)` — Adiciona ponto de controle
- `clear()` — Limpa todos os pontos

## Sobre o Loader OBJ

A função `loadSimpleOBJ` lê o arquivo `.OBJ` linha por linha e processa:
- `v x y z` → Posições dos vértices
- `vt s t` → Coordenadas de textura (armazenadas mas não utilizadas ainda)
- `vn nx ny nz` → Vetores normais (armazenados mas não utilizados ainda)
- `f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3` → Faces (triângulos)

Para cada vértice da face, o buffer contém: `x, y, z, r, g, b` (posição + cor), totalizando 6 floats por vértice.

## Tarefa 4 — Câmera em Primeira Pessoa (2026-06-02)

Implementação de uma câmera em primeira pessoa como uma classe `Camera` que encapsula posição, orientação (yaw/pitch) e parâmetros de projeção, expondo métodos `Mover` (`processKeyboard`) e `Rotacionar` (`processMouseMovement`).

### Arquivos adicionados

| Arquivo | Função |
|---|---|
| `include/Camera.h` | Header da classe `Camera` |
| `src/Camera.cpp` | Implementação da classe |
| `src/Hello3DCamera.cpp` | Programa principal: cubo renderizado com câmera FPS |
| `src/Hello3DCamera.md` | Documentação detalhada da entrega (matemática, controles, design) |

### Controles do `Hello3DCamera`

| Tecla | Ação | Método chamado |
|---|---|---|
| W / S | Mover frente / trás | `processKeyboard(FORWARD/BACKWARD)` |
| A / D | Strafe esquerda / direita | `processKeyboard(LEFT/RIGHT_DIR)` |
| Q / E | Descer / subir (eixo Y) | `processKeyboard(DOWN/UP)` |
| Mouse | Olhar em volta (yaw/pitch) | `processMouseMovement(xoff, yoff)` |
| Scroll | Zoom (FOV 1° a 45°) | `processMouseScroll(yoff)` |
| ESC | Sair | — |

### Compilação

O `CMakeLists.txt` foi atualizado para gerar **dois executáveis**:

```bash
mkdir -p build && cd build
cmake ..
make

# Vivencial 1 (visualizador OBJ):
./VivencialOBJViewer

# Tarefa 4 (câmera FPS):
./Hello3DCamera
```

## Tarefa 5 — Iluminação Phong (2026-06-02)

Implementação do modelo de iluminação Phong com reflexão ambiente, difusa e especular.

### Conceitos

- **Ambiente (Ka):** Iluminação básica que simula luz indireta
- **Difusa (Kd):** Reflexão dependente do ângulo entre luz e superfície (Lambert)
- **Especular (Ks):** Brilho direcional (Phong tradicional)
- **Atenuação:** Luz diminui com a distância (Kc + Kl*d + Kq*d²)

### Arquivos

| Arquivo | Função |
|---|---|
| `src/Iluminado.cpp` | Exercício com iluminação Phong |

### Compilação

```bash
cd build
./Iluminado
```

## Tarefa 6 — Trajetórias (2026-06-16)

Sistema de trajetórias cíclicas para objetos da cena, com interpolação linear entre pontos de controle.

### Funcionalidades

- Adicionar pontos de controle na posição atual do objeto
- Translação cíclica (ao chegar no último ponto, volta ao primeiro)
- Interpolação linear entre pontos (LERP)
- Salvar/carregar pontos em arquivo
- Suporte a múltiplos objetos

### Implementação no `main.cpp`

A trajetória está integrada ao visualizador OBJ principal:

| Tecla | Ação |
|---|---|
| **P** | Entrar no modo Trajetória |
| **N** | Adiciona ponto na posição do objeto selecionado |
| **SPACE** | Play / Pause da trajetória |
| **F** | Salva trajetórias em arquivo (`trajetorias.txt`) |
| **L** | Carrega trajetórias de arquivo |
| **X** | Limpa trajetória do objeto selecionado |

### Estrutura de Dados

```cpp
struct Trajectory {
    vector<glm::vec3> points;    // Pontos de controle
    bool playing = false;        // Se está em ciclo
    float speed = 1.0f;          // Velocidade (unidades/seg)
    float progress = 0.0f;       // Progresso entre dois pontos [0,1]
    int segmentIndex = 0;        // Índice do segmento atual
    
    glm::vec3 getCurrentPosition(const glm::vec3& basePosition) const;
    void advance(float deltaTime);
    void addPoint(const glm::vec3& p);
    void clear();
};
```

### Exercício Separado: `TrajectoryDemo`

Um exercício separado demonstra o sistema de trajetórias com iluminação Phong:

```bash
cd build
./TrajectoryDemo
```

**Controles do `TrajectoryDemo`:**

| Tecla | Ação |
|---|---|
| **WASD/QE** | Mover câmera |
| **Mouse** | Olhar em volta |
| **TAB** | Trocar objeto selecionado |
| **1** | Adicionar ponto na posição do objeto |
| **2** | Remover último ponto |
| **3** | Limpar todos os pontos |
| **SPACE** | Ativar/desativar trajetória |
| **R** | Resetar trajetória |
| **UP/DOWN** | Ajustar velocidade |
| **S** | Salvar pontos em arquivo |
| **L** | Carregar pontos de arquivo |

## Compilação Geral

```bash
mkdir -p build && cd build
cmake ..
make

# Executáveis disponíveis:
./VivencialOBJViewer    # Principal (com trajetórias)
./TrajectoryDemo        # Exercício separado
./Hello3D               # Tarefa 1
./CuboInstanciado       # Tarefa 2
./Texturizado           # Tarefa 3
./Hello3DCamera         # Tarefa 4
./Iluminado             # Tarefa 5
```

## Configuração para Laboratórios Unisinos (Windows)

Para compilar nos laboratórios da Unisinos, siga o tutorial oficial:
https://github.com/guilhermechagaskurtz/CGCCHibrido/blob/main/tutorial_vivenciais.md

Resumo dos passos:
1. Verificar se `C:\msys64` existe (MSYS2 + MinGW)
2. Baixar CMake portable e Git portable
3. Configurar o PATH no `.vscode/settings.json`
4. Instalar extensões C/C++ e CMake Tools no VSCode
5. Copiar DLLs do MSYS2 para a pasta `build/`

## Referências

- [LearnOpenGL - Shaders](https://learnopengl.com/Getting-started/Shaders)
- [LearnOpenGL - Transformations](https://learnopengl.com/Getting-started/Transformations)
- [GLFW Input Guide](https://www.glfw.org/docs/latest/input_guide.html)
- [GLM Documentation](https://github.com/g-truc/glm)
- [Wavefront OBJ Specification](https://en.wikipedia.org/wiki/Wavefront_.obj_file)
- Código base: https://github.com/guilhermechagaskurtz/CGCCHibrido
