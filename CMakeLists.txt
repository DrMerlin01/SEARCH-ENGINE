cmake_minimum_required(VERSION 3.10)

project(SEARCH-ENGINE)
set(CMAKE_CXX_STANDARD 17)

add_executable("search_engine" 
               "src/main.cpp"
               "src/tests.cpp" 
               "src/test_example_functions.cpp" 
               "src/string_processing.cpp" 
               "src/search_server.cpp" 
               "src/request_queue.cpp" 
               "src/remove_duplicates.cpp" 
               "src/read_input_functions.cpp" 
               "src/process_queries.cpp" 
               "src/document.cpp" 
               "inc/assert.h"
               "inc/concurrent_map.h"
               "inc/document.h"
               "inc/log_duration.h"
               "inc/paginator.h"
               "inc/process_queries.h"
               "inc/read_input_functions.h"
               "inc/remove_duplicates.h"
               "inc/request_queue.h"
               "inc/search_server.h"
               "inc/string_processing.h"
               "inc/test_example_functions.h")