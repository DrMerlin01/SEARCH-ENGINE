cmake_minimum_required(VERSION 3.10)

project(SEARCH-ENGINE)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_SYSTEM_NAME MATCHES "^MINGW")
    set(SYSTEM_LIBS -lstdc++)
else()
    set(SYSTEM_LIBS)
endif()

set(INCLUDE_DIR inc)
set(SOURCE_DIR src)

set(FILES_MAIN "${SOURCE_DIR}/main.cpp")
set(FILES_TESTS "${SOURCE_DIR}/tests.cpp" "${INCLUDE_DIR}/assert.h")
SET(FILES_SEARCH_ENGINE "${SOURCE_DIR}/test_example_functions.cpp" 
                        "${SOURCE_DIR}/string_processing.cpp" 
                        "${SOURCE_DIR}/search_server.cpp" 
                        "${SOURCE_DIR}/request_queue.cpp" 
                        "${SOURCE_DIR}/remove_duplicates.cpp" 
                        "${SOURCE_DIR}/read_input_functions.cpp" 
                        "${SOURCE_DIR}/process_queries.cpp" 
                        "${SOURCE_DIR}/document.cpp"
                        "${INCLUDE_DIR}/concurrent_map.h"
                        "${INCLUDE_DIR}/document.h"
                        "${INCLUDE_DIR}/log_duration.h"
                        "${INCLUDE_DIR}/paginator.h"
                        "${INCLUDE_DIR}/process_queries.h"
                        "${INCLUDE_DIR}/read_input_functions.h"
                        "${INCLUDE_DIR}/remove_duplicates.h"
                        "${INCLUDE_DIR}/request_queue.h"
                        "${INCLUDE_DIR}/search_server.h"
                        "${INCLUDE_DIR}/string_processing.h"
                        "${INCLUDE_DIR}/test_example_functions.h")
		        
source_group("Source" FILES ${FILES_MAIN})
source_group("Tests" FILES ${FILES_TESTS})
source_group("Search Engine" FILES ${FILES_SEARCH_ENGINE})

add_executable("search_engine" ${FILES_MAIN} ${FILES_TESTS} ${FILES_SEARCH_ENGINE})
target_link_libraries("search_engine" ${SYSTEM_LIBS})