
macro( srpc_all_headers_path OUT_LIST PREFIX_PATH )
    list( APPEND ${OUT_LIST}
        ${PREFIX_PATH}/srpc/server
        ${PREFIX_PATH}/srpc/common
        ${PREFIX_PATH}/srpc/client
    )
endmacro( )
