#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <threads/lfthpool.h>

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include <ctest.h>

int main(int argc, const char *argv[]) {
    return ctest_main(argc, argv);
}
