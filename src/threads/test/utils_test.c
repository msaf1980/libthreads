#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <threads/utils.h>

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include <ctest.h>

CTEST(utils, threads_cpu_count) {
	long cpu = threads_cpu_count();
    printf(" (cpu %ld) ", cpu);
    ASSERT_TRUE(cpu > 0);
}

int main(int argc, const char *argv[]) {
    return ctest_main(argc, argv);
}
