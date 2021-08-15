# Scan source dir for standart source files and exclude main.cpp

include_directories(${PROJECT_SOURCE_DIR}/src)

set(REQUIRED_SOURCES)

if(NOT HAVE_PTHREAD_BARRIER)
    add_definitions(-DNO_PTHREAD_BARRIER=1)
    list(APPEND REQUIRED_SOURCES ${PROJECT_SOURCE_DIR}/src/pthread_barrier.c)
endif()

set(TEST_LIBRARIES Threads::Threads)

if(BUILD_SHARED_LIBS)
    list(APPEND TEST_LIBRARIES threads_shared ${LIBCONCURENT_SHARED})
else()
    list(APPEND TEST_LIBRARIES threads ${LIBCONCURENT})
endif()

# Build tests
add_executable(test_utils
    utils_test.c
    ${REQUIRED_SOURCES}
)
target_link_libraries(test_utils ${TEST_LIBRARIES})
#target_compile_options(test_thpool PRIVATE -Wdeclaration-after-statement)
add_test(
    NAME test_utils
    COMMAND $<TARGET_FILE:test_utils>
)
set_tests_properties(test_utils PROPERTIES LABELS "utils")

add_executable(test_usem
    usem_test.c
    ${REQUIRED_SOURCES}
)
target_link_libraries(test_usem ${TEST_LIBRARIES})
add_test(
    NAME test_usem
    COMMAND $<TARGET_FILE:test_usem>
)
set_tests_properties(test_usem PROPERTIES LABELS "usem")

add_executable(test_lusem
    lusem_test.c
    ${REQUIRED_SOURCES}
)
target_link_libraries(test_lusem ${TEST_LIBRARIES})
add_test(
    NAME test_lusem
    COMMAND $<TARGET_FILE:test_lusem>
)
set_tests_properties(test_lusem PROPERTIES LABELS "lusem")

add_executable(test_psem
    psem_test.c
    ${REQUIRED_SOURCES}
)
target_link_libraries(test_psem ${TEST_LIBRARIES})
add_test(
    NAME test_psem
    COMMAND $<TARGET_FILE:test_psem>
)
set_tests_properties(test_utils PROPERTIES LABELS "psem")

add_executable(test_thpool
    thpool_test.c
    thpool/thpool_no_work.c
    thpool/thpool_api.c
    thpool/thpool_pause_resume.c
    thpool/thpool_wait.c
    thpool/thpool_worker_try_once.c
    ${REQUIRED_SOURCES}
)
target_link_libraries(test_thpool ${TEST_LIBRARIES})
#target_compile_options(test_thpool PRIVATE -Wdeclaration-after-statement)
add_test(
    NAME test_thpool
    COMMAND $<TARGET_FILE:test_thpool>
)
set_tests_properties(test_thpool PROPERTIES LABELS "thpool")

add_executable(bench_thpool thpool_bench.c ${REQUIRED_SOURCES})
target_link_libraries(bench_thpool ${TEST_LIBRARIES})

add_executable(test_lfthpool
    lfthpool_test.c
    lfthpool/lfthpool_no_work.c
    lfthpool/lfthpool_api.c
    lfthpool/lfthpool_pause_resume.c
    lfthpool/lfthpool_worker_try_once.c
    ${REQUIRED_SOURCES}
)
target_link_libraries(test_lfthpool ${TEST_LIBRARIES})
#target_compile_options(test_lfthpool PRIVATE -Wdeclaration-after-statement)
add_test(
    NAME test_lfthpool
    COMMAND $<TARGET_FILE:test_lfthpool>
)
set_tests_properties(test_lfthpool PROPERTIES LABELS "lfthpool")

add_executable(bench_lfthpool lfthpool_bench.c ${REQUIRED_SOURCES})
target_link_libraries(bench_lfthpool ${TEST_LIBRARIES})
