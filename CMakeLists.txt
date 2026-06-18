cmake_minimum_required(VERSION 3.20)
project(AgentBasedModeling CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
endif()

find_package(glm REQUIRED)
find_package(SDL2 REQUIRED)

set(SOURCES
    main.cpp
    src/System.cpp
    src/Agent.cpp
    src/Env.cpp
    src/Pathfinding.cpp
    src/Buttons.cpp
    src/Statistics.cpp
    src/ChatBox.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glm::glm
    SDL2::SDL2
)

if(APPLE)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -pedantic -O3
    )
endif()
