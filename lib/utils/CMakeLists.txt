set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-to-int-cast")
aux_source_directory(dsa UTILS_SRCS)
aux_source_directory(printer UTILS_SRCS)
add_library(utils OBJECT ${UTILS_SRCS})
