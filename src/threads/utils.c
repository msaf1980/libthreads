#if defined(__MACH__) || defined(BSD)

#include <sys/types.h>
#include <sys/sysctl.h>

#else /* Linux and Solaris */

#include <unistd.h>

#endif

int threads_cpu_count()
{
#if defined(__MACH__) || defined(BSD)
	int cpus = 0;
	size_t len = sizeof(cpus);
	sysctlbyname("hw.ncpu", &cpus, &len, NULL, 0);
	return cpus;
#else /* Linux and Solaris */
	return (int) sysconf(_SC_NPROCESSORS_ONLN);
#endif
}
