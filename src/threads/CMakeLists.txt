set(
    THREADS_SOURCES
    utils.c
    lusem.c
    thpool.c
    lfthpool.c
)

if(BUILD_SHARED_LIBS)
    add_library(threads_shared SHARED ${THREADS_SOURCES})
    target_link_libraries(threads_shared ${LIBCONCURRENT_SHARED})
    set_target_properties(
        threads_shared
        PROPERTIES OUTPUT_NAME threads
                   SOVERSION "${VERSION_MAJOR}"
                   VERSION "${VERSION}")
endif()
add_library(threads STATIC ${THREADS_SOURCES})
target_link_libraries(threads ${LIBCONCURENT})
set_target_properties(threads PROPERTIES OUTPUT_NAME threads)

include(GNUInstallDirs)
install(TARGETS threads
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/threads"
        DESTINATION "include"
)


configure_file(${PROJECT_SOURCE_DIR}/libthreads.pc.in ${CMAKE_BINARY_DIR}/libthreads.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/libthreads.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

if (BUILD_TEST)
   add_subdirectory(test)
endif() # END TEST
