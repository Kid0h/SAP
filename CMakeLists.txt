cmake_minimum_required(VERSION 3.20.0)
project(SAP VERSION 1.0.0)

# General
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

if (WIN32)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

# Sources
file(GLOB SOURCES
    "src/*.cpp"
    "src/*.c"
)

# Build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SAP_DEBUG True)
    add_definitions(-DSAP_DEBUG=1)
endif()

# Executable
add_executable(SAP ${SOURCES})

# FMT
add_subdirectory("vendor/fmt")

# libav
set(LIBAV_INCLUDE_DIR "vendor/ffmpeg/build/include")
set(LIBAV_LIBRARY_DIR "vendor/ffmpeg/build/lib")

# MiniAudio
set(MINIAUDIO_DIR "vendor/miniaudio")

# Includes
target_include_directories(SAP PRIVATE ${LIBAV_INCLUDE_DIR} ${MINIAUDIO_DIR})

# Links
target_link_directories(SAP PRIVATE ${LIBAV_LIBRARY_DIR})
target_link_libraries(SAP PRIVATE fmt::fmt "Bcrypt.lib" "libavutil" "libavcodec" "libavformat" "libswresample" "Strmiids.lib" "Mfplat.lib" "Mfuuid.lib")