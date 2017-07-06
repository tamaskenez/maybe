find_package(LLVM REQUIRED)

set(CMAKE_ENABLE_EXPORTS 1)

add_executable(kch9 toy.cpp)
target_link_libraries(kch9 PRIVATE )

set(LLVM_LINK_COMPONENTS
  Core
  ExecutionEngine
  Object
  Support
  native
  )

