add_executable(eosio.assert.wasm
   ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.assert.cpp
   )

target_compile_options(eosio.assert.wasm PUBLIC --std=c++17 -fcolor-diagnostics)

target_include_directories(eosio.assert.wasm
   PUBLIC 
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(eosio.assert.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/abi/eosio.assert.abi" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
