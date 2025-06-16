# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html. For
# more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.25.0)

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Declares the project name. The project name can be accessed via ${
# PROJECT_NAME}, Since this is the top level CMakeLists.txt, the project name is
# also accessible with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync
# within the top level build script scope).
project("native-activity" C)

set(BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE) # don't build the supplied examples
string(REGEX MATCH "android-([0-9]+)" _ ${ANDROID_PLATFORM} FORCE)
set(ANDROID_API_VERSION
    ${CMAKE_MATCH_1}
    CACHE STRING "" FORCE)
set(ANDROID_ARCH
    ${CMAKE_ANDROID_ARCH}
    CACHE STRING "" FORCE)
set(PLATFORM
    "Android"
    CACHE STRING "" FORCE)

add_subdirectory(deps/raylib)

include_directories(deps/libav/${ANDROID_ABI}/include)

set(libav-libs swresample avutil avcodec avformat)
foreach(lib IN LISTS libav-libs)
  add_library(${lib} SHARED IMPORTED)
  set_target_properties(
    ${lib}
    PROPERTIES IMPORTED_LOCATION
               ${CMAKE_SOURCE_DIR}/deps/libav/${ANDROID_ABI}/lib${lib}.so)
endforeach()

# build native_app_glue as a static lib

include_directories(${CMAKE_PROJECT_NAME}
                    ${NDK}/sources/android/native_app_glue)

add_library(native_app_glue STATIC
            ${NDK}/sources/android/native_app_glue/android_native_app_glue.c)

set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here; for
# GameActivity/NativeActivity derived applications, the same library name must
# be used in the AndroidManifest.xml file.
add_library(
  ${CMAKE_PROJECT_NAME} SHARED
  # List C/C++ source files with relative paths to this CMakeLists.txt.
  main.c render.c audio.c aaudio_bind.c libav_bind.c)

# Specifies libraries CMake should link to your target library. You can link
# libraries from various origins, such as libraries defined in this build
# script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  # List libraries link to the target library
  android
  native_app_glue
  raylib
  aaudio
  ${libav-libs}
  log)
