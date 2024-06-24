set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-to-int-cast")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")
aux_source_directory(. VENDOR_LXML_SRCS)
add_library(vendor_lxml OBJECT ${VENDOR_LXML_SRCS})