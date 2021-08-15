#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include <threads/lfthpool.h>

#include <ctest.h>

CTEST(lfthpool_no_work, test) {
	lfthpool_t pool = lfthpool_create(8, 4);
	lfthpool_destroy(pool);
}
