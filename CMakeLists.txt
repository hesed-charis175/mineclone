cmake_minimum_required(VERSION 3.16)
project(MinecraftClone CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

add_executable(minecraft_clone
    src/main.cpp
    src/Shader.cpp
    src/BiomeMap.cpp
    src/Camera.cpp
    src/Texture.cpp
    src/stb_image_impl.cpp
    src/Chunk.cpp
    src/ChunkManager.cpp
    src/PerlinNoise.cpp
)

target_include_directories(minecraft_clone PRIVATE
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(minecraft_clone PRIVATE
    ${SDL2_LIBRARIES}
    GLEW::GLEW
    OpenGL::GL
    glm::glm
)

# Copy shaders and texture assets next to the binary so relative paths work
add_custom_command(TARGET minecraft_clone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:minecraft_clone>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:minecraft_clone>/assets
)
