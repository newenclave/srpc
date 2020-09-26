
set(exe_name srpc_test)

include( "../srpc_files.cmake" )

set(SRPC_DIR_LIST)
srpc_all_headers_path( SRPC_DIR_LIST "../include" )

list( APPEND src_dirs
      .
      ${SRPC_DIR_LIST} 
      )

foreach( src_dir ${src_dirs} )
    aux_source_directory(${src_dir} exe_src)
    file(GLOB headers ${src_dir}/*.h)
    list(APPEND exe_src ${headers})
endforeach( )

include_directories( ${INCLUDE_DIR}/srpc/common )

add_executable(${exe_name} ${exe_src})

target_link_libraries(${exe_name} ${Protobuf_LIBRARIES})

if(WIN32)
    target_link_libraries(${exe_name} ws2_32.lib)
endif(WIN32)

if(MINGW)
    target_link_libraries( ${exe_name} ws2_32 mswsock )
endif(MINGW)


if(Protobuf_FOUND)
    target_link_libraries( ${exe_name} common_protobuf )
endif(Protobuf_FOUND)