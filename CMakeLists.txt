
#to build cmake -DProtobuf_INCLUDE_DIR="C:\sdk_root\external\protobuf\include" -DProtobuf_LIBRARY="C:\sdk_root\external\protobuf\lib\libprotobuf.lib" -DProtobuf_LIBRARY_DEBUG="C:\sdk_root\external\protobuf\lib\libprotobufd.lib" -DProtobuf_PROTOC_EXECUTABLE="C:\github\protobuf\build_msvc\Debug\protoc.exe" -DProtobuf_SRC_ROOT_FOLDER="C:\github\protobuf\src" ../../srpc

if(WIN32)
    set(
        CMAKE_USER_MAKE_RULES_OVERRIDE
        "${CMAKE_CURRENT_SOURCE_DIR}/msvc_override.cmake"
    )
endif(WIN32)

cmake_minimum_required(VERSION 2.8)

project(srpc)

find_package( Protobuf REQUIRED)

set( INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include )

message("Include dir: " ${PROJECT_SOURCE_DIR}/include)

set( INCLUDE_DIRS
     ${INCLUDE_DIR}
     ${PROTOCOL_INCLUDE_DIR}
     ${PROTOBUF_INCLUDE_DIRS}
   )

include_directories( ${PROJECT_SOURCE_DIR} ${INCLUDE_DIRS} )

add_subdirectory(tests)
