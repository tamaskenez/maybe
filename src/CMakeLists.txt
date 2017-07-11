find_package(nowide REQUIRED)
find_package(microlib REQUIRED)
find_package(fmt REQUIRED)
find_package(mpark_variant REQUIRED)

file(GLOB headers *.h)
add_executable(maybe
    ${headers}
    compiler.cpp lexer.cpp command_line.cpp log.cpp
    compiler_main.cpp
    filereader.cpp
    utils.cpp globals.cpp
)

target_link_libraries(maybe PRIVATE
    nowide::nowide-static
    microlib::microlib
    fmt::fmt
    mpark_variant
)
