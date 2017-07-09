find_package(nowide REQUIRED)
find_package(microlib REQUIRED)

file(GLOB headers *.h)
add_executable(maybe
    ${headers} compiler.cpp lexer.cpp)

target_link_libraries(maybe PRIVATE
    nowide::nowide-static
    microlib::microlib
)
