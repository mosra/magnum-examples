# Next to the defaults we need also Java language
project(JavaViewer C CXX Java)

# CMake 2.8.8 is required for CMAKE_JAVA_JAR_ENTRY_POINT
cmake_minimum_required(VERSION 2.8.8)

find_package(Magnum REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Java REQUIRED)
find_package(JNI REQUIRED)

include(UseJava)

# Macro for generating JNI headers
function(jni_generate_header header_name jar_target)
    foreach(argument ${ARGN})
        set(classes ${classes} ${argument})
    endforeach()

    # It's documented as CLASS_DIR in UseJava.cmake, which is wrong
    get_property(class_dir TARGET ${jar_target} PROPERTY CLASSDIR)
    set(out ${header_name})
    add_custom_command(
        OUTPUT ${out}
        COMMAND ${Java_JAVAH_EXECUTABLE} -classpath ${class_dir} -o ${CMAKE_CURRENT_BINARY_DIR}/${out} ${classes}
        DEPENDS ${jar_target}
        )
    set(${header_name} ${out} PARENT_SCOPE)
endfunction()

if(NOT WIN32)
    set(PLUGIN_IMPORTER_DIR ${MAGNUM_PLUGINS_IMPORTER_INSTALL_DIR})
else()
    set(PLUGIN_IMPORTER_DIR importers)
endif()

# JAR and native header
find_jar(STANFORDIMPORTER_JAR JStanfordImporter PATHS ${PLUGIN_IMPORTER_DIR})
if(STANFORDIMPORTER_JAR-NOTFOUND)
    message(ERROR "StanfordImporter JAR was not found")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/JavaViewer.manifest.cmake ${CMAKE_CURRENT_BINARY_DIR}/JavaViewer.manifest)

set(JavaViewer_Java_SRCS
    cz/mosra/magnum/JavaViewer/JavaViewer.java
    cz/mosra/magnum/JavaViewer/NativeCanvas.java
    )

set(CMAKE_JAVA_COMPILE_FLAGS -Xlint)
set(CMAKE_JAVA_JAR_ENTRY_POINT cz/mosra/magnum/JavaViewer/JavaViewer)
set(CMAKE_JAVA_JAR_MANIFEST_FILE ${CMAKE_CURRENT_BINARY_DIR}/JavaViewer.manifest)
add_jar(JavaViewer ${JavaViewer_Java_SRCS})

jni_generate_header(NativeCanvas.h JavaViewer
    cz.mosra.magnum.JavaViewer.JavaViewer
    cz.mosra.magnum.JavaViewer.NativeCanvas
    )

# Native library
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

include_directories(${JNI_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

corrade_add_resource(Shaders shaders PhongShader.frag PhongShader.vert)

set(JavaViewer_CXX_SRCS
    JavaViewer.cpp
    NativeCanvas.h
    NativeCanvas.cpp
    PhongShader.cpp
    ViewedObject.cpp
    ${Shaders}
    )

add_library(NativeCanvas SHARED ${JavaViewer_CXX_SRCS})
target_link_libraries(NativeCanvas ${CORRADE_UTILITY_LIBRARY} ${MAGNUM_LIBRARY} ${JAVA_AWT_LIBRARY} ${OPENGL_gl_LIBRARY})
