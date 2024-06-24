set(CLANG "clang-14")
set(ARMCC "arm-linux-gnueabihf-gcc")

add_custom_target(vendor_libsysy
  COMMAND ${CLANG} -S -emit-llvm ${CMAKE_SOURCE_DIR}/vendor/libsysy/libsysy64.c -o ${CMAKE_BINARY_DIR}/vendor/libsysy/libsysy64.ll
  COMMAND ${ARMCC} -mcpu=cortex-a72 -S ${CMAKE_SOURCE_DIR}/vendor/libsysy/libsysy32.c -o ${CMAKE_BINARY_DIR}/vendor/libsysy/libsysy32.s
  DEPENDS ${CMAKE_SOURCE_DIR}/vendor/libsysy/libsysy64.c ${CMAKE_SOURCE_DIR}/vendor/libsysy/libsysy32.c
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
