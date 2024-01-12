add_library(doctest
  doctest.h
  main.cpp
)

target_include_directories(doctest
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)