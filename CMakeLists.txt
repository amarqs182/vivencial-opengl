cmake_minimum_required(VERSION 3.10)
project(VivencialOBJViewer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenGL
find_package(OpenGL REQUIRED)

# GLFW via pkg-config (Arch Linux)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

# GLM - header-only
find_path(GLM_INCLUDE_DIR glm/glm.hpp
    HINTS /usr/include /usr/local/include /opt/homebrew/include
)
if(NOT GLM_INCLUDE_DIR)
    message(FATAL_ERROR "GLM not found. Install: sudo pacman -S glm")
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${GLFW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIR}
)

# Source files - vivencial (OBJ viewer)
set(SOURCES_VIVENCIAL
    ${CMAKE_SOURCE_DIR}/src/glad.c
    ${CMAKE_SOURCE_DIR}/src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES_VIVENCIAL})

target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    ${GLFW_LIBRARIES}
    dl
    m
)

# Tarefa 1 - Criando o ambiente de Programação de cenas 3D
set(SOURCES_HELLO3D
    ${CMAKE_SOURCE_DIR}/src/glad.c
    ${CMAKE_SOURCE_DIR}/src/Hello3D.cpp
)

add_executable(Hello3D ${SOURCES_HELLO3D})

target_link_libraries(Hello3D
    ${OPENGL_LIBRARIES}
    ${GLFW_LIBRARIES}
    dl
    m
)

# Tarefa 2 - Instanciando objetos na cena 3D
set(SOURCES_CUBO_INST
    ${CMAKE_SOURCE_DIR}/src/glad.c
    ${CMAKE_SOURCE_DIR}/src/CuboInstanciado.cpp
)

add_executable(CuboInstanciado ${SOURCES_CUBO_INST})

target_link_libraries(CuboInstanciado
    ${OPENGL_LIBRARIES}
    ${GLFW_LIBRARIES}
    dl
    m
)

# Tarefa 3 - Adicionando Texturas
set(SOURCES_TEXTURED
    ${CMAKE_SOURCE_DIR}/src/glad.c
    ${CMAKE_SOURCE_DIR}/src/Texturizado.cpp
)

add_executable(Texturizado ${SOURCES_TEXTURED})

target_link_libraries(Texturizado
    ${OPENGL_LIBRARIES}
    ${GLFW_LIBRARIES}
    dl
    m
)

# Tarefa 5 - Adicionando Iluminação
set(SOURCES_LIGHTING
    ${CMAKE_SOURCE_DIR}/src/glad.c
    ${CMAKE_SOURCE_DIR}/src/Iluminado.cpp
)

add_executable(Iluminado ${SOURCES_LIGHTING})

target_link_libraries(Iluminado
    ${OPENGL_LIBRARIES}
    ${GLFW_LIBRARIES}
    dl
    m
)

# Tarefa 4 - Câmera em primeira pessoa
# (Adicionada em 2026-06-02 conforme a Tarefa "Adicionando uma câmera em primeira pessoa")
set(SOURCES_CAMERA
    ${CMAKE_SOURCE_DIR}/src/glad.c
    ${CMAKE_SOURCE_DIR}/src/Camera.cpp
    ${CMAKE_SOURCE_DIR}/src/Hello3DCamera.cpp
)

add_executable(Hello3DCamera ${SOURCES_CAMERA})

target_link_libraries(Hello3DCamera
    ${OPENGL_LIBRARIES}
    ${GLFW_LIBRARIES}
    dl
    m
)

# Tarefa - Definindo trajetórias para alguns objetos
set(SOURCES_TRAJECTORY
    ${CMAKE_SOURCE_DIR}/src/glad.c
    ${CMAKE_SOURCE_DIR}/src/Camera.cpp
    ${CMAKE_SOURCE_DIR}/src/Trajectory.cpp
    ${CMAKE_SOURCE_DIR}/src/TrajectoryDemo.cpp
)

add_executable(TrajectoryDemo ${SOURCES_TRAJECTORY})

target_link_libraries(TrajectoryDemo
    ${OPENGL_LIBRARIES}
    ${GLFW_LIBRARIES}
    dl
    m
)
