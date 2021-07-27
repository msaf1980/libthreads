# Scan source dir for standart source files and exclude main.cpp

include_directories(${PROJECT_SOURCE_DIR}/src)

set(REQUIRED_SOURCES)

# if(NOT HAVE_PTHREAD_BARRIER)
#     add_definitions(-DNO_PTHREAD_BARRIER=1)
#     list(APPEND REQUIRED_SOURCES ${PROJECT_SOURCE_DIR}/src/pthread_barrier.c)
# endif()

set(TEST_LIBRARIES threads Threads::Threads)

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
