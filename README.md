# SEARCH-ENGINE
Document search engine with negative keywords (documents with these words will not be displayed in the search results). It works in the likeness of a search engine, such as 
Yandex. The ranking of the results is based on TF-IDF.
# Building and Run
```
  1. mkdir BuildSearchEngine && cd BuildSearchEngine
  2. cmake ..
  3. cmake --build .
  4. Start ./search_engine or search_engine.exe
```
# System requirements and Stack
  1. C++17
  2. GCC version 8.1.0
  3. Cmake 3.21.2 (minimal 3.10)
  4. TF-IDF
# Future plans
  1. Add a user interface for adding documents and searching through them
  2. Add tests for all functionality
