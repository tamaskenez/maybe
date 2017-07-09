#include "nowide/args.hpp"
#include "nowide/cstdio.hpp"
#include "nowide/cstdlib.hpp"

#include "ul/ul.h"

int main(int argc, char* argv[])
{
    try {
    nowide::args nwa(argc, argv); // converts args to utf8 (windows-only)
        FOR(i, 0, argc) {
            printf("%d: %s\n", i, argv[i]);
        }
        return EXIT_SUCCESS;
    } catch(std::exception& e) {
        fprintf(stderr, "Aborting, exception: %s\n", e.what());
    } catch(...) {
        fprintf(stderr, "Aborting, unknown exception\n");
    }
    return EXIT_FAILURE;
}
