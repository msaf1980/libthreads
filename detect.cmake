include(CheckFunctionExists)
include(CheckSymbolExists)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED) # Threads::Threads
set(CMAKE_REQUIRED_LIBRARIES Threads::Threads) # required libs for check_function_exists
check_symbol_exists(pthread_barrier_init pthread.h HAVE_PTHREAD_BARRIER) # for MacOS
check_symbol_exists(pthread_spin_init pthread.h HAVE_PTHREAD_SPIN) # for MacOS
#check_function_exists(pthread_spin_init HAVE_PTHREAD_SPIN) # for MacOS
