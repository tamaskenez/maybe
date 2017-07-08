find_package(LLVM REQUIRED)

set(CMAKE_ENABLE_EXPORTS 1)

add_executable(kch9 toy.cpp)

set(LLVM_LINK_COMPONENTS
  Core
  ExecutionEngine
  Object
  Support
  native
)

foreach(n IN LISTS LLVM_AVAILABLE_LIBS) #${LLVM_LINK_COMPONENTS})
    target_link_libraries(kch9 PRIVATE ${n})
endforeach()

target_include_directories(kch9 PRIVATE ${LLVM_INCLUDE_DIRS})
