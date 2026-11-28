cmake_minimum_required(VERSION 3.20)
project(ClimateVisualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. Find Vulkan SDK (Works automatically if LunarG Vulkan SDK is installed on Windows/Mac)
find_package(Vulkan REQUIRED)

# 2. Automatically download GLFW, GLM, and nlohmann_json if not found locally
include(FetchContent)

find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    message(STATUS "Fetching GLFW...")
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
endif()

find_package(glm QUIET)
if(NOT glm_FOUND)
    message(STATUS "Fetching GLM...")
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
    )
    FetchContent_MakeAvailable(glm)
endif()

# 3. Explicitly list your project files (prevents scanning the build/ folder!)
set(PROJECT_SOURCES
    src/VulkanSetup.cpp
    src/VulkanSetup.h
    src/VulkanSurface.cpp
    src/VulkanSurface.h
    src/Swapchain.cpp
    src/Swapchain.h
    src/DepthResources.cpp
    src/DepthResources.h
    src/GraphicsPipeline.cpp
    src/GraphicsPipeline.h
    src/ComputeShaders.cpp
    src/ComputeShaders.h
    src/VulkanBuffer.cpp
    src/VulkanBuffer.h
    src/FrameSync.cpp
    src/FrameSync.h
    src/Synchronization.cpp
    src/Synchronization.h
    src/Vertex.h
    src/drawingIn3D.cpp
    src/drawingIn3D.h
    src/Camera.cpp
    src/Camera.h
    src/Texture.cpp
    src/Texture.h
)

add_executable(ClimateVisualizer ${PROJECT_SOURCES})

# 4. Include directories
target_include_directories(ClimateVisualizer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${Vulkan_INCLUDE_DIRS}
)

# 5. Link libraries
target_link_libraries(ClimateVisualizer PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
)

# 6. Windows Visual Studio specific settings (prevents min/max macro bugs)
if(WIN32)
    target_compile_definitions(ClimateVisualizer PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

# =========================================================================
# Task T2: Synthetic Climate Generator & JSON-to-.bin Preprocessor CLI
# =========================================================================
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(ClimatePreprocessor
    tools/preprocessor/Preprocessor.cpp
)
target_link_libraries(ClimatePreprocessor PRIVATE nlohmann_json::nlohmann_json)
if(WIN32)
    target_compile_definitions(ClimatePreprocessor PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()