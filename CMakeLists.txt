cmake_minimum_required(VERSION 3.30.2 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 23)

project(ImageCreator LANGUAGES C CXX)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.c)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GUID_WINDOWS)
elseif(APPLE)
    find_library(CFLIB CoreFoundation)
    target_link_libraries(${PROJECT_NAME} ${CFLIB})
    target_compile_definitions(${PROJECT_NAME} PRIVATE GUID_CFUUID)
elseif(ANDROID)
    # GUID_ANDROID is used in the headers, so make PUBLIC
    target_compile_definitions(${PROJECT_NAME} PUBLIC GUID_ANDROID)
else()
    find_package(Libuuid REQUIRED)
    if (NOT LIBUUID_FOUND)
        message(FATAL_ERROR
            "You might need to run 'sudo apt-get install uuid-dev' or similar")
    endif()
    target_compile_definitions(${PROJECT_NAME} PRIVATE GUID_LIBUUID)
endif()
