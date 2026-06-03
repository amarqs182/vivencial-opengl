# Resultado - Tarefa 1: Criando o ambiente de Programação de cenas 3D

## Print da execução

![Hello3D - Cubo colorido por face](../Attachments/Hello3D-screenshots/cubo-colorido.png)

## Descrição

Programa exibe uma janela GLFW com título "Ola 3D -- Amarildo" contendo um cubo 3D com 6 faces coloridas (vermelho, verde, azul, amarelo, magenta, ciano).

## Controles

- **X** → Liga/desliga rotação no eixo X
- **Y** → Liga/desliga rotação no eixo Y
- **Z** → Liga/desliga rotação no eixo Z
- **ESC** → Sair

## Tecnologias

- C++ (padrão C++11)
- OpenGL 3.3+ via GLAD
- GLFW para janela e eventos
- GLM para matrizes

## Compilação

```bash
mkdir -p build && cd build && cmake .. && make Hello3D
./Hello3D
```

## Print de tela

![Cubo 3D colorido](../Attachments/Hello3D-screenshots/cubo-colorido.png)
