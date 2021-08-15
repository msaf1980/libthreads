#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include <threads/thpool.h>

#include <ctest.h>

CTEST(thpool_no_work, test) {
	thpool_t pool = thpool_create(8, 4);
	thpool_destroy(pool);
}
