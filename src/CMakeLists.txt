set (LWIP_INCLUDE_DIRS
    "${LWIP_DIR}/src/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

set (LWIP_LIBRARY_DIRS
    "${LWIP_DIR}/build/contrib/ports/unix/example_app"
)

set (LWIP_COMPILER_FLAGS
    "-pthread"
)

add_library(lwipcs cs.c sys.c tcp.c udp.c)
target_include_directories(lwipcs PRIVATE ${LWIP_INCLUDE_DIRS})
target_compile_options(lwipcs PRIVATE ${LWIP_COMPILER_FLAGS})
target_link_directories(lwipcs PRIVATE ${LWIP_LIBRARY_DIRS})
target_link_libraries(lwipcs lwipcore)

add_executable(test-lwip-cs test.c)
target_include_directories(test-lwip-cs PRIVATE ${LWIP_INCLUDE_DIRS})
target_compile_options(test-lwip-cs PRIVATE ${LWIP_COMPILER_FLAGS})
target_link_directories(test-lwip-cs PRIVATE ${LWIP_LIBRARY_DIRS})
target_link_libraries(test-lwip-cs lwipcore lwipcs)
