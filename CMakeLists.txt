cmake_minimum_required(VERSION 3.16)
project(LibVIO VERSION 1.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(VIO_BUILD_TESTS "Build the LibVIO tests" ON)

# Vendored cJSON, built with hidden symbols so libvio.so does not export
# cJSON_* and cannot clash with another cJSON in the same process.
add_library(vio_cjson OBJECT src/third_party/cjson/cJSON.c)
set_target_properties(vio_cjson PROPERTIES
  C_VISIBILITY_PRESET hidden
  POSITION_INDEPENDENT_CODE ON
)

add_library(vio SHARED
  $<TARGET_OBJECTS:vio_cjson>
  src/log.c
  src/config/bus_config.c
  src/slave/slave_loader.c
  src/adc/adc.c
  src/bus/signals.c
  src/dma/dma.c
  src/fifo/fifo.c
  src/fileio/fileio.c
  src/i2c/i2c.c
  src/i2c/i2c_helper.c
  src/net/net.c
  src/pty/pty.c
  src/spi/spi.c
  src/spi/spi_helper.c
)
add_library(vio::vio ALIAS vio)
set_target_properties(vio PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)
target_include_directories(vio PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_options(vio PRIVATE -g -Wall)
target_link_libraries(vio PRIVATE ${CMAKE_DL_LIBS} m)
# Fail at link time, not at dlopen time, if a symbol is missing.
target_link_options(vio PRIVATE -Wl,--no-undefined)

install(TARGETS vio EXPORT vioTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/vio DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# The model contract: API reference, host functions and its version, for
# tools that write device models. Found with
# `pkg-config --variable=contractdir vio` or VIO_CONTRACT_DIR (CMake).
set(VIO_CONTRACT_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/vio/contract)
install(DIRECTORY contract/ DESTINATION ${VIO_CONTRACT_INSTALL_DIR})

install(EXPORT vioTargets
  NAMESPACE vio::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vio
)
configure_package_config_file(cmake/vioConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/vioConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vio
  PATH_VARS VIO_CONTRACT_INSTALL_DIR
)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/vioConfigVersion.cmake
  COMPATIBILITY SameMajorVersion
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/vioConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/vioConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vio
)

# vio.pc finds the prefix from its own location, so `cmake --install
# --prefix` and moved installs work.
file(RELATIVE_PATH VIO_PC_TO_PREFIX "/${CMAKE_INSTALL_LIBDIR}/pkgconfig" "/")
string(REGEX REPLACE "/$" "" VIO_PC_TO_PREFIX "${VIO_PC_TO_PREFIX}")
configure_file(cmake/vio.pc.in ${CMAKE_CURRENT_BINARY_DIR}/vio.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vio.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# Apache-2.0 asks redistributions to carry the license and the notice.
install(FILES LICENSE NOTICE DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/vio)

if(VIO_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
