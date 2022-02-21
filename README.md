# SEARCH-ENGINE
Document search engine with negative keywords (documents with these words will not be displayed in the search results). It works in the likeness of a search engine, such as 
Yandex. The ranking of the results is based on TF-IDF.
# Building and Run
GCC is used for the assembly:
```
  1. g++ main.cpp tests.cpp test_example_functions.cpp string_processing.cpp search_server.cpp request_queue.cpp remove_duplicates.cpp read_input_functions.cpp process_queries.cpp document.cpp assert.cpp -Wall -Werror -std=c++17 -o search_engine
  2. Start ./search_engine or search_engine.exe
```
# System requirements and Stack
  1. C++17
  2. GCC version 8.1.0
  3. TF-IDF
# Future plans
  1. Add a script to build the project
