foreach(name signals log fileio fifo bus_config)
  add_executable(test_${name} test_${name}.c)
  target_link_libraries(test_${name} PRIVATE vio)
  add_test(NAME ${name} COMMAND test_${name})
endforeach()

add_library(test_spi_slave MODULE fixtures/spi_slave.c)
add_library(test_i2c_slave MODULE fixtures/i2c_slave.c)
add_library(test_legacy_slave MODULE fixtures/legacy_slave.c)
add_library(test_abi2_slave MODULE fixtures/spi_slave.c)
target_compile_definitions(test_abi2_slave PRIVATE FIXTURE_ABI=2)
foreach(fixture test_spi_slave test_i2c_slave test_abi2_slave)
  target_include_directories(${fixture} PRIVATE ${PROJECT_SOURCE_DIR}/include)
endforeach()
set(SLAVE_FIXTURES test_spi_slave test_i2c_slave test_legacy_slave test_abi2_slave)
add_executable(test_slaves test_slaves.c)
target_link_libraries(test_slaves PRIVATE vio)
add_dependencies(test_slaves ${SLAVE_FIXTURES})
add_test(NAME slaves COMMAND test_slaves
  $<TARGET_FILE:test_spi_slave> $<TARGET_FILE:test_i2c_slave>)

add_executable(test_bus_init test_bus_init.c)
target_link_libraries(test_bus_init PRIVATE vio)
add_dependencies(test_bus_init ${SLAVE_FIXTURES})
add_test(NAME bus_init COMMAND test_bus_init
  $<TARGET_FILE:test_spi_slave> $<TARGET_FILE:test_i2c_slave>
  $<TARGET_FILE:test_legacy_slave> $<TARGET_FILE:test_abi2_slave>)

add_executable(test_i2c_10bit test_i2c_10bit.c)
target_link_libraries(test_i2c_10bit PRIVATE vio)
add_dependencies(test_i2c_10bit test_i2c_slave)
add_test(NAME i2c_10bit COMMAND test_i2c_10bit $<TARGET_FILE:test_i2c_slave>)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(NAME api_reference_up_to_date
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/gen_api_reference.py --check)
else()
  message(STATUS "Python 3 not found: skipping the API reference check")
endif()

add_test(NAME install_consumer
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/install_consumer.sh ${CMAKE_BINARY_DIR})
