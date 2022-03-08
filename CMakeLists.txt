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

# ffmpeg
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}")
find_package(FFmpeg COMPONENTS AVUTIL AVCODEC AVFORMAT SWRESAMPLE REQUIRED)
# Extra definitions
if (WIN32)
target_link_libraries(SAP PRIVATE mfplat.lib mfuuid.lib ole32.lib strmiids.lib bcrypt.lib ole32.lib user32.lib)
else()
target_link_options(SAP PRIVATE ${FFMPEG_DEFINITIONS})
endif()
# message("${FFMPEG_INCLUDE_DIRS}")
# message("${FFMPEG_LIBRARIES}")

# MiniAudio
set(MINIAUDIO_DIR "vendor/miniaudio")

# Includes
target_include_directories(SAP PRIVATE ${FFMPEG_INCLUDE_DIRS} ${MINIAUDIO_DIR})

# Links
target_link_libraries(SAP PRIVATE fmt::fmt ${FFMPEG_LIBRARIES})