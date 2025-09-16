cmake_minimum_required(VERSION 3.22)

project(olsr_lite VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OLSR_LITE_WITH_GUI "Build GUI with ImGui/GLFW" ON)

include(FetchContent)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.0
)

FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG v2.0.6
  SOURCE_SUBDIR cmake
)

FetchContent_MakeAvailable(nlohmann_json glfw imgui glad)

glad_add_library(glad_gl_core_43 STATIC REPRODUCIBLE LOADER API gl:core=4.3)
set(GLAD_TGT glad_gl_core_43)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_library(imgui_lib STATIC
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_compile_definitions(imgui_lib PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD2)
target_link_libraries(imgui_lib PUBLIC glfw ${GLAD_TGT})

if(NOT TARGET olsr_core)
  file(GLOB_RECURSE ALL_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hxx
  )
  set(OLSR_CORE_SRC ${ALL_SRC})
  list(FILTER OLSR_CORE_SRC EXCLUDE REGEX "/app/")
  list(FILTER OLSR_CORE_SRC EXCLUDE REGEX "/ui/")
  add_library(olsr_core STATIC ${OLSR_CORE_SRC})
  target_include_directories(olsr_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_link_libraries(olsr_core PUBLIC nlohmann_json)
endif()

if(NOT TARGET olsr_ui)
  file(GLOB_RECURSE OLSR_UI_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.hxx
  )
  add_library(olsr_ui STATIC ${OLSR_UI_SRC})
  target_include_directories(olsr_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_link_libraries(olsr_ui PUBLIC olsr_core imgui_lib)
endif()

if(OLSR_LITE_WITH_GUI)
  add_executable(olsr_lite
    src/app/Main.cpp
    src/app/MainGui.cpp
  )
  target_include_directories(olsr_lite PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_link_libraries(olsr_lite PRIVATE olsr_core olsr_ui imgui_lib ${GLAD_TGT} glfw)
else()
  add_executable(olsr_lite
    src/app/Main.cpp
  )
  target_include_directories(olsr_lite PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_link_libraries(olsr_lite PRIVATE olsr_core)
endif()

set_target_properties(olsr_lite PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
  set_target_properties(olsr_lite PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_${cfg} "${CMAKE_BINARY_DIR}"
  )
endforeach()

install(TARGETS olsr_lite RUNTIME DESTINATION bin)
