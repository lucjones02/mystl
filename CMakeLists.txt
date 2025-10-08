cmake_minimum_required(VERSION 3.28)
project(learning_data_structures)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(data_structures INTERFACE)
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/*.hpp")
target_include_directories(data_structures INTERFACE ${PROJECT_SOURCE_DIR}/include)

add_subdirectory(tests)
