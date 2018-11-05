
aux_source_directory(. src_dir)

set(exe_name srpc_test)

add_executable(${exe_name} ${src_dir})


target_link_libraries(${exe_name} ${Protobuf_LIBRARIES})

if(WIN32)
    target_link_libraries(${exe_name} ws2_32.lib)
endif(WIN32)

if(MINGW)
    target_link_libraries( ${exe_name} ws2_32 mswsock )
endif(MINGW)

